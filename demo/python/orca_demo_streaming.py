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
import platform
import re
import subprocess
import time
from collections import deque
from itertools import chain
from typing import (
    Sequence,
)

import pvorca
import tiktoken
from pvorca import OrcaActivationLimitError
from pvspeaker import PvSpeaker

CUSTOM_PRON_PATTERN = r"\{(.*?\|.*?)\}"
CUSTOM_PRON_PATTERN_NO_WHITESPACE = r"\{(.*?\|.*?)\}(?!\s)"


def linux_machine() -> str:
    machine = platform.machine()
    if machine == "x86_64":
        return machine
    elif machine in ["aarch64", "armv7l"]:
        arch_info = ("-" + machine) if "64bit" in platform.architecture()[0] else ""
    else:
        raise NotImplementedError("Unsupported CPU architecture: `%s`" % machine)

    cpu_info = ""
    try:
        cpu_info = subprocess.check_output(["cat", "/proc/cpuinfo"]).decode("utf-8")
        cpu_part_list = [x for x in cpu_info.split("\n") if "CPU part" in x]
        cpu_part = cpu_part_list[0].split(" ")[-1].lower()
    except Exception as e:
        raise RuntimeError("Failed to identify the CPU with `%s`\nCPU info: `%s`" % (e, cpu_info))

    if "0xd03" == cpu_part:
        return "cortex-a53" + arch_info
    elif "0xd07" == cpu_part:
        return "cortex-a57" + arch_info
    elif "0xd08" == cpu_part:
        return "cortex-a72" + arch_info
    elif "0xd0b" == cpu_part:
        return "cortex-a76" + arch_info
    else:
        raise NotImplementedError("Unsupported CPU: `%s`." % cpu_part)


def get_first_audio_wait_chunks() -> int:
    wait_chunks = 0
    if platform.system() == "Linux":
        machine = linux_machine()
        if "cortex" in machine:
            wait_chunks = 1
    return wait_chunks


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


def main() -> None:
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
        "--text_to_stream",
        "-t",
        required=True,
        help="Text to be streamed to Orca")
    parser.add_argument(
        "--tokens_per_second",
        type=int,
        default=15,
        help="Number of tokens per second to be streamed to Orca, simulating an LLM response.")
    parser.add_argument(
        "--audio_wait_chunks",
        type=int,
        default=0,
        help="Number of PCM chunks to wait before starting to play audio. Default: system-dependent.")
    parser.add_argument(
        "--buffer_size_secs",
        type=int,
        default=20,
        help="The size in seconds of the internal buffer used by pvspeaker to play audio.")
    parser.add_argument(
        "--show_audio_devices",
        action="store_true",
        help="Only list available audio output devices and exit")
    parser.add_argument(
        '--audio-device-index',
        type=int,
        default=-1,
        help='Index of input audio device')
    args = parser.parse_args()

    if args.show_audio_devices:
        devices = PvSpeaker.get_available_devices()
        for i in range(len(devices)):
            print("index: %d, device name: %s" % (i, devices[i]))
        exit(0)

    access_key = args.access_key
    model_path = args.model_path
    library_path = args.library_path
    text = args.text_to_stream
    tokens_per_second = args.tokens_per_second
    audio_wait_chunks = max(args.audio_wait_chunks, get_first_audio_wait_chunks())
    buffer_size_secs = args.buffer_size_secs
    audio_device_index = args.audio_device_index

    orca = pvorca.create(access_key=access_key, model_path=model_path, library_path=library_path)
    stream = orca.stream_open()

    speaker = None
    pcm_buf = deque()
    try:
        speaker = PvSpeaker(sample_rate=orca.sample_rate, bits_per_sample=16, buffer_size_secs=buffer_size_secs,
                            device_index=audio_device_index)
        speaker.start()
    except ValueError:
        print(
            "\nWarning: Failed to initialize PvSpeaker. Orca will still generate PCM data, but it will not be played.\n")

    try:
        print(f"Orca version: {orca.version}\n")

        print(f"Simulated text stream:")
        tokens = tokenize_text(text=text)

        is_start_playing = False
        time_first_audio_available = None
        time_start_text_stream = time.time()

        for token in tokens:
            print(f"{token}", end="", flush=True)
            pcm = stream.synthesize(text=token)

            if pcm is not None:
                if time_first_audio_available is None:
                    time_first_audio_available = time.time()
                if speaker is not None:
                    pcm_buf.append(pcm)
                    if len(pcm_buf) > audio_wait_chunks:
                        is_start_playing = True

            if is_start_playing and len(pcm_buf) > 0:
                pcm = pcm_buf.popleft()
                written = speaker.write(pcm)
                if written < len(pcm):
                    pcm_buf.appendleft(pcm[written:])

            time.sleep(1 / tokens_per_second)

        text_stream_duration_seconds = time.time() - time_start_text_stream

        remaining_pcm = stream.flush()
        if time_first_audio_available is None:
            time_first_audio_available = time.time()
        pcm_buf.append(remaining_pcm)

        first_audio_available_seconds = time_first_audio_available - time_start_text_stream
        print(f"\n\nTime to finish text stream:  {text_stream_duration_seconds:.2f} seconds")
        print(f"Time to receive first audio: {first_audio_available_seconds:.2f} seconds after text stream started")

        if speaker is not None:
            print("\nWaiting for audio to finish ...")
            speaker.flush(list(chain.from_iterable(pcm_buf)))

    except KeyboardInterrupt:
        speaker.stop()
        print("\nStopped...")
    except OrcaActivationLimitError:
        print("\nAccessKey has reached its processing limit")
    finally:
        orca.delete()


if __name__ == "__main__":
    main()
