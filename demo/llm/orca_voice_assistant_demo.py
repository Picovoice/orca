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

from src import *

MAX_WAIT_TIME_FIRST_AUDIO = 10

DEFAULT_USER_PROMPT = "Your question: "


def get_user_input_init_kwargs(args: argparse.Namespace) -> Dict[str, str]:
    kwargs = dict()
    user_input_type = UserInputs(args.user_input)

    if user_input_type is UserInputs.TEXT:
        kwargs["prompt"] = DEFAULT_USER_PROMPT

    elif user_input_type is UserInputs.VOICE:
        kwargs["audio_device_index"] = args.audio_device_index
        kwargs["transcriber"] = args.transcriber

        kwargs["transcriber_params"] = dict()
        transcriber_type = Transcribers(args.transcriber)
        if transcriber_type is Transcribers.PICOVOICE_CHEETAH:
            if not args.picovoice_access_key:
                raise ValueError("Picovoice access key is required when using voice user input")
            kwargs["transcriber_params"]["access_key"] = args.picovoice_access_key
            kwargs["transcriber_params"]["endpoint_duration_sec"] = args.endpoint_duration_sec

    return kwargs


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


def get_synthesizer_init_kwargs(args: argparse.Namespace) -> Dict[str, str]:
    kwargs = dict()
    synthesizer_type = Synthesizers(args.synthesizer)

    if synthesizer_type is Synthesizers.PICOVOICE_ORCA:
        if not args.picovoice_access_key:
            raise ValueError("Picovoice access key is required when using Picovoice TTS")
        kwargs["access_key"] = args.picovoice_access_key
        kwargs["model_path"] = args.orca_model_path
        kwargs["library_path"] = args.orca_library_path

    elif synthesizer_type is Synthesizers.OPENAI:
        if not args.openai_access_key:
            raise ValueError(
                f"An OpenAI access key is required when using OpenAI models. Specify with `--openai-access-key`.")
        kwargs["access_key"] = args.openai_access_key

    return kwargs


def print_welcome_message() -> None:
    print("Orca instant audio generation demo!\n")


def main(args: argparse.Namespace) -> None:
    llm_type = LLMs(args.llm)

    timer = Timer()

    user_input_init_kwargs = get_user_input_init_kwargs(args)
    user_input = UserInput.create(UserInputs(args.user_input), **user_input_init_kwargs)

    audio_output = StreamingAudioDevice.from_default_device()

    synthesizer_init_kwargs = get_synthesizer_init_kwargs(args)
    synthesizer = Synthesizer.create(
        Synthesizers(args.synthesizer),
        play_audio_callback=audio_output.play,
        timer=timer,
        **synthesizer_init_kwargs)

    llm_init_kwargs = get_llm_init_kwargs(args)
    llm = LLM.create(llm_type, **llm_init_kwargs)

    progress_printer = ProgressPrinter(
        show_llm_response=False,
        llm_response_init_message="LLM response: ",
        timer_llm_init_message="Wait for LLM: ",
        timer_tts_init_message="Wait for TTS: ",
    )

    print_welcome_message()

    try:
        while True:
            timer.reset()

            audio_output.start(sample_rate=synthesizer.sample_rate)

            text = user_input.get_user_prompt()

            progress_printer.start(f"Using {synthesizer}")

            timer.log_time_llm_request()

            progress_printer.update_timer_llm(ProgressPrinter.TimerEvent(start=True))

            generator = llm.chat(user_input=text)

            llm_message = ""
            for token in generator:
                if token is None:
                    continue

                if timer.is_first_token:
                    timer.log_time_first_llm_token()
                    progress_printer.update_timer_llm(
                        ProgressPrinter.TimerEvent(num_milliseconds=timer.get_time_to_first_token()))
                    progress_printer.update_timer_tts(ProgressPrinter.TimerEvent(start=True))

                progress_printer.update_llm_response(token)

                llm_message += token

                if synthesizer.input_streamable:
                    synthesizer.synthesize(token)

                if not timer.before_first_audio:
                    progress_printer.update_timer_tts(
                        ProgressPrinter.TimerEvent(num_milliseconds=timer.get_time_to_first_audio()))

                timer.increment_num_tokens()

            timer.log_time_last_llm_token()

            if synthesizer.input_streamable:
                synthesizer.flush()
            else:
                synthesizer.synthesize(llm_message)

            wait_start_time = time.time()
            while timer.before_first_audio:
                if time.time() - wait_start_time > MAX_WAIT_TIME_FIRST_AUDIO:
                    print(f"Waited for {MAX_WAIT_TIME_FIRST_AUDIO}s for first audio but did not receive any. Exiting")
                    break

            progress_printer.update_timer_tts(
                ProgressPrinter.TimerEvent(num_milliseconds=timer.get_time_to_first_audio()))

            progress_printer.stop()
            audio_output.wait_and_terminate()

    except KeyboardInterrupt:
        pass

    synthesizer.terminate()
    audio_output.wait_and_terminate()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Text-to-speech streaming synthesis")

    parser.add_argument(
        "--user-input",
        default=UserInputs.TEXT.value,
        choices=[u.value for u in UserInputs],
        help="Choose user input type")
    parser.add_argument(
        "--transcriber",
        default=Transcribers.PICOVOICE_CHEETAH.value,
        choices=[t.value for t in Transcribers],
        help="Choose transcriber to use if `--user-input` is set to voice")
    parser.add_argument(
        "--audio-device-index",
        type=int,
        default=-1,
        help="Index of input audio device")
    parser.add_argument(
        "--endpoint-duration-sec",
        type=float,
        default=1.,
        help="Duration in seconds for speechless audio to be considered an endpoint")
    parser.add_argument(
        "--show-audio-devices",
        action="store_true",
        help="Only list available devices and exit")

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
        default=25,
        type=int,
        help="Imitated tokens per second")

    parser.add_argument(
        "--synthesizer",
        default=Synthesizers.PICOVOICE_ORCA.value,
        choices=[s.value for s in Synthesizers],
        help="Choose voice synthesizer to use")
    parser.add_argument("--picovoice-access-key", "-a", help="AccessKey obtained from Picovoice Console")
    parser.add_argument("--orca-model-path", "-m", help="Path to the model parameters file")
    parser.add_argument("--orca-library-path", "-l", help="Path to Orca's dynamic library")

    arg = parser.parse_args()

    if arg.show_audio_devices:
        for index, name in enumerate(PvRecorder.get_available_devices()):
            print('Device #%d: %s' % (index, name))
        exit(0)

    main(arg)