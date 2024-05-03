# Orca Streaming Text-to-Speech Engine Python Demo

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

## Orca

Orca is an on-device streaming text-to-speech engine that is designed for use with LLMs, enabling zero-latency
voice assistants. Orca is:

- Private; All voice processing runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64)
    - Android and iOS
    - Chrome, Safari, Firefox, and Edge
    - Raspberry Pi (5, 4, 3) and NVIDIA Jetson Nano

## Compatibility

- Python 3.8+
- Runs on Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64), Raspberry Pi (5, 4, 3), and NVIDIA Jetson Nano.

## Installation

```console
pip3 install pvorcademo
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs. You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Usage

Orca supports two modes of operation: streaming and single synthesis.

In the streaming synthesis mode, Orca processes an incoming text stream in real-time and generates audio in parallel.
This is demonstrated in the Orca streaming demo.

In the single synthesis mode, the text is synthesized in a single call to the Orca engine.

### Streaming synthesis demo

In this demo, we simulate a response from a language model by creating a text stream from a user-defined text.
We stream that text to Orca and play the synthesized audio as soon as it gets generated.

To run it, execute the following:

```console
orca_demo_streaming --access_key ${ACCESS_KEY} --text-to-stream ${TEXT}
```

Replace `${ACCESS_KEY}` with your `AccessKey` obtained from Picovoice Console and `${TEXT}` with your text to be
streamed to Orca. Please note that this demo was not tested on macOS.

### Single synthesis demo

To synthesize speech in a single call to Orca and without audio playback, run the following:

```console
orca_demo --access_key ${ACCESS_KEY} --text ${TEXT} --output_path ${WAV_OUTPUT_PATH}
```

Replace `${ACCESS_KEY}` with yours obtained from Picovoice Console, `${TEXT}` with your text to be synthesized,
and `${WAV_OUTPUT_PATH}` with a path to a `.wav` file where the generated audio will be stored as a single-channel,
16-bit PCM `.wav` file.
