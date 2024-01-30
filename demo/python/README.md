# Orca Text-to-Speech Engine Demo

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

## Orca

Orca is an on-device text-to-speech engine producing high-quality, realistic, spoken audio with zero latency. Orca is:

- Private; All voice processing runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64)
    - Android and iOS
    - Chrome, Safari, Firefox, and Edge
    - Raspberry Pi (5, 4, 3) and NVIDIA Jetson Nano

## Compatibility

- Python 3.7+
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

To synthesize speech, run the following:

```console
orca_demo --access_key ${ACCESS_KEY} --text ${TEXT} --output_path ${WAV_OUTPUT_PATH}
```

Replace `${ACCESS_KEY}` with yours obtained from Picovoice Console, `${TEXT}` with your text to be synthesized, 
and `${WAV_OUTPUT_PATH}` with a path to a `.wav` file where the generated audio will be stored as a single-channel, 
16-bit PCM `.wav` file.
