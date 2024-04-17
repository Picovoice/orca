import time
from dataclasses import dataclass
from datetime import datetime


@dataclass
class Colors:
    GREEN = "\033[92m"
    DARK_GREEN = "\u001B[32m"
    GREY = "\033[90m"
    BLUE = "\033[94m"
    RESET = "\033[0m"
    ORANGE = "\033[93m"
    RED = "\033[91m"
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

    @staticmethod
    def _to_rounded_string(t: float) -> str:
        return f"{round(t, 1):.1f}s"

    def num_seconds_to_first_audio(self) -> float:
        return self.time_first_audio - self.time_first_llm_token

    def num_seconds_to_first_token(self) -> float:
        return self.time_first_llm_token - self.time_llm_request

    def pretty_print_diffs(self) -> None:
        print(Colors.DARK_GREEN)

        print(
            f"Delay for first text token: {self._to_rounded_string(self.time_first_llm_token - self.time_llm_request)}")
        print(
            f"Time to generate text: {self._to_rounded_string(self.time_last_llm_token - self.time_first_llm_token)} ",
            end="")
        print(f"(tokens / second = ~{self._num_tokens / (self.time_last_llm_token - self.time_first_llm_token)})")
        print(
            f"Time to first audio after first token: "
            f"{Colors.GREEN}{self._to_rounded_string(self.num_seconds_to_first_audio())}{Colors.DARK_GREEN}",
            end="")
        if self.initial_audio_delay > 0.1:
            print(
                f" (added delay of `{self._to_rounded_string(self.initial_audio_delay)}` "
                "to ensure continuous audio)")

        else:
            print()
        print(Colors.RESET)

    def debug_print(self) -> None:
        def to_hms(t: float) -> str:
            date_object = datetime.fromtimestamp(t)
            return date_object.strftime("%H:%M:%S.%f")[:-3]

        print(f"time LLM request {to_hms(self.time_llm_request)}")
        print(f"time first LLM token {to_hms(self.time_first_llm_token)}")
        print(f"time last LLM token {to_hms(self.time_last_llm_token)}")
        print(f"time first synthesis request {to_hms(self.time_first_synthesis_request)}")
        print(f"time first audio {to_hms(self.time_first_audio)}")


class ProgressPrinter:
    PROGRESS_BAR_MAX_RED_SECONDS = 2.0
    PROGRESS_BAR_SYMBOLS_PER_SECONDS = 50
    PROGRESS_BAR_SYMBOL = "#"

    MAX_GREEN_VALUE = 0.7
    MAX_RED_VALUE = 0.85

    def __init__(
            self,
            timer_tts_message: str = "",
            timer_llm_message: str = "",
            progress_bar_max_red_seconds: float = PROGRESS_BAR_MAX_RED_SECONDS,
            progress_bar_symbols_per_second: float = PROGRESS_BAR_SYMBOLS_PER_SECONDS,
            progress_bar_symbol: str = PROGRESS_BAR_SYMBOL,
    ) -> None:
        self._progress_bar_symbols_per_second = progress_bar_symbols_per_second
        self._progress_bar_color_max = progress_bar_max_red_seconds * progress_bar_symbols_per_second
        self._progress_bar_symbol = progress_bar_symbol

        self._timer_tts_message = timer_tts_message
        self._timer_llm_message = timer_llm_message

    @staticmethod
    def _colored_string(text: str, red: float, green: float, blue: float, bold: bool = False) -> str:
        s = Colors.BOLD if bold else ""
        s = f"{s}\033[38;2;{int(red * 255)};{int(green * 255)};{int(blue * 255)}m{text}\033[0m"
        return s

    def _print_colored_progress_bar(self, num_seconds: float, bold: bool = False) -> tuple[float, float, float]:

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

    def print_timing_stats(self, num_seconds_first_llm_token: float, num_seconds_first_audio: float) -> None:
        print(self._colored_string(self._timer_llm_message, 0, self.MAX_GREEN_VALUE, 0), end="")
        red, green, blue = self._print_colored_progress_bar(num_seconds_first_llm_token)
        num_seconds_string = f"{round(num_seconds_first_llm_token, 1):.1f}s"
        print(f" {self._colored_string(num_seconds_string, red, green, blue)}", flush=True)

        print(self._colored_string(self._timer_tts_message, 0, self.MAX_GREEN_VALUE, 0, bold=True), end="")
        red, green, blue = self._print_colored_progress_bar(num_seconds_first_audio, bold=True)
        num_seconds_string = f"{round(num_seconds_first_audio, 1):.1f}s"
        print(f" {self._colored_string(num_seconds_string, red, green, blue, bold=True)}", flush=True)


__all__ = [
    "Colors",
    "ProgressPrinter",
    "Timer",
]
