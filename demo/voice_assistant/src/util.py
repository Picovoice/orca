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

import time
from dataclasses import dataclass
from typing import Tuple


@dataclass
class Colors:
    GREEN = "\033[92m"
    RESET = "\033[0m"
    BOLD = "\033[1m"


@dataclass
class Timer:
    time_llm_request: float = -1.0
    time_first_llm_token: float = -1.0
    time_last_llm_token: float = -1.0
    time_first_synthesis_request: float = -1.0
    time_first_audio: float = -1.0
    initial_audio_delay: float = 0.0

    before_first_audio: bool = True
    _is_first_synthesis_request: bool = True
    _num_tokens: int = 0

    @staticmethod
    def _get_time() -> float:
        return time.time()

    def log_time_llm_request(self) -> None:
        self.time_llm_request = self._get_time()

    def log_time_first_llm_token(self) -> None:
        self.time_first_llm_token = self._get_time()

    def log_time_last_llm_token(self) -> None:
        self.time_last_llm_token = self._get_time()

    def maybe_log_time_first_synthesis_request(self) -> None:
        if self._is_first_synthesis_request:
            self.time_first_synthesis_request = self._get_time()
            self._is_first_synthesis_request = False

    def maybe_log_time_first_audio(self) -> None:
        if self.before_first_audio:
            self.time_first_audio = self._get_time()
            self.before_first_audio = False

    def increment_num_tokens(self) -> None:
        self._num_tokens += 1

    @property
    def is_first_token(self) -> bool:
        return self._num_tokens == 0

    def set_initial_audio_delay(self, delay: float) -> None:
        self.initial_audio_delay = delay

    def num_seconds_to_first_audio(self) -> float:
        return self.time_first_audio - self.time_first_llm_token

    def num_seconds_to_first_token(self) -> float:
        return self.time_first_llm_token - self.time_llm_request

    def reset(self) -> None:
        self.time_llm_request = -1.0
        self.time_first_llm_token = -1.0
        self.time_last_llm_token = -1.0
        self.time_first_synthesis_request = -1.0
        self.time_first_audio = -1.0
        self.initial_audio_delay = 0.0

        self._is_first_synthesis_request = True
        self.before_first_audio = True

        self._num_tokens = 0


class TimingPrinter:
    TIMER_MESSAGE = "Time to wait for"

    TIMER_BAR_MAX_RED_SECONDS = 2.0
    TIMER_BAR_SYMBOLS_PER_SECONDS = 40
    TIMER_BAR_SYMBOL = ">"

    MAX_GREEN_VALUE = 0.6
    MAX_RED_VALUE = 0.75

    def __init__(
            self,
            llm_string: str,
            synthesizer_string: str,
            timer_bar_max_red_seconds: float = TIMER_BAR_MAX_RED_SECONDS,
            timer_bar_symbols_per_second: float = TIMER_BAR_SYMBOLS_PER_SECONDS,
            timer_bar_symbol: str = TIMER_BAR_SYMBOL,
    ) -> None:
        max_length = len(llm_string) if len(llm_string) > len(synthesizer_string) else len(synthesizer_string)
        llm_info_string = llm_string.ljust(max_length)
        synthesizer_info_string = synthesizer_string.ljust(max_length)

        self._timer_message_llm = f"{self.TIMER_MESSAGE} {llm_info_string} : "
        self._timer_message_tts = f"{self.TIMER_MESSAGE} {synthesizer_info_string} : "

        self._progress_bar_color_max = timer_bar_max_red_seconds * timer_bar_symbols_per_second
        self._progress_bar_symbols_per_second = timer_bar_symbols_per_second
        self._progress_bar_symbol = timer_bar_symbol

    @staticmethod
    def _colored_string(text: str, red: float, green: float, blue: float, bold: bool = False) -> str:
        s = Colors.BOLD if bold else ""
        s = f"{s}\033[38;2;{int(red * 255)};{int(green * 255)};{int(blue * 255)}m{text}{Colors.RESET}"
        return s

    def _print_colored_progress_bar(self, num_seconds: float, bold: bool = False) -> Tuple[float, float, float]:
        red = 0
        green = self.MAX_GREEN_VALUE
        blue = 0

        half_max_length = self._progress_bar_color_max // 2

        length = int(num_seconds * self._progress_bar_symbols_per_second)
        for i in range(length):
            if i < half_max_length:
                red = min(i / (half_max_length - 1), self.MAX_RED_VALUE)
            else:
                green = max(0.5 - (i - half_max_length) / (half_max_length - 1), 0)

            print(f"{self._colored_string(self._progress_bar_symbol, red, green, blue, bold=bold)}", end="")

        return red, green, blue

    def _print_timer_bar_llm(self, num_seconds_first_llm_token: float) -> None:
        print(self._colored_string(self._timer_message_llm, 0, self.MAX_GREEN_VALUE, 0), end="")

        red, green, blue = self._print_colored_progress_bar(num_seconds_first_llm_token)

        num_seconds_string = f"{round(num_seconds_first_llm_token, 1):.1f}s"
        print(f" {self._colored_string(num_seconds_string, red, green, blue)}", flush=True)

    def _print_timer_bar_tts(self, num_seconds_first_audio: float) -> None:
        print(self._colored_string(self._timer_message_tts, 0, self.MAX_GREEN_VALUE, 0, bold=True), end="")

        red, green, blue = self._print_colored_progress_bar(num_seconds_first_audio, bold=True)

        num_seconds_string = f"{round(num_seconds_first_audio, 1):.1f}s"
        print(f" {self._colored_string(num_seconds_string, red, green, blue, bold=True)}", flush=True)

    def print_timing_stats(self, num_seconds_first_llm_token: float, num_seconds_first_audio: float) -> None:
        print()
        self._print_timer_bar_llm(num_seconds_first_llm_token)
        self._print_timer_bar_tts(num_seconds_first_audio)


__all__ = [
    "Colors",
    "TimingPrinter",
    "Timer",
]
