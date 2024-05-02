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
from typing import Dict

from pvrecorder import PvRecorder

from src import (
    LLM,
    LLMs,
    Synthesizer,
    Synthesizers,
    TimingPrinter,
    Timer,
    UserInput,
    UserInputs,
    StreamingAudioDevice,
    Transcribers,
)

MAX_WAIT_TIME_FIRST_AUDIO = 10


def get_user_input_init_kwargs(args: argparse.Namespace) -> Dict[str, str]:
    kwargs = dict()

    user_input_type = UserInputs(args.user_input)
    if user_input_type is UserInputs.VOICE:
        kwargs["audio_device_index"] = args.input_audio_device_index

        kwargs["transcriber"] = Transcribers.PICOVOICE_CHEETAH
        kwargs["transcriber_params"] = dict()
        if args.picovoice_access_key is None:
            raise ValueError("Picovoice access key is required when using voice user input")
        kwargs["transcriber_params"]["access_key"] = args.picovoice_access_key
        if args.speech_endpoint_duration_sec is not None:
            kwargs["transcriber_params"]["endpoint_duration_sec"] = args.speech_endpoint_duration_sec

    elif user_input_type is UserInputs.TEXT:
        kwargs["llm_type"] = LLMs(args.llm)

    return kwargs


def get_llm_init_kwargs(args: argparse.Namespace) -> Dict[str, str]:
    kwargs = dict()
    llm_type = LLMs(args.llm)

    if llm_type is LLMs.OPENAI:
        if args.openai_access_key is None:
            raise ValueError(
                f"An OpenAI access key is required when using OpenAI models. Specify with `--openai-access-key`.")
        if args.tokens_per_second is not None:
            raise ValueError(f"Tokens per second is not supported for `{llm_type}`")

        kwargs["access_key"] = args.openai_access_key
        if args.system_message is not None:
            kwargs["system_message"] = args.system_message

    elif llm_type is LLMs.DUMMY:
        if args.tokens_per_second is not None:
            kwargs["tokens_per_second"] = args.tokens_per_second

    return kwargs


def get_synthesizer_init_kwargs(args: argparse.Namespace) -> Dict[str, str]:
    kwargs = dict()
    synthesizer_type = Synthesizers(args.synthesizer)

    if synthesizer_type is Synthesizers.PICOVOICE_ORCA:
        if args.picovoice_access_key is None:
            raise ValueError("Picovoice access key is required when using Picovoice TTS")
        kwargs["access_key"] = args.picovoice_access_key
        kwargs["model_path"] = args.orca_model_path
        kwargs["library_path"] = args.orca_library_path

    elif synthesizer_type is Synthesizers.OPENAI:
        if args.openai_access_key is None:
            raise ValueError(
                f"An OpenAI access key is required when using OpenAI models. Specify with `--openai-access-key`.")
        kwargs["access_key"] = args.openai_access_key

    return kwargs


def main(args: argparse.Namespace) -> None:
    max_num_interactions = args.num_interactions

    user_input_init_kwargs = get_user_input_init_kwargs(args)
    user_input = UserInput.create(UserInputs(args.user_input), **user_input_init_kwargs)

    audio_output = StreamingAudioDevice.from_default_device()

    timer = Timer()

    synthesizer_init_kwargs = get_synthesizer_init_kwargs(args)
    synthesizer = Synthesizer.create(
        Synthesizers(args.synthesizer),
        play_audio_callback=audio_output.play,
        timer=timer,
        **synthesizer_init_kwargs)

    llm_init_kwargs = get_llm_init_kwargs(args)
    llm = LLM.create(LLMs(args.llm), **llm_init_kwargs)

    timing_printer = TimingPrinter(llm_string=f"{llm}", synthesizer_string=f"{synthesizer}")

    try:
        num_interactions_counter = 0
        while True:
            timer.reset()

            audio_output.start(sample_rate=synthesizer.sample_rate)

            text = user_input.get_user_input()

            timer.log_time_llm_request()
            text_generator = llm.chat(user_input=text)

            llm_message = ""
            printed_stats = False
            for token in text_generator:
                if token is None:
                    continue

                if timer.is_first_token:
                    timer.log_time_first_llm_token()

                llm_message += token

                if synthesizer.text_streamable:
                    synthesizer.synthesize(token)

                if not timer.before_first_audio and not printed_stats:
                    timing_printer.print_timing_stats(
                        num_seconds_first_llm_token=timer.num_seconds_to_first_token(),
                        num_seconds_first_audio=timer.num_seconds_to_first_audio(),
                    )
                    printed_stats = True
                    print(f"Answering with {synthesizer} ...")

                timer.increment_num_tokens()

            timer.log_time_last_llm_token()

            if synthesizer.text_streamable:
                synthesizer.flush()
            else:
                synthesizer.synthesize(llm_message)

            wait_start_time = time.time()
            while timer.before_first_audio:
                if time.time() - wait_start_time > MAX_WAIT_TIME_FIRST_AUDIO:
                    print(
                        f"Waited for {MAX_WAIT_TIME_FIRST_AUDIO}s for first audio but did not receive any. Exiting")
                    break

            if not printed_stats:
                timing_printer.print_timing_stats(
                    num_seconds_first_llm_token=timer.num_seconds_to_first_token(),
                    num_seconds_first_audio=timer.num_seconds_to_first_audio())
                print(f"Answering with {synthesizer} ...")

            audio_output.flush_and_terminate()

            num_interactions_counter += 1

            if 0 < max_num_interactions == num_interactions_counter:
                print("\nDemo complete!")
                break

            print()

    except KeyboardInterrupt:
        pass

    synthesizer.terminate()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Text-to-speech streaming synthesis")

    parser.add_argument(
        "--user-input",
        default=UserInputs.VOICE.value,
        choices=[u.value for u in UserInputs],
        help="Choose type of input type")
    parser.add_argument(
        "--input-audio-device-index",
        type=int,
        default=-1,
        help="Index of input audio device")
    parser.add_argument(
        "--speech-endpoint-duration-sec",
        type=float,
        default=None,
        help="Duration in seconds for speechless audio to be considered an endpoint")
    parser.add_argument(
        "--show-audio-devices",
        action="store_true",
        help="Only list available devices and exit")

    parser.add_argument(
        "--llm",
        default=LLMs.DUMMY.value,
        choices=[llm.value for llm in LLMs],
        help="Choose LLM to use")
    parser.add_argument(
        "--openai-access-key",
        default=None,
        help="Open AI access key. Needed when using openai models")
    parser.add_argument(
        "--system-message",
        default=None,
        help="The system message to use to prompt the LLM response")
    parser.add_argument(
        "--tokens-per-second",
        default=None,
        type=int,
        help="Imitated tokens per second to use for Dummy LLM")

    parser.add_argument(
        "--tts",
        dest="synthesizer",
        default=Synthesizers.PICOVOICE_ORCA.value,
        choices=[s.value for s in Synthesizers],
        help="Choose voice synthesizer to use")
    parser.add_argument(
        "--picovoice-access-key",
        default=None,
        help="AccessKey obtained from Picovoice Console")
    parser.add_argument(
        "--orca-model-path",
        default=None,
        help="Path to the model parameters file")
    parser.add_argument(
        "--orca-library-path",
        default=None,
        help="Path to Orca's dynamic library")

    parser.add_argument(
        "--num-interactions",
        type=int,
        default=-1,
        help="Number of interactions with LLM run before completing the demo. Default is -1 (run indefinitely)")

    arg = parser.parse_args()

    if arg.show_audio_devices:
        for index, name in enumerate(PvRecorder.get_available_devices()):
            print('Device #%d: %s' % (index, name))
        exit(0)

    main(arg)
