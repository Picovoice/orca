import io
import json
import librosa
import numpy as np
import os
import requests
import shutil
import sys
import zipfile

from argparse import ArgumentParser

GITLAB_API_URL = "https://gitlab.com/api/v4/projects"

NUM_AUDIOS = 150

JOB_NAMES = [
    "build-beta-linux-release",
    "build-beta-mac-m1-release",
    "build-beta-windows-x86-release",
    "build-beta-windows-arm64-release",
    "build-beta-rpi3-release",
    "build-beta-rpi3-64-release",
    "build-beta-rpi4-release",
    "build-beta-rpi4-64-release",
    "build-beta-rpi5-release",
    "build-beta-rpi5-64-release",
]

REFERENCE_JOB_NAME = "build-beta-linux-release"

AVERAGE_THRESHOLD = 0.005
OUTLIER_THRESHOLD = 0.01
OUTLIER_COUNT_THRESHOLD = 0.05


def get_pipeline_artifacts(
        project_id: str,
        gitlab_api_token,
        job_name,
        pipeline_id,
        perf_artifact_path):
    perf_job_req = \
        f"{GITLAB_API_URL}/{project_id}/pipelines/" \
        f"{pipeline_id}/jobs?private_token={gitlab_api_token}&per_page=100"
    res = requests.get(perf_job_req)
    res_content = json.loads(res.text)

    perf_job = next((x for x in res_content if x["name"] == job_name), None)
    if perf_job is None:
        print(
            f"Cannot retrieve artifacts because job `{job_name}` not found in the pipeline results "
            f"`{pipeline_id}`. Try to retrieve more jobs in your request by increasing the `per_page` value.")
        sys.exit(1)

    print(f"Getting artifacts from last successful job "
          f"`{job_name}` ({perf_job['id']})...")
    perf_artifact_req = \
        f"{GITLAB_API_URL}/{project_id}/jobs/{perf_job['id']}/artifacts?" \
        f"private_token={gitlab_api_token}"
    res = requests.get(perf_artifact_req)
    if not res.ok or res.content is None:
        print(f"No artifacts were found associated with job "
              f"{job_name}` ({perf_job['id']})")
        sys.exit(1)

    with zipfile.ZipFile(io.BytesIO(res.content)) as zf:
        zf.extractall(perf_artifact_path)


def check_relative(
        gitlab_api_token: str,
        target_branch_name: str,
        project_id: str,
        branch_pipeline_id: str):
    print(f"Fetching pipelines for project_id `{project_id}` and target_branch `{target_branch_name}`...")
    pipeline_id_req = \
        f"{GITLAB_API_URL}/{project_id}/pipelines?" \
        f"ref={target_branch_name}&status=success&private_token={gitlab_api_token}"
    res = requests.get(pipeline_id_req)
    res_content = json.loads(res.text)
    pipeline_id = None
    print(f"Found the following pipeline_ids: `{[p['id'] for p in res_content]}`")
    for pipeline in res_content:
        if "schedule" not in pipeline["source"]:
            pipeline_id = pipeline["id"]
            break
    if pipeline_id is None:
        print(f"Cannot retrieve target performance because no valid pipeline was found.")
        sys.exit(1)

    for job_name in JOB_NAMES:
        print(f"Getting target job `{job_name}` for pipeline id `{pipeline_id}`...")
        target_artifact_path = os.path.join("artifacts", "target", job_name)
        get_pipeline_artifacts(
            project_id,
            gitlab_api_token,
            job_name,
            pipeline_id,
            target_artifact_path)

        print(f"Getting branch job `{job_name}` for pipeline id `{branch_pipeline_id}`...")
        branch_artifact_path = os.path.join("artifacts", "branch", job_name)
        get_pipeline_artifacts(
            project_id,
            gitlab_api_token,
            job_name,
            branch_pipeline_id,
            branch_artifact_path)

    failures = 0
    for job_name in JOB_NAMES:
        successes = 0

        for index in range(NUM_AUDIOS):
            target_artifact_path = os.path.join("artifacts", "target", job_name, f'res/pcm/{index:03d}.wav')
            branch_artifact_path = os.path.join("artifacts", "branch", job_name, f'res/pcm/{index:03d}.wav')
            title = f'branch vs {target_branch_name} {job_name}/{index}.wav'

            target_pcm, _ = librosa.load(target_artifact_path)
            branch_pcm, _ = librosa.load(branch_artifact_path)

            if target_pcm.shape != branch_pcm.shape:
                print(f'Length difference for {title} ({target_pcm.shape} != {branch_pcm.shape})')
                continue

            diff_pcm = np.abs(branch_pcm - target_pcm)
            diff_mean = diff_pcm.mean()
            diff_max = diff_pcm.max()
            diff_outliers = np.mean(diff_pcm > OUTLIER_THRESHOLD)

            if diff_mean > AVERAGE_THRESHOLD or diff_outliers > OUTLIER_COUNT_THRESHOLD:
                print(f'{title} {diff_mean:.06f} {diff_max:.06f} {diff_outliers:.06f}')
                failures += 1
            else:
                successes += 1

        if successes < NUM_AUDIOS * 0.95:
            failures += 1

    if failures > 0:
        exit(1)


def main():
    parser = ArgumentParser()
    parser.add_argument('--gitlab_api_token', required=True)
    parser.add_argument('--target_branch_name', required=True)
    parser.add_argument('--project_id', required=True)
    parser.add_argument('--branch_pipeline_id', required=True)
    args = parser.parse_args()

    check_relative(
        args.gitlab_api_token,
        args.target_branch_name,
        args.project_id,
        args.branch_pipeline_id)


if __name__ == '__main__':
    main()
