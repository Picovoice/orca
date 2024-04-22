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
import struct
import time
import wave
from dataclasses import dataclass
from typing import (
    Generator,
    Sequence,
)

from pvorca import create, OrcaActivationLimitError

CUSTOM_PRON_OPENING_MARKER = "{"
CUSTOM_PRON_CLOSING_MARKER = "}"
CUSTOM_PRON_SEPARATOR = "|"
CUSTOM_PRON_PATTERN = r"\s\{(.*?\|.*?)\}\s"


@dataclass
class ChunkMetadata:
    pcm: Sequence[int]
    audio_seconds: float
    proc_seconds: float


def token_generator(text: str) -> Generator[str, None, None]:
    text = re.sub(CUSTOM_PRON_PATTERN, lambda x: " {" + x.group(1).replace(' ', '_') + "} ", text)

    def is_valid_custom_pron(token: str) -> bool:
        return (token.startswith(CUSTOM_PRON_OPENING_MARKER) and
                token.endswith(CUSTOM_PRON_CLOSING_MARKER) and
                CUSTOM_PRON_SEPARATOR in token)

    for itok, tok in enumerate(text.split()):
        if is_valid_custom_pron(tok):
            tok = tok.replace('_', ' ')
        if itok == len(text.split()) - 1:
            yield tok
        else:
            yield f"{tok} "


def main(args: argparse.Namespace) -> None:
    access_key = args.access_key
    model_path = args.model_path
    library_path = args.library_path
    output_path = args.output_path
    text = args.text
    no_audio = args.no_audio
    save_audio_chunks = args.save_audio_chunks

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
        initial_tic = time.time()

        tic = initial_tic
        for token in token_generator(text=text):
            pcm_chunk = orca_stream.synthesize(text=token)

            if pcm_chunk is not None:
                toc = time.time()

                if audio_device is not None:
                    audio_device.play(pcm_chunk)

                pcm_chunk_metadata = ChunkMetadata(
                    pcm=pcm_chunk,
                    audio_seconds=len(pcm_chunk) / orca.sample_rate,
                    proc_seconds=toc - tic)
                pcm_chunks_metadata.append(pcm_chunk_metadata)

                tic = time.time()

        pcm_chunk = orca_stream.flush()
        if pcm_chunk is not None:
            toc = time.time()

            if audio_device is not None:
                audio_device.play(pcm_chunk)

            pcm_chunk_metadata = ChunkMetadata(
                pcm=pcm_chunk,
                audio_seconds=len(pcm_chunk) / orca.sample_rate,
                proc_seconds=toc - tic)
            pcm_chunks_metadata.append(pcm_chunk_metadata)

        final_toc = time.time()
        processing_time = final_toc - initial_tic

        pcm = [sample for chunk_metadata in pcm_chunks_metadata for sample in chunk_metadata.pcm]

        with wave.open(output_path, "wb") as output_file:
            output_file.setnchannels(1)
            output_file.setsampwidth(2)
            output_file.setframerate(orca.sample_rate)
            output_file.writeframes(struct.pack(f"{len(pcm)}h", *pcm))

        if save_audio_chunks:
            for i, chunk_metadata in enumerate(pcm_chunks_metadata):
                pcm = chunk_metadata.pcm
                with wave.open(output_path.replace('.wav', f'_{i}.wav'), "wb") as output_file:
                    output_file.setnchannels(1)
                    output_file.setsampwidth(2)
                    output_file.setframerate(orca.sample_rate)
                    output_file.writeframes(struct.pack(f"{len(pcm)}h", *pcm))

        print(
            f"Generated {len(pcm_chunks_metadata)} audio chunk{'' if len(pcm_chunks_metadata) == 1 else 's'} "
            f"in {processing_time:.2f} seconds.")
        for i, chunk_metadata in enumerate(pcm_chunks_metadata):
            print(
                f"Audio chunk #{i}: length = {chunk_metadata.audio_seconds:.2f}s, "
                f"processing time = {chunk_metadata.proc_seconds:.2f}s.")
        print()
        print(f"Final audio written to `{output_path}`.")

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
        '--save-audio-chunks',
        action='store_true',
        help=
        'Saves audio chunks as separate WAV files with the same name as the output file but with '
        'an index appended to the name.')

    main(parser.parse_args())
