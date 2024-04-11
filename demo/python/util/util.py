import time
from dataclasses import dataclass
from datetime import datetime


@dataclass
class Colors:
    GREEN = "\033[92m"
    GREY = "\033[90m"
    RESET = "\033[0m"


@dataclass
class Timestamps:
    time_llm_request: float = -1.0
    time_first_llm_token: float = -1.0
    time_last_llm_token: float = -1.0
    time_first_synthesis_request: float = -1.0
    time_first_audio: float = -1.0
    initial_audio_delay: float = 0.0

    is_first_audio: bool = True
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
        if self.is_first_audio:
            self.time_first_audio = self._get_time()
            self.is_first_audio = False

    def increment_num_tokens(self) -> None:
        self._num_tokens += 1
        if self._num_tokens == 1:
            self.log_time_first_llm_token()

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
        self.is_first_audio = True

        self._num_tokens = 0

    def pretty_print_diffs(self) -> None:
        def to_ms(t: float) -> float:
            return round(t * 1_000, -1)

        print(Colors.GREEN)

        print(
            f"Tokens / second: ~{self._num_tokens / (self.time_last_llm_token - self.time_llm_request):.0f}",
            end="")
        print(f" (delay until first token: {to_ms(self.time_first_llm_token - self.time_llm_request):.0f}ms)")

        print(f"Time to generate text: {to_ms(self.time_last_llm_token - self.time_first_llm_token):.0f}ms")
        print(f"Time to first audio: {to_ms(self.time_first_audio - self.time_first_llm_token):.0f}ms", end="")
        if self.initial_audio_delay > 0:
            print(
                f" (added delay of `{to_ms(self.initial_audio_delay):.0f}ms` "
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


__all__ = [
    "Colors",
    "Timestamps",
]
