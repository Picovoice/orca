#
#    Copyright 2024 Picovoice Inc.
#
#    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
#    file accompanying this source.
#
#    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
#    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
#    specific language governing permissions and limitations under the License.
#

import argparse
import re
import time
import traceback
from typing import Sequence

import tiktoken
from pvorca import OrcaActivationLimitError
from sounddevice import PortAudioError

from _audio_device import StreamingAudioDevice
from _orca_thread import OrcaThread

CUSTOM_PRON_PATTERN = r"\{(.*?\|.*?)\}"
CUSTOM_PRON_PATTERN_NO_WHITESPACE = r"\{(.*?\|.*?)\}(?!\s)"


def tokenize_text(text: str) -> Sequence[str]:
    text = re.sub(CUSTOM_PRON_PATTERN_NO_WHITESPACE, r'{\1} ', text)

    custom_pronunciations = re.findall(CUSTOM_PRON_PATTERN, text)
    custom_pronunciations = set(["{" + pron + "}" for pron in custom_pronunciations])

    encoder = tiktoken.encoding_for_model("gpt-4")
    tokens_raw = [encoder.decode([i]) for i in encoder.encode(text)]

    custom_pron = ""
    tokens_with_custom_pronunciations = []
    for i, token in enumerate(tokens_raw):
        in_custom_pron = False
        for pron in custom_pronunciations:
            in_custom_pron_global = len(custom_pron) > 0
            current_match = token.strip() if not in_custom_pron_global else custom_pron + token
            if pron.startswith(current_match):
                custom_pron += token.strip() if not in_custom_pron_global else token
                in_custom_pron = True

        if not in_custom_pron:
            if custom_pron != "":
                tokens_with_custom_pronunciations.append(f" {custom_pron}" if i != 0 else custom_pron)
                custom_pron = ""
            tokens_with_custom_pronunciations.append(token)

    return tokens_with_custom_pronunciations


def main(args: argparse.Namespace) -> None:
    access_key = args.access_key
    model_path = args.model_path
    library_path = args.library_path
    text = args.text_to_stream
    tokens_per_second = args.tokens_per_second
    audio_wait_chunks = args.audio_wait_chunks

    audio_device = None
    try:
        audio_device = StreamingAudioDevice.from_default_device()
        play_audio_callback = audio_device.play
    except PortAudioError as e:
        print(traceback.format_exc())
        print(
            "WARNING: Failed to initialize audio device, see details above. Falling back to running "
            "the demo without audio\n")

        def play_audio_callback(pcm: Sequence[int]):
            pass

    orca = OrcaThread(
        play_audio_callback=play_audio_callback,
        num_tokens_per_second=tokens_per_second,
        access_key=access_key,
        model_path=model_path,
        library_path=library_path,
        audio_wait_chunks=audio_wait_chunks,
    )

    if audio_device is not None:
        audio_device.start(sample_rate=orca.sample_rate)
    orca.start()

    try:
        print(f"Orca version: {orca.version}\n")

        print(f"Simulated text stream:")
        tokens = tokenize_text(text=text)

        time_start_text_stream = time.time()
        for token in tokens:
            print(f"{token}", end="", flush=True)

            orca.synthesize(text=token)

            time.sleep(1 / tokens_per_second)

        text_stream_duration_seconds = time.time() - time_start_text_stream

        orca.flush()

        first_audio_available_seconds = orca.get_time_first_audio_available() - time_start_text_stream
        print(f"\n\nTime to finish text stream:  {text_stream_duration_seconds:.2f} seconds")
        print(f"Time to receive first audio: {first_audio_available_seconds:.2f} seconds after text stream started\n")

        if audio_device is not None:
            print("Waiting for audio to finish ...")
            audio_device.flush_and_terminate()

    except OrcaActivationLimitError:
        print("AccessKey has reached its processing limit")
    finally:
        orca.delete()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--access_key",
        "-a",
        required=True,
        help="AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)")
    parser.add_argument(
        "--library_path",
        "-l",
        help="Absolute path to dynamic library. Default: using the library provided by `pvorca`")
    parser.add_argument(
        "--model_path",
        "-m",
        help="Absolute path to Orca model. Default: using the model provided by `pvorca`")
    parser.add_argument(
        "--text-to-stream",
        "-t",
        required=True,
        help="Text to be streamed to Orca")
    parser.add_argument(
        "--tokens-per-second",
        type=int,
        default=15,
        help="Number of tokens to be streamed per second to Orca, simulating an LLM response.")
    parser.add_argument(
        "--audio-wait-chunks",
        type=int,
        default=None,
        help="Number of PCM chunks to wait before starting to play audio. Default: system-dependent.")

    main(parser.parse_args())
