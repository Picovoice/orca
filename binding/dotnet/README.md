# Orca Binding for .NET

## Orca Streaming Text-to-Speech Engine

Orca is an on-device streaming text-to-speech engine that is designed for use with LLMs, enabling zero-latency
voice assistants. Orca is:

- Private; All speech synthesis runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64, arm64)
    - Android and iOS
    - Raspberry Pi (3, 4, 5)
    - Chrome, Safari, Firefox, and Edge

## Requirements

- .NET 8.0

## Compatibility

Platform compatible with .NET Framework 4.6.1+:

- Windows (x86_64)

Platforms compatible with .NET Core 2.0+:

- macOS (x86_64)
- Windows (x86_64)

Platform compatible with .NET 6.0+:

- Raspberry Pi:
  - 3 (32 and 64 bit)
  - 4 (32 and 64 bit)
  - 5 (32 and 64 bit)
- Linux (x86_64)
- macOS (arm64)
- Windows (arm64)

## Installation

You can install the latest version of Orca by getting the latest [Orca Nuget package](https://www.nuget.org/packages/Picovoice.Orca/)
in Visual Studio or using the .NET CLI.

```console
dotnet add package Picovoice.Orca
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs. You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Usage

Orca supports two modes of operation: streaming and single synthesis.
In the streaming synthesis mode, Orca processes an incoming text stream in real-time and generates audio in parallel.
In the single synthesis mode, a complete text is synthesized in a single call to the Orca engine.

Create an instance of the Orca engine:

```csharp
using Pv;

const string accessKey = "${ACCESS_KEY}"; // Obtained from the Picovoice Console (https://console.picovoice.ai/)

Orca orca = Orca.Create(accessKey);
```

Replace the `${ACCESS_KEY}` with your AccessKey obtained from [Picovoice Console](https://console.picovoice.ai/).

To synthesize a text stream, create an `Orca.OrcaStream` object and add text to it one-by-one:

```csharp
Orca.OrcaStream stream = orca.StreamOpen();

foreach (string textChunk in TextGenerator())
{
    short[] streamPcm = orcaStream.Synthesize(textChunk);
    if (streamPcm != null)
    {
        // handle pcm chunk
    }
}

short[] flushPcm = orcaStream.Flush();
if (flushPcm != null)
{
    // handle final pcm chunk
}
```

The `TextGenerator()` function can be any stream generating text, for example an LLM response.
Orca produces audio chunks in parallel to the incoming text stream, and returns the raw PCM whenever enough context has
been added via `orcaStream.Synthesize()`.
To ensure smooth transitions between chunks, the `orcaStream.Synthesize()` function returns an audio chunk that only
includes the audio for a portion of the text that has been added.
To generate the audio for the remaining text, `orcaStream.Flush()` needs to be invoked.

`OrcaStream` implements `IDisposable`, so you either call `.Dispose()` to close it or wrap it in a using statement:

```csharp
using(Orca.OrcaStream stream = orca.StreamOpen())
{
    // .. OrcaStream usage here
}
```

If the complete text is known before synthesis, single synthesis mode can be used to generate speech in a single call to
Orca:

```csharp
// Return raw audio data (PCM) and alignments
OrcaAudio res = orca.Synthesize("${TEXT}");

// Save the generated audio to a WAV file directly and just return alignments
OrcaWord[] alignments = orca.SynthesizeToFile("${TEXT}", "${OUTPUT_PATH}");
```

Replace `${TEXT}` with the text to be synthesized and `${OUTPUT_PATH}` with the path to save the generated audio as a
single-channel 16-bit PCM WAV file.
In single synthesis mode, Orca returns metadata of the synthesized audio in the form of a list of `OrcaWord`
objects.
You can print the metadata with:

```csharp
foreach (OrcaWord w in alignments)
{
    Console.WriteLine($"word=\"{w.Word}\", start_sec={w.StartSec}, end_sec={w.EndSec}");
    foreach (OrcaPhoneme p in token.Phonemes)
    {
        Console.WriteLine($"\tphoneme=\"{p.Phoneme}\", start_sec={p.StartSec}, end_sec={p.EndSec}");
    }
}
```

`Orca` will have its resources freed by the garbage collector, but to have resources freed immediately after use,
wrap it in a using statement:

```csharp
using(Orca orca = Orca.Create(accessKey))
{
    // .. Orca usage here
}
```

### Text input

Orca supports a wide range of English characters, including letters, numbers, symbols, and punctuation marks.
You can get a list of all supported characters with the `.valid_characters` property.
Pronunciations of characters or words not supported by this list can be achieved with
[custom pronunciations](#custom-pronunciations).

### Custom pronunciations

Orca allows to embed custom pronunciations in the text via the syntax: `{word|pronunciation}`.\
The pronunciation is expressed in [ARPAbet](https://en.wikipedia.org/wiki/ARPABET) phonemes, for example:

- "This is a {custom|K AH S T AH M} pronunciation"
- "{read|R IY D} this as {read|R EH D}, please."
- "I {live|L IH V} in {Sevilla|S EH V IY Y AH}. We have great {live|L AY V} sports!"

### Language and Voice

Orca Streaming Text-to-Speech can synthesize speech in different languages and with a variety of voices,
each of which is characterized by a model file (`.pv`) located in [lib/common](../../lib/common).
The language and gender of the speaker is indicated in the file name.

To create an instance of the engine with a specific voice, use:

```csharp
Orca orca = Orca.Create(accessKey, "${MODEL_PATH}");
```

and replace `${MODEL_PATH}` with the path to the model file with the desired voice.

### Speech control

Orca allows for keyword arguments to control the synthesized speech. They can be provided to the `StreamOpen`
method or the single synthesis methods `Synthesize` and `SynthesizeToFile`:

- `speechRate`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher (lower) value
  produces speech that is faster (slower). The default is `1.0`.
- `randomState`: Sets the random state for sampling during synthesis. This can be used to ensure that the synthesized
  speech is deterministic across different runs. Valid values are all non-negative integers. If not provided, a random
  seed will be chosen and the synthesis process will be non-deterministic.

### Orca properties

To obtain the set of valid characters, call `orca.ValidCharacters`.\
To retrieve the maximum number of characters allowed, call `orca.MaxCharacterLimit`.\
The sample rate of Orca is `orca.SampleRate`.

### Alignment Metadata

Along with the raw PCM or saved audio file, Orca returns metadata for the synthesized audio in single synthesis mode.
The `OrcaWord` object has the following properties:

- **Word:** String representation of the word.
- **Start Time:** Indicates when the word started in the synthesized audio. Value is in seconds.
- **End Time:** Indicates when the word ended in the synthesized audio. Value is in seconds.
- **Phonemes:** A list of `OrcaPhoneme` objects.

The `OrcaPhoneme` object has the following properties:

- **Phoneme:** String representation of the phoneme.
- **Start Time:** Indicates when the phoneme started in the synthesized audio. Value is in seconds.
- **End Time:** Indicates when the phoneme ended in the synthesized audio. Value is in seconds.

## Demos

The [Orca dotnet demo project](https://github.com/Picovoice/orca/tree/main/demo/dotnet) is a .NET command line application that allows for synthesizing audio using Orca.
