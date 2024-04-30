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
import json
import os
import re
import struct
import time
import wave
from dataclasses import dataclass
from typing import Optional, Sequence

import tiktoken
from pvorca import create, OrcaActivationLimitError

CUSTOM_PRON_OPENING_MARKER = "{"
CUSTOM_PRON_CLOSING_MARKER = "}"
CUSTOM_PRON_SEPARATOR = "|"
CUSTOM_PRON_PATTERN = r"\{(.*?\|.*?)\}"
CUSTOM_PRON_PATTERN_NO_WHITESPACE = r"\{(.*?\|.*?)\}(?!\s)"

SIMULATED_TOKENS_PER_SECOND = 10


@dataclass
class TokenMetadata:
    string: str
    pcm: Optional[Sequence[int]] = None


@dataclass
class ChunkMetadata:
    pcm: Sequence[int]
    audio_seconds: float
    proc_seconds: float


def save_wav(output_path: str, pcm: Sequence[int], sample_rate: int) -> None:
    with wave.open(output_path, "wb") as output_file:
        output_file.setnchannels(1)
        output_file.setsampwidth(2)
        output_file.setframerate(sample_rate)
        output_file.writeframes(struct.pack(f"{len(pcm)}h", *pcm))


def save_token_metadata(output_folder: str, token_metadata: Sequence[TokenMetadata], sample_rate: int) -> None:
    output_path = os.path.join(output_folder, "0.wav")
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    json_data = []
    for i, metadata in enumerate(token_metadata):
        pcm_path = None
        if metadata.pcm is not None:
            pcm_path = os.path.abspath(
                os.path.join(
                    os.path.dirname(output_path), 
                    f"{os.path.basename(output_path)[:-4]}{i}.wav"))
            save_wav(
                output_path=pcm_path, 
                pcm=metadata.pcm, 
                sample_rate=sample_rate)
    
        json_data.append({"string": metadata.string, "pcm_path": pcm_path})
        
    with open(os.path.join(os.path.dirname(output_path), "metadata.json"), "w") as json_file:
        json.dump(json_data, json_file)

    print(f"Metadata saved to `{output_path}`.")


def tokenize_text(text: str) -> Sequence[str]:
    text = re.sub(CUSTOM_PRON_PATTERN_NO_WHITESPACE, r'{\1} ', text)

    custom_prons = re.findall(CUSTOM_PRON_PATTERN, text)
    custom_prons = set(["{" + pron + "}" for pron in custom_prons])

    encoder = tiktoken.encoding_for_model("gpt-4")
    tokens_raw = [encoder.decode([i]) for i in encoder.encode(text)]

    custom_pron = ""
    tokens_with_custom_prons = []
    for i, token in enumerate(tokens_raw):
        in_custom_pron = False
        for pron in custom_prons:
            in_custom_pron_global = len(custom_pron) > 0
            current_match = token.strip() if not in_custom_pron_global else custom_pron + token
            if pron.startswith(current_match):
                custom_pron += token.strip() if not in_custom_pron_global else token
                in_custom_pron = True

        if not in_custom_pron:
            if custom_pron != "":
                tokens_with_custom_prons.append(f" {custom_pron}" if i != 0 else custom_pron)
                custom_pron = ""
            tokens_with_custom_prons.append(token)

    return tokens_with_custom_prons


def main(args: argparse.Namespace) -> None:
    access_key = args.access_key
    model_path = args.model_path
    library_path = args.library_path
    output_path = args.output_path
    text = args.text
    no_audio = args.no_audio
    save_metadata_folder = args.save_metadata_folder

    if not output_path.lower().endswith('.wav'):
        raise ValueError('Given argument --output_path must have WAV file extension')

    orca = create(access_key=access_key, model_path=model_path, library_path=library_path)
    orca_stream = orca.open_stream()

    try:
        print(f"Orca version: {orca.version}\n")

        audio_device = None
        if not no_audio:
            from demo_util import StreamingAudioDevice
            audio_device = StreamingAudioDevice.from_default_device()
            audio_device.start(sample_rate=orca.sample_rate)

        pcm_chunks_metadata = []
        token_metadata = []

        first_audio_seconds = None

        print(f"Simulated text stream ({SIMULATED_TOKENS_PER_SECOND} tokens / second):")
        tokens = tokenize_text(text=text)

        initial_tic = time.time()
        for token in tokens:
            print(f"{token}", end="", flush=True)

            tic = time.time()
            pcm_chunk = orca_stream.synthesize(text=token)

            if pcm_chunk is not None:
                toc = time.time()

                if first_audio_seconds is None:
                    first_audio_seconds = time.time() - initial_tic

                if audio_device is not None:
                    audio_device.play(pcm_chunk)

                pcm_chunk_metadata = ChunkMetadata(
                    pcm=pcm_chunk,
                    audio_seconds=len(pcm_chunk) / orca.sample_rate,
                    proc_seconds=toc - tic)
                pcm_chunks_metadata.append(pcm_chunk_metadata)

            token_metadata.append(TokenMetadata(string=token, pcm=pcm_chunk))

            time.sleep(1 / SIMULATED_TOKENS_PER_SECOND)

        print("\n")

        tic = time.time()
        pcm_chunk = orca_stream.flush()
        if first_audio_seconds is None:
            first_audio_seconds = time.time() - initial_tic

        if pcm_chunk is not None:
            toc = time.time()

            if audio_device is not None:
                audio_device.play(pcm_chunk)

            pcm_chunk_metadata = ChunkMetadata(
                pcm=pcm_chunk,
                audio_seconds=len(pcm_chunk) / orca.sample_rate,
                proc_seconds=toc - tic)
            pcm_chunks_metadata.append(pcm_chunk_metadata)

            token_metadata.append(TokenMetadata(string="", pcm=pcm_chunk))

        pcm = [sample for chunk_metadata in pcm_chunks_metadata for sample in chunk_metadata.pcm]

        save_wav(output_path=output_path, pcm=pcm, sample_rate=orca.sample_rate)
        print(f"Audio started playing after {first_audio_seconds:.2f} seconds.\n")
        print("Summary of audio generation:")
        print(
            f"Generated {len(pcm_chunks_metadata)} audio chunk{'' if len(pcm_chunks_metadata) == 1 else 's'} in total.")

        for i, chunk_metadata in enumerate(pcm_chunks_metadata):
            print(
                f"Audio chunk #{i}: length = {chunk_metadata.audio_seconds:.2f}s, "
                f"processing time = {chunk_metadata.proc_seconds:.2f}s.")
        print()
        print(f"Final audio written to `{output_path}`.")

        if save_metadata_folder:
            save_token_metadata(
                output_folder=save_metadata_folder,
                token_metadata=token_metadata,
                sample_rate=orca.sample_rate)

        if audio_device is not None:
            audio_device.wait_and_terminate()

    except OrcaActivationLimitError:
        print("AccessKey has reached its processing limit")
    finally:
        orca.delete()


if __name__ == "__main__":
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
        '--model_path',
        '-m',
        help='Absolute path to Orca model. Default: using the model provided by `pvorca`')
    parser.add_argument(
        '--no-audio',
        action='store_true',
        help='Do not play audio')
    parser.add_argument(
        '--save-metadata-folder',
        default=None,
        help='Path to save metadata for creating animations')

    main(parser.parse_args())
