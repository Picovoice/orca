#
#    Copyright 2024-2025 Picovoice Inc.
#
#    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
#    file accompanying this source.
#
#    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
#    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
#    specific language governing permissions and limitations under the License.
#

import argparse
import json
import os
import struct
import time
import wave

from pvorca import create, OrcaActivationLimitError
from typing import List


def get_available_languages() -> List[str]:
    test_data_path = os.path.join(os.path.dirname(__file__), "../../resources/.test/test_data.json")
    with open(test_data_path, "r", encoding="utf-8") as f:
        test_data = json.load(f)

    return [x["language"] for x in test_data["tests"]["sentence_tests"]]


def get_available_genders() -> List[str]:
    return ["male", "female"]


def get_model_path(language: str, gender: str) -> str:
    model_name = f'orca_params_{language}_{gender}.pv'
    model_path = os.path.join(os.path.dirname(__file__), f"../../lib/common/{model_name}")
    if os.path.exists(model_path):
        return model_path
    else:
        available_gender = None
        models_dir = os.path.join(os.path.dirname(__file__), "../../lib/common")
        for filename in os.listdir(models_dir):
            if filename.startswith(f"orca_params_{language}_") and os.path.isfile(os.path.join(models_dir, filename)):
                available_gender = filename.split(".")[0].split("_")[-1]

        raise ValueError(f"Gender '{gender}' is not available with language '{language}'. "
                         f"Please use gender '{available_gender}'.")


def main() -> None:
    available_languages = get_available_languages()
    available_genders = get_available_genders()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--access_key',
        '-a',
        required=True,
        help='AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)')
    parser.add_argument(
        '--text',
        '-t',
        required=True,
        help='Text to be synthesized')
    parser.add_argument(
        '--output_path',
        '-o',
        required=True,
        help='Absolute path to .wav file where the generated audio will be stored')
    parser.add_argument(
        '--library_path',
        '-l',
        help='Absolute path to dynamic library. Default: using the library provided by `pvorca`')
    parser.add_argument(
        "--model_path",
        "-m",
        help="Absolute path to Orca model")
    parser.add_argument(
        "--language",
        help=f"The language you would like to run the demo in. "
             f"Available languages are {', '.join(available_languages)}.")
    parser.add_argument(
        "--gender",
        help=f"The gender of the synthesized voice. "
             f"Available genders are {', '.join(available_genders)}.")
    args = parser.parse_args()

    access_key = args.access_key
    model_path = args.model_path
    language = args.language
    gender = args.gender
    library_path = args.library_path
    output_path = args.output_path
    text = args.text

    if not model_path:
        if language not in available_languages:
            raise ValueError(f"Given argument --language `{language}` is not an available language. "
                             f"Available languages are {', '.join(available_languages)}.")
        if gender not in available_genders:
            raise ValueError(f"Given argument --gender `{gender}` is not an available gender. "
                             f"Available genders are {', '.join(available_genders)}.")
        model_path = get_model_path(language, gender)

    if not output_path.lower().endswith('.wav'):
        raise ValueError('Given argument --output_path must have WAV file extension')

    orca = create(access_key=access_key, model_path=model_path, library_path=library_path)

    try:
        print(f"Orca version: {orca.version}")

        start = time.time()

        pcm, alignments = orca.synthesize(text)

        processing_time = time.time() - start
        length_sec = len(pcm) / orca.sample_rate

        with wave.open(output_path, "wb") as output_file:
            output_file.setnchannels(1)
            output_file.setsampwidth(2)
            output_file.setframerate(orca.sample_rate)
            output_file.writeframes(struct.pack(f"{len(pcm)}h", *pcm))

        print(
            f"Orca took {processing_time:.2f} seconds to synthesize {length_sec:.2f} seconds of speech which is "
            f"~{length_sec / processing_time:.0f} times faster than real-time.")
        print(f"Audio written to `{output_path}`.")
    except OrcaActivationLimitError:
        print("AccessKey has reached its processing limit")
    finally:
        orca.delete()


if __name__ == "__main__":
    main()
