import argparse
import os
import subprocess
import tempfile
from uuid import uuid4

from _util import (
    Languages,
    Speakers,
    speaker_language_dict,
)

# Keep the following in sync with GitHub test data
TEST_SENTENCES = {
    Languages.ENGLISH.value: "It doesn't matter how slowly you go, as long as you do not stop!",
    Languages.SPANISH.value: "No importa lo lento que vayas, ¡siempre que no te detengas!"
}

ALIGNMENT_TEST_SENTENCES = {
    Languages.ENGLISH.value: "Test alignment.",
    Languages.SPANISH.value: "Pruebe la alineación."
}

EXACT_ALIGNMENT_TEST_MODEL_IDENTIFIER = {
    Languages.ENGLISH.value: "orca_params_en_female",
    Languages.SPANISH.value: "orca_params_es_female"
}

RANDOM_STATE = 42


def infer_language_from_speaker(speaker: Speakers) -> str:
    return speaker_language_dict[speaker].value


def main(access_key, test_resource_folder) -> None:

    for x in Speakers:
        language = infer_language_from_speaker(x)
        serializer_path = os.path.join(
            os.path.dirname(__file__),
            f'../build/release/x86_64/pv_orca_serializer_{x.value}')
        serializer_path_large = os.path.join(
            os.path.dirname(__file__),
            f'../build/release/x86_64/pv_orca_serializer_{x.value}_16')
        hippo_language_info_path = os.path.join(
            os.path.dirname(__file__),
            f'../res/core/pv_language_info_{language}.json'
        )
        orca_language_info_path = os.path.join(
            os.path.dirname(__file__),
            f'../res/language_info/pv_language_info_orca_normalizer_{language}.json'
        )
        model_name = f"orca_params_{x.value}"
        param_path = os.path.join(
            os.path.dirname(__file__),
            f'../res/param/{model_name}.pv')

        large_model_name = f"orca_params_{x.value}_16"
        large_param_path = os.path.join(
            os.path.dirname(__file__),
            f'../res/param/{large_model_name}.pv')

        cmd_large = None
        if Languages.JAPANESE.value == language:
            tokenizer_data_path = os.path.join(
                os.path.dirname(__file__),
                f'../res/normalizer/tokenizer_data/ipadic.bin'
            )
            cmd = [
                os.path.abspath(serializer_path),
                '-h', os.path.abspath(hippo_language_info_path),
                '-l', os.path.abspath(orca_language_info_path),
                '-d', os.path.abspath(tokenizer_data_path),
                '-o', os.path.abspath(param_path)]
        else:
            cmd = [
                os.path.abspath(serializer_path),
                '-h', os.path.abspath(hippo_language_info_path),
                '-l', os.path.abspath(orca_language_info_path),
                '-o', os.path.abspath(param_path)]
            if os.path.isfile(serializer_path_large):
                cmd_large = [
                    os.path.abspath(serializer_path_large),
                    '-h', os.path.abspath(hippo_language_info_path),
                    '-l', os.path.abspath(orca_language_info_path),
                    '-o', os.path.abspath(large_param_path)
                ]

        subprocess.run(cmd)
        if cmd_large:
            subprocess.run(cmd_large)

        if access_key is not None and test_resource_folder is not None and language in TEST_SENTENCES:
            app_path = os.path.join(os.path.dirname(__file__), f'../build/release/x86_64/pv_orca_app')
            streaming_app_path = \
                os.path.join(
                    os.path.dirname(__file__),
                    f'../build/release/x86_64/pv_orca_input_output_streaming_app')

            for path, identifier in zip([app_path, streaming_app_path], ["single", "stream"]):
                github_test_file_path = os.path.join(test_resource_folder, f"{model_name}_{identifier}.wav")

                synthesize_cmd = [
                    os.path.abspath(path),
                    '-a', access_key,
                    '-m', os.path.abspath(param_path),
                    '-t', TEST_SENTENCES[language],
                    '-s', str(RANDOM_STATE),
                    '-o', github_test_file_path,
                ]

                print(f"Produced pv file for model `{model_name}`. Now producing test files.")
                subprocess.run(synthesize_cmd)
                print(f"\033[92mSaved wav to use in github tests to `{github_test_file_path}` \033[0m")
                print(f"`{' '.join(synthesize_cmd)}`")

                if identifier == "single" and EXACT_ALIGNMENT_TEST_MODEL_IDENTIFIER[language] in model_name:
                    print(f"Producing alignment test file for model `{model_name}`.")

                    tmp_output_path = os.path.join(tempfile.gettempdir(), f"{uuid4()}.wav")
                    synthesize_cmd = [
                        os.path.abspath(path),
                        '-a', access_key,
                        '-m', os.path.abspath(param_path),
                        '-t', ALIGNMENT_TEST_SENTENCES[language],
                        '-s', str(RANDOM_STATE),
                        '-o', tmp_output_path,
                        '-v', '1',
                    ]
                    subprocess.run(synthesize_cmd)
                    if os.path.exists(tmp_output_path):
                        os.remove(tmp_output_path)

                    print(f"\033[92mMake sure to update the test data in GitHub to the alignments above.\033[0m")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--access_key",
        type=str,
        default=None,
        help="Picovoice AccessKey")
    parser.add_argument(
        "--test_resource_folder",
        type=str,
        default=None,
        help="Path to the folder in Orca GitHub where the test resources will be saved.")
    args = parser.parse_args()

    main(args.access_key, args.test_resource_folder)
