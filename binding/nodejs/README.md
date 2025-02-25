# Orca Binding for Node.js

## Orca Streaming Text-to-Speech Engine

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

Orca is an on-device streaming text-to-speech engine that is designed for use with LLMs, enabling zero-latency
voice assistants. Orca is:

- Private; All speech synthesis runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), and Windows (x86_64, arm64)
    - Android and iOS
    - Chrome, Safari, Firefox, and Edge
    - Raspberry Pi (3, 4, 5)

## Compatibility

- Node.js 16+
- Runs on Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64, arm64), and Raspberry Pi (3, 4, 5).

## Installation

```console
npm install @picovoice/orca-node
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Usage

Orca supports two modes of operation: streaming and single synthesis.
In the streaming synthesis mode, Orca processes an incoming text stream in real-time and generates audio in parallel.
In the single synthesis mode, the complete text needs to be known in advance and is synthesized in a single call to
the Orca engine.

Create an instance of the Orca engine:

```typescript
const { Orca } = require("@picovoice/orca-node");

const accessKey = "${ACCESS_KEY}"; // Obtained from the Picovoice Console (https://console.picovoice.ai/)

const orca = new Orca(accessKey);
```

Replace `${ACCESS_KEY}` with your AccessKey obtained from [Picovoice Console](https://console.picovoice.ai/).

To synthesize a text stream, create an `OrcaStream` object and add text to it one-by-one:

```typescript
const stream = orca.streamOpen();

for (const textChunk of textGenerator()) {
  const pcm = stream.synthesize(textChunk);
  if (pcm !== null) {
    // handle pcm
  }
}
```

The `textGenerator()` function can be any stream generating text, for example an LLM response.
The `OrcaStream` object buffers input text until there is enough to generate audio. If there is not enough text to
generate
audio, `null` is returned.

To ensure smooth transitions between chunks, the `stream.synthesize()` function returns an audio chunk that only
includes the audio for a portion of the text that has been added.

When done, call `flush` to synthesize any remaining text, and `close` to delete the `OrcaStream` object.

```typescript
const flushedPcm = stream.flush();
if (flushedPcm !== null) {
  // handle flushed pcm
}

stream.close()
```

If the complete text is known before synthesis, single synthesis mode can be used to generate speech in a single call to
Orca:

```typescript
const result = orca.synthesize("${TEXT}");

const alignments = orca.synthesizeToFile("${TEXT}", "${OUTPUT_PATH}");
```

Replace `${TEXT}` with the text to be synthesized and `${OUTPUT_PATH}` with the path to save the generated audio as a
single-channel 16-bit PCM WAV file.
In single synthesis mode, Orca returns metadata of the synthesized audio in the form of a list of `OrcaAlignment`
objects.

When done make sure to explicitly release the resources using:

```typescript
orca.release()
```

### Text input

Orca supports a wide range of English characters, including letters, numbers, symbols, and punctuation marks.
You can get a list of all supported characters by calling `validCharacters()`.
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

```typescript
const orca = new Orca(accessKey, { modelPath: "${MODEL_PATH}" });
```

and replace `${MODEL_PATH}` with the path to the model file with the desired voice.

### Speech control

Orca allows for keyword arguments to control the synthesized speech. They can be provided to the `streamOpen`
method or the single synthesis methods `synthesize` and `synthesizeToFile`:

- `speechRate`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher (lower) value
  produces speech that is faster (slower). The default is `1.0`.
- `randomState`: Sets the random state for sampling during synthesis. This can be used to ensure that the synthesized
  speech is deterministic across different runs. Valid values are all non-negative integers. If not provided, a random
  seed will be chosen and the synthesis process will be non-deterministic.

```typescript
const synthesizeParams = {
  speechRate: 1.3,
  randomState: 42,
};

// Streaming synthesis
const OrcaStream = await orca.streamOpen(synthesizeParams);

// Single synthesis
const result = await orca.synthesize("${TEXT}", synthesizeParams);
const alignments = await orca.synthesizeToFile("${TEXT}", "${OUTPUT_PATH}", synthesizeParams);
```

### Orca properties

To obtain the set of valid characters, call `orca.validCharacters`.
To retrieve the maximum number of characters allowed, call `orca.maxCharacterLimit`.
The sample rate of Orca is `orca.sampleRate`.

### Alignment Metadata

Along with the raw PCM or saved audio file, Orca returns metadata for the synthesized audio in single synthesis mode.
The `OrcaAlignment` object has the following properties:

- **Word:** String representation of the word.
- **Start Time:** Indicates when the word started in the synthesized audio. Value is in seconds.
- **End Time:** Indicates when the word ended in the synthesized audio. Value is in seconds.
- **Phonemes:** An array of `OrcaPhoneme` objects.

The `OrcaPhoneme` object has the following properties:

- **Phoneme:** String representation of the phoneme.
- **Start Time:** Indicates when the phoneme started in the synthesized audio. Value is in seconds.
- **End Time:** Indicates when the phoneme ended in the synthesized audio. Value is in seconds.

## Demos

[Orca Node.js demo package](https://www.npmjs.com/package/@picovoice/orca-node-demo) provides command-line utilities for
processing audio using Orca.
