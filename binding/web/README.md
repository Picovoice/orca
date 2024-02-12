# Orca Binding for Web

## Orca Text-to-Speech Engine

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

Orca is an on-device text-to-speech engine producing high-quality, realistic, spoken audio with zero latency. Orca is:

- Private; All voice processing runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), and Windows (x86_64)
    - Chrome, Safari, Firefox, and Edge
    - Raspberry Pi (3, 4, 5) and NVIDIA Jetson Nano

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
and feeds it to Orca. Copy a model file for the desired voice (male or female) into the public
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

### Init Options

Set `synthesizeErrorCallback` to handle any errors that occur while synthesizing.

```typescript
// Optional
const options = {
  synthesizeErrorCallback: (error) => {
  }
}
```

### Initialize Orca

Create a `speechCallback` function to get the synthesized speech result
from the engine:

```typescript
function speechCallback(orcaSpeech: OrcaSpeech) {
  console.log(orcaSpeech.speech)
}
```

Create an instance of `Orca` on the main thread:

```typescript
const orca = await Orca.create(
  "${ACCESS_KEY}",
  speechCallback,
  orcaModel,
  options // optional options
);
```

Or create an instance of `Orca` in a worker thread:

```typescript
const orca = await OrcaWorker.create(
  "${ACCESS_KEY}",
  speechCallback,
  orcaModel,
  options // optional options
);
```

### Custom Pronunciations

Orca allows the embedding of custom pronunciations in the text via the syntax: `{word|pronunciation}`. The pronunciation
is expressed in [ARPAbet](https://en.wikipedia.org/wiki/ARPABET) phonemes, for example:

- "This is a {custom|K AH S T AH M} pronunciation"
- "{read|R IY D} this as {read|R EH D}, please."
- "I {live|L IH V} in {Sevilla|S EH V IY Y AH}. We have great {live|L AY V} sports!"

### Synthesize Speech

The `synthesize` function will send the text to the engine.
The speech audio is received from `speechCallback` as mentioned above.

```typescript
orca.synthesize("${TEXT}");
```

### Speech Control

Orca allows for an additional argument to be provided to the `synthesize` method to control the synthesized speech:

- `speechRate`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher value
  produces speech that is faster, and a lower value produces speech that is slower. The default value is `1.0`.

```typescript
const speechRate = 1.3;

orca.synthesize(TEXT, speechRate);
```

### Orca Properties

To obtain the set of valid punctuation symbols, call `.validPunctuationSymbols`. To retrieve the maximum number of
characters allowed, call `.maxCharacterLimit`. The sample rate of Orca is `.sampleRate`.

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

For example usage refer to our [Web demo application](../../demo/web).
