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
from typing import Dict, Sequence

from src import *

MAX_WAIT_TIME_FIRST_AUDIO = 5


def get_llm_init_kwargs(args: argparse.Namespace) -> Dict[str, str]:
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


def get_synthesizer_init_kwargs(args: argparse.Namespace) -> Sequence[Dict[str, str]]:
    kwargs = []

    for synthesizer_type_string in args.synthesizers:
        synthesizer_type = Synthesizers(synthesizer_type_string)

        kwargs.append(dict())
        kwargs[-1]["synthesizer_type"] = synthesizer_type

        if synthesizer_type is Synthesizers.PICOVOICE_ORCA:
            if not args.picovoice_access_key:
                raise ValueError("Picovoice access key is required when using Picovoice TTS")
            kwargs[-1]["access_key"] = args.picovoice_access_key
            kwargs[-1]["model_path"] = args.picovoice_model_path
            kwargs[-1]["library_path"] = args.picovoice_library_path

        elif synthesizer_type is Synthesizers.OPENAI:
            if not args.openai_access_key:
                raise ValueError(
                    f"An OpenAI access key is required when using OpenAI models. Specify with `--openai-access-key`.")
            kwargs[-1]["access_key"] = args.openai_access_key

    return kwargs


def main(args: argparse.Namespace) -> None:
    llm_type = LLMs(args.llm)

    timer = Timer()

    audio_output = StreamingAudioDevice.from_default_device()

    synthesizers = []
    synthesizer_init_kwargs = get_synthesizer_init_kwargs(args)
    for kwargs in synthesizer_init_kwargs:
        synthesizers.append(Synthesizer.create(
            kwargs.pop("synthesizer_type"),
            play_audio_callback=audio_output.play,
            timer=timer,
            **kwargs))

    llm_init_kwargs = get_llm_init_kwargs(args)
    llm = LLM.create(llm_type, **llm_init_kwargs)

    print("PICOVOICE ORCA STREAMING TTS DEMO")
    print(
        "This demo let's you chat with an LLM. The response is read out loud by a TTS system. Press Ctrl+C to exit.\n")

    progress_printer = ProgressPrinter(
        llm_response_init_message="LLM response: ",
        timer_init_message="Time to first audio: ",
    )

    try:
        while True:
            previous_prompt = None
            for synthesizer in synthesizers:
                timer.reset()

                audio_output.start(sample_rate=synthesizer.sample_rate)
                # dashboard_printer.setup(synthesizer_name=str(synthesizer))

                text = llm.get_user_input(previous_prompt=previous_prompt)
                previous_prompt = text

                progress_printer.start(f"Using {synthesizer}")

                timer.log_time_llm_request()
                generator = llm.chat(user_input=text)

                llm_message = ""
                for token in generator:
                    if token is None:
                        continue

                    progress_printer.update_llm_response(token)
                    if timer.is_first_token:
                        progress_printer.update_timer(start=True)
                    timer.increment_num_tokens()

                    # print(token, flush=True, end="")

                    llm_message += token

                    if synthesizer.input_streamable:
                        synthesizer.synthesize(token)

                    if not timer.is_first_audio:
                        progress_printer.update_timer(num_milliseconds=timer.get_time_to_first_audio())
                        # progress_printer.print_first_audio_message(num=timer.get_time_to_first_audio())
                    # else:
                    #     progress_printer.print_first_audio_message(progress_printer.PROGRESS_BAR_SYMBOL)

                # dashboard_printer.print_assistant_message(" (waiting for audio to finish...)")
                # print(" (waiting for audio to finish...)", flush=True, end="")
                timer.log_time_last_llm_token()

                if synthesizer.input_streamable:
                    synthesizer.flush()
                else:
                    synthesizer.synthesize(llm_message)

                wait_seconds = 0
                while timer.is_first_audio:
                    increment = 0.01
                    time.sleep(increment)
                    wait_seconds += increment
                    if wait_seconds > MAX_WAIT_TIME_FIRST_AUDIO:
                        break

                progress_printer.stop()
                # dashboard_printer.print_first_audio_message(num=timestamps.get_time_to_first_audio())
                # dashboard_printer.reset()

                audio_output.wait_and_terminate()

                # timer.pretty_print_diffs()
                # dashboard_printer.cursor_down()

    except KeyboardInterrupt:
        pass

    for synthesizer in synthesizers:
        synthesizer.terminate()
    audio_output.wait_and_terminate()


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
        "--synthesizers",
        nargs="+",
        default=[Synthesizers.PICOVOICE_ORCA.value, Synthesizers.OPENAI.value],
        choices=[s.value for s in Synthesizers],
        help="Choose voice synthesizer to use")
    parser.add_argument("--picovoice-access-key", "-a", help="AccessKey obtained from Picovoice Console")
    parser.add_argument("--picovoice-model-path", "-m", help="Path to the model parameters file")
    parser.add_argument("--picovoice-library-path", "-l", help="Path to Orca's dynamic library")

    main(parser.parse_args())
