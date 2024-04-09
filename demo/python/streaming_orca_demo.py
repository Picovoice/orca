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
import logging
import os
import random
import sys
import termios  # Unix-specific
import time
import tty
import threading
import queue
from typing import *

import numpy as np
import pyaudio

from pvorca import create, OrcaActivationLimitError

os.environ['PYAUDIO_NO_ALSA_ERROR_MESSAGES'] = '1'

logging.basicConfig(level=logging.INFO)

SENTENCES = [
    "Your flight to Paris is scheduled for tomorrow morning at nine AM. Don't forget to pack your passport, and arrive at the airport at least two hours before departure to allow time for security checks.",
    "Your meeting with the marketing team has been rescheduled to Friday at two PM. Be sure to review the updated agenda and prepare any necessary materials beforehand.",
    "The new restaurant in town, 'Culinary Delights,' has received excellent reviews for its fusion cuisine. Would you like me to make a reservation for dinner this weekend?",
    "Your favorite artist, John Smith, is performing live at the concert hall next Saturday. Tickets are selling out fast, shall I secure seats for you and your friends?",
    "Your fitness tracker data indicates that you've achieved your weekly step goal. Congratulations! How about rewarding yourself with a relaxing massage or a movie night?",
    "Your car's maintenance reminder just popped up. It's time for an oil change and tire rotation. Shall I schedule an appointment with your preferred auto service center?",
    "Your friend's birthday is coming up next week. Would you like assistance in selecting the perfect gift or planning a surprise celebration?",
    "Your subscription to the meditation app expires in three days. Would you like to renew it, or shall I explore other mindfulness resources for you?",
    "Your credit card statement shows a higher-than-usual balance this month. Shall I provide a breakdown of your expenses to help you identify any unnecessary purchases?",
    "Your favorite book club is hosting a discussion on the latest bestseller next Thursday. Would you like me to add it to your calendar, or perhaps you'd prefer to join virtually?"
]

SAMPLE_RATE = 22050

KEYBOARD_INTERRUPT_KEY = '\x03'
ENTER_KEY = '\r'

audio_queue = queue.Queue()

p = pyaudio.PyAudio()
stream = p.open(
    format=pyaudio.paInt16,  # 16-bit pcm
    channels=1,
    rate=SAMPLE_RATE,
    output=True)


def play_audio_worker() -> None:
    while True:
        pcm = audio_queue.get()
        if pcm is None:
            logging.error("Found invalid pcm, exiting audio worker")
            break

        stream.write(pcm.tobytes())


def main(args: argparse.Namespace) -> None:
    access_key = args.access_key
    model_path = args.model_path
    library_path = args.library_path
    simulate_llm_response = args.simulate_llm_response

    orca = create(access_key=access_key, model_path=model_path, library_path=library_path)
    orca_stream = orca.open_stream()

    audio_thread = threading.Thread(target=play_audio_worker, daemon=True)
    audio_thread.start()

    if not simulate_llm_response:
        print("Start typing your text (press Ctrl+C to exit and Enter to flush):")
    else:
        print("Simulating random LLM response...starting in 2 seconds")

    def read_char():
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

    def text_generator() -> Generator[str, None, None]:
        for c in SENTENCES[random.randint(0, len(SENTENCES) - 1)]:
            time.sleep(0.01)
            if c == '\n':
                yield ' '
            yield c

    generator = text_generator()

    text = ""
    is_first = True
    num_tokens = 0
    start = time.time()
    while True:
        if simulate_llm_response:
            try:
                char = next(generator)
            except StopIteration:
                is_last = True
                char = ENTER_KEY
        else:
            char = read_char()

        print(char, end="", flush=True)

        if char == ' ':
            token = f"{text}" if is_first else f" {text}"
            is_first = False
            pcm = orca_stream.synthesize(text=token)
            if len(pcm) > 0:
                audio_queue.put(np.array(pcm, dtype=np.int16))
            text = ""
            num_tokens += 1
        elif char == KEYBOARD_INTERRUPT_KEY:
            break
        elif char == ENTER_KEY:  # Ctrl+C or Enter key
            print()
            token = f"{text}" if is_first else f" {text}"
            pcm = orca_stream.synthesize(text=token)
            if len(pcm) > 0:
                audio_queue.put(np.array(pcm, dtype=np.int16))
            pcm = orca_stream.flush()
            if len(pcm) > 0:
                audio_queue.put(np.array(pcm, dtype=np.int16))

            if simulate_llm_response:
                break
            text = ""
            is_first = True
        else:
            text += char

    orca_stream.close()
    end = time.time()
    logging.info(
        f"Total time: {end - start:.2f}s, simulated number of tokens: {num_tokens}, "
        f"simulated tokens/s: {num_tokens / (end - start):.2f}")

    audio_queue.put(None)
    audio_thread.join()

    p.terminate()

    orca.delete()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Text-to-speech streaming synthesis")
    parser.add_argument("--access_key", "-a", required=True, help="AccessKey obtained from Picovoice Console")
    parser.add_argument("--model_path", "-m", help="Path to the model parameters file")
    parser.add_argument("--library_path", "-l", help="Path to Orca's dynamic library")
    parser.add_argument(
        "--simulate-llm-response",
        action="store_true",
        help="Simulate LLM response (for testing purposes)")

    main(parser.parse_args())
