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
import time

from demo_util import *


def get_llm_init_kwargs(args: argparse.Namespace) -> dict:
    kwargs = dict()
    llm_type = LLMs(args.llm)

    if llm_type is LLMs.OPENAI:
        if not args.openai_access_key:
            raise ValueError(
                f"An OpenAI access key is required when using OpenAI models. Specify with `--openai-access-key`.")
        kwargs["access_key"] = args.openai_access_key

    elif llm_type is LLMs.DUMMY:
        kwargs["tokens_per_second"] = args.tokens_per_second

    return kwargs


def get_synthesizer_init_kwargs(args: argparse.Namespace) -> dict:
    kwargs = dict()
    synthesizer_type = Synthesizers(args.synthesizer)

    if synthesizer_type is Synthesizers.PICOVOICE_ORCA:
        if not args.picovoice_access_key:
            raise ValueError("Picovoice access key is required when using Picovoice TTS")
        kwargs["access_key"] = args.picovoice_access_key
        kwargs["model_path"] = args.picovoice_model_path
        kwargs["library_path"] = args.picovoice_library_path

    elif synthesizer_type is Synthesizers.OPENAI:
        if not args.openai_access_key:
            raise ValueError(
                f"An OpenAI access key is required when using OpenAI models. Specify with `--openai-access-key`.")
        kwargs["access_key"] = args.openai_access_key

    return kwargs


def main(args: argparse.Namespace) -> None:
    llm_type = LLMs(args.llm)
    synthesizer_type = Synthesizers(args.synthesizer)

    timestamps = Timestamps()

    audio_output = StreamingAudioOutput.from_default_device()

    synthesizer_init_kwargs = get_synthesizer_init_kwargs(args)
    synthesizer = Synthesizer.create(
        synthesizer_type,
        play_audio_callback=audio_output.play,
        timestamps=timestamps,
        **synthesizer_init_kwargs)

    audio_output.start(sample_rate=synthesizer.sample_rate)

    synthesize_text_callback = synthesizer.synthesize if synthesizer.input_streamable else None

    llm_init_kwargs = get_llm_init_kwargs(args)
    llm = LLM.create(
        llm_type, 
        synthesize_text_callback=synthesize_text_callback,
        **llm_init_kwargs)

    print("PICOVOICE ORCA STREAMING TTS DEMO")
    print("This demo let's you chat with an LLM. The response is read out loud by a TTS system. Press Ctrl+C to exit.\n")

    try:
        while True:
            text = llm.get_user_input()
            generator = llm.chat(user_input=text)

            llm_message = ""
            num_tokens = 0
            
            while True:
                try:
                    if timestamps.time_llm_request < 0:
                        timestamps.time_llm_request = time.time()
                    token = next(generator)  # TODO: change to standard loop 

                    if timestamps.time_first_llm_token < 0:
                        timestamps.time_first_llm_token = time.time()

                    if token is not None:
                        print(token, end="", flush=True)

                    if token is not None:
                        llm_message += token
                except StopIteration:
                    print(" (waiting for audio to finish...)", flush=True)
                    timestamps.time_last_llm_token = time.time()

                    if synthesizer.input_streamable:
                        synthesizer.flush()
                        synthesizer.wait()
                    else:
                        synthesizer.synthesize(llm_message)

                    audio_output.wait()

                    if synthesizer.input_streamable:
                        synthesizer.reset()

                    break

                num_tokens += 1

            timestamps.pretty_print_diffs(num_tokens=num_tokens)
            timestamps.reset()

    except KeyboardInterrupt:
        pass

    if synthesizer.input_streamable:
        synthesizer.wait_and_terminate()

    audio_output.wait_and_terminate()

    synthesizer.delete()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Text-to-speech streaming synthesis")
    parser.add_argument(
        "--llm",
        default=LLMs.DUMMY.value,
        choices=[l.value for l in LLMs],
        help="Choose LLM to use")
    parser.add_argument(
        "--openai-access-key",
        help="Open AI access key. Needed when using openai models")
    parser.add_argument(
        "--tokens-per-second",
        "-t",
        default=10,
        type=int,
        help="Imitated tokens per second")

    parser.add_argument(
        "--synthesizer",
        default=Synthesizers.PICOVOICE_ORCA.value,
        choices=[s.value for s in Synthesizers],
        help="Choose voice synthesizer to use")
    parser.add_argument("--picovoice-access-key", "-a", help="AccessKey obtained from Picovoice Console")
    parser.add_argument("--picovoice-model-path", "-m", help="Path to the model parameters file")
    parser.add_argument("--picovoice-library-path", "-l", help="Path to Orca's dynamic library")

    main(parser.parse_args())
