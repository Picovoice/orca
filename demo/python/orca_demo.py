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

import pvorca


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--access_key',
        '-a',
        help='AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)')
    parser.add_argument(
        '--text',
        '-t',
        help='Text to be synthesized')
    parser.add_argument(
        '--output_path',
        '-o',
        help='Absolute path to .wav file where the generated audio will be stored')
    parser.add_argument(
        "--model_path",
        "-m",
        help="Absolute path to Orca model")
    parser.add_argument(
        '--device',
        help='Device to run inference on (`best`, `cpu:{num_threads}` or `gpu:{gpu_index}`). '
             'Default: automatically selects best device')
    parser.add_argument(
        '--library_path',
        '-l',
        help='Absolute path to dynamic library. Default: using the library provided by `pvorca`')
    parser.add_argument(
        '--show_inference_devices',
        action='store_true',
        help='Print devices that are available to run Orca inference')
    args = parser.parse_args()

    if args.show_inference_devices:
        print('\n'.join(pvorca.available_devices(library_path=args.library_path)))
        return

    access_key = args.access_key
    model_path = args.model_path
    device = args.device
    library_path = args.library_path
    output_path = args.output_path
    text = args.text

    if access_key is None or text is None or output_path is None or model_path is None:
        raise ValueError("Arguments --access_key, --text, --output_path and --model_path are required.")

    if not output_path.lower().endswith('.wav'):
        raise ValueError('Given argument --output_path must have WAV file extension')

    orca = pvorca.create(
        access_key=access_key,
        model_path=model_path,
        device=device,
        library_path=library_path)

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
