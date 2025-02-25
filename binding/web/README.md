# Orca Binding for Web

## Orca Streaming Text-to-Speech Engine

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

Orca is an on-device streaming text-to-speech engine that is designed for use with LLMs, enabling zero-latency
voice assistants. Orca is:

- Private; All speech synthesis runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), and Windows (x86_64)
    - Android and iOS
    - Chrome, Safari, Firefox, and Edge
    - Raspberry Pi (3, 4, 5)

## Compatibility

- Chrome / Edge
- Firefox
- Safari

### Restrictions

IndexedDB is required to use `Orca` in a worker thread. Browsers without IndexedDB support
(i.e. Firefox Incognito Mode) should use `Orca` in the main thread.

## Installation

### Package

Using `Yarn`:

```console
yarn add @picovoice/orca-web
```

or using `npm`:

```console
npm install --save @picovoice/orca-web
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Usage

For the web packages, there are two methods to initialize Orca.

### Public Directory

**NOTE**: Due to modern browser limitations of using a file URL, this method does __not__ work if used without hosting a
server.

This method fetches the [model file](https://github.com/Picovoice/orca/tree/main/lib/common) from the public directory
and feeds it to Orca. Copy a model file for the desired language and voice into the public
directory:

```console
cp ${ORCA_MODEL_FILE} ${PATH_TO_PUBLIC_DIRECTORY}
```

### Base64

**NOTE**: This method works without hosting a server, but increases the size of the model file roughly by 33%.

This method uses a base64 string of the model file and feeds it to Orca. Use the built-in script `pvbase64` to
base64 your model file:

```console
npx pvbase64 -i ${ORCA_MODEL_FILE} -o ${OUTPUT_DIRECTORY}/${MODEL_NAME}.js
```

The output will be a js file which you can import into any file of your project. For detailed information
about `pvbase64`,
run:

```console
npx pvbase64 -h
```

### Orca Model

Orca saves and caches your model file in IndexedDB to be used by WebAssembly. Use a different `customWritePath` variable
to hold multiple models and set the `forceWrite` value to true to force re-save a model file.

Either `base64` or `publicPath` must be set to instantiate Orca. If both are set, Orca will use the `base64` model.

```typescript
const orcaModel = {
  publicPath: "${MODEL_RELATIVE_PATH}",
  // or
  base64: "${MODEL_BASE64_STRING}",

  // Optionals
  customWritePath: "orca_model",
  forceWrite: false,
  version: 1,
}
```

### Initialize Orca

Create an instance of `Orca` on the main thread:

```typescript
const orca = await Orca.create(
  "${ACCESS_KEY}",
  orcaModel
);
```

Or create an instance of `Orca` in a worker thread:

```typescript
const orca = await OrcaWorker.create(
  "${ACCESS_KEY}",
  orcaModel
);
```

### Streaming vs. Single Synthesis

Orca supports two modes of operation: streaming and single synthesis.
In the streaming synthesis mode, Orca processes an incoming text stream in real-time and generates audio in parallel.
In the single synthesis mode, the complete text needs to be known in advance and is synthesized in a single call to
the Orca engine.

### Custom Pronunciations

Orca allows the embedding of custom pronunciations in the text via the syntax: `{word|pronunciation}`. The pronunciation
is expressed in [ARPAbet](https://en.wikipedia.org/wiki/ARPABET) phonemes, for example:

- "This is a {custom|K AH S T AH M} pronunciation"
- "{read|R IY D} this as {read|R EH D}, please."
- "I {live|L IH V} in {Sevilla|S EH V IY Y AH}. We have great {live|L AY V} sports!"

### Orca Properties

To obtain the complete set of valid characters, call `.validCharacters`. To retrieve the maximum number of
characters allowed, call `.maxCharacterLimit`. The sample rate of the generated `Int16Array` is `.sampleRate`.

### Usage

#### Streaming Synthesis

To use streaming synthesis, call `streamOpen` to create an `OrcaStream` object.

```typescript
const orcaStream = await orca.streamOpen();
```

Then, call `synthesize` on `orcaStream` to generate speech from a stream of text:

```typescript
function* textStream(): IterableIterator<string> {
  ... // yield text chunks e.g. from an LLM response
}

for (const textChunk of textStream()) {
  const pcm = await orcaStream.synthesize(textChunk);
  if (pcm !== null) {
    // handle pcm
  }
}
```

The `OrcaStream` object buffers input text until there is enough to generate audio. If there is not enough text to
generate
audio, `null` is returned.

When done, call `flush` to synthesize any remaining text, and `close` to delete the `orcaStream` object.

```typescript
const flushedPcm = orcaStream.flush();
if (flushedPcm !== null) {
  // handle pcm
}

orcaStream.close();
```

#### Single Synthesis

To use single synthesis, simply call `synthesize` directly on the `Orca` instance. The `synthesize` function will send
the text to the engine and return the speech audio data as an `Int16Array` as well as
the [alignments metadata](#alignments-metadata).

```typescript
const { pcm, alignments } = await orca.synthesize("${TEXT}");
```

### Speech Control

Orca allows for additional arguments to control the synthesized speech.
These can be provided to `streamOpen` or one of the single mode `synthesize` methods:

- `speechRate`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher value produces
  speech that is faster, and a lower value produces speech that is slower. The default value is `1.0`.

```typescript
const synthesizeParams = {
  speechRate: 1.3,
};

// Streaming synthesis
const OrcaStream = await orca.streamOpen(synthesizeParams);

// Single synthesis
const result = await orca.synthesize("${TEXT}", synthesizeParams);

```

### Alignments Metadata

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

### Clean Up

Clean up used resources by `Orca` or `OrcaWorker`:

```typescript
await orca.release();
```

### Terminate (Worker only)

Terminate `OrcaWorker` instance:

```typescript
await orca.terminate();
```

## Demo

For example usage refer to our [Web demo application](https://github.com/Picovoice/orca/tree/main/demo/web).
