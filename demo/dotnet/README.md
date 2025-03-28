# Orca Streaming Text-to-Speech Engine Demos for .NET

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

## Orca

Orca is an on-device streaming text-to-speech engine that is designed for use with LLMs, enabling zero-latency
voice assistants. Orca is:

- Private; All speech synthesis runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64)
    - Android and iOS
    - Chrome, Safari, Firefox, and Edge
    - Raspberry Pi (3, 4, 5)

## Requirements

- .NET 8.0

## Compatibility

- Linux (x86_64)
- macOS (x86_64, arm64)
- Windows (x86_64)
- Raspberry Pi:
  - 3 (32 and 64 bit)
  - 4 (32 and 64 bit)
  - 5 (32 and 64 bit)

## Installation

Both demos use [Microsoft's .NET 8.0](https://dotnet.microsoft.com/download).

Build with the dotnet CLI:

```console
dotnet build -c StreamingDemo.Release
dotnet build -c FileDemo.Release
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Usage

NOTE: File path arguments must be absolute paths. The working directory for the following dotnet commands is:

```console
orca/demo/dotnet/OrcaDemo
```

For both demos, you can use `--help/-h` to see the list of input arguments.

### Streaming Synthesis Demo

In this demo, we simulate a response from a language model by creating a text stream from a user-defined text.
We stream that text to Orca and play the synthesized audio as soon as it gets generated.

To run it, execute the following:

```console
dotnet run -c StreamingDemo.Release -- --access_key ${ACCESS_KEY} --text_to_stream ${TEXT}
```

Replace `${ACCESS_KEY}` with your `AccessKey` obtained from Picovoice Console and `${TEXT}` with your text to be
streamed to Orca.

### Single Synthesis Demo

To synthesize speech in a single call to Orca and without audio playback, run the following:

```console
dotnet run -c FileDemo.Release -- --access_key ${ACCESS_KEY} --text ${TEXT} --output_path ${WAV_OUTPUT_PATH}
```

Replace `${ACCESS_KEY}` with yours obtained from Picovoice Console, `${TEXT}` with your text to be synthesized,
and `${WAV_OUTPUT_PATH}` with a path to a `.wav` file where the generated audio will be stored as a single-channel,
16-bit PCM `.wav` file.
