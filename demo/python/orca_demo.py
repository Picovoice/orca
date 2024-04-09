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
import struct
import wave

from pvorca import create, OrcaActivationLimitError


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--access_key',
        required=True,
        help='AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)')
    parser.add_argument(
        '--text',
        required=True,
        help='Text to be synthesized')
    parser.add_argument(
        '--output_path',
        required=True,
        help='Absolute path to .wav file where the generated audio will be stored')
    parser.add_argument(
        '--library_path',
        help='Absolute path to dynamic library. Default: using the library provided by `pvorca`')
    parser.add_argument(
        '--model_path',
        help='Absolute path to Orca model. Default: using the model provided by `pvorca`')
    args = parser.parse_args()

    if not args.output_path.lower().endswith('.wav'):
        raise ValueError('Given argument --output_path must have WAV file extension')

    orca = create(access_key=args.access_key, model_path=args.model_path, library_path=args.library_path)

    try:
        print('Orca version: %s' % orca.version)
        pcm, _ = orca.synthesize(args.text)
        length_sec = len(pcm) / orca.sample_rate
        with wave.open(args.output_path, 'wb') as output_file:
            output_file.setnchannels(1)
            output_file.setsampwidth(2)
            output_file.setframerate(orca.sample_rate)
            output_file.writeframes(struct.pack('%dh' % len(pcm), *pcm))
        print('%.2f seconds of audio were written to `%s`.' % (length_sec, args.output_path))
    except OrcaActivationLimitError:
        print('AccessKey has reached its processing limit')
    finally:
        orca.delete()


if __name__ == '__main__':
    main()
