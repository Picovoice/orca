# Orca Text-to-Speech Engine

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

Orca is an on-device text-to-speech engine producing high-quality, realistic, spoken audio with zero latency. Orca is:

- Private; All voice processing runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64)
    - Android and iOS
    - Chrome, Safari, Firefox, and Edge
    - Raspberry Pi (5, 4, 3) and NVIDIA Jetson Nano

## Compatibility

- iOS 13.0 or higher

## Installation

<!-- markdown-link-check-disable -->
The Orca iOS binding is available via [Cocoapods](https://cocoapods.org/pods/Orca-iOS). To import it into your iOS
project, add the following line to your Podfile and run `pod install`:
<!-- markdown-link-check-enable -->

```ruby
pod 'Orca-iOS'
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Usage

Orca supports two modes of operation: streaming and single synthesis.
In the streaming synthesis mode, Orca processes an incoming text stream in real-time and generates audio in parallel.
In the single synthesis mode, a complete text is synthesized in a single call to the Orca engine.

Create an instance of the Orca engine:

```swift
import Orca

let accessKey : String = // .. accessKey provided by Picovoice Console (https://console.picovoice.ai/)

let modelPath = Bundle(for: type(of: self)).path(
        forResource: "${MODEL_FILE}", // Name of the model file name for Orca
        ofType: "pv")!

do {
    let orca = try Orca(accessKey: accessKey, modelPath: modelPath)
} catch { }
```

Alternatively, you can provide `modelPath` as an absolute path to the model file on device.

To synthesize a text stream, create an `Orca.OrcaStream` object and add text to it one-by-one:

```swift
let orcaStream = try orca.streamOpen()

for textChunk in textGenerator() {
  let pcm = orcaStream.synthesize(textChunk)
  if pcm != nil {
    // handle pcm
  }
}

let pcm = orcaStream.flush()
if pcm != nil {
  // handle pcm
}
```

The `textGenerator()` function can be any stream generating text, for example an LLM response.
Orca produces audio chunks in parallel to the incoming text stream, and returns the raw PCM whenever enough context has
been added via `orcaStream.synthesize()`.
To ensure smooth transitions between chunks, the `orcaStream.synthesize()` function returns an audio chunk that only
includes the audio for a portion of the text that has been added.
To generate the audio for the remaining text, `orcaStream.flush()` needs to be invoked.
When done with streaming text synthesis, the `Orca.OrcaStream` object needs to be closed:

```swift
orcaStream.close()
```

If the complete text is known before synthesis, single synthesis mode can be used to generate speech in a single call to
Orca:

```swift
// Return raw PCM and alignments
let (pcm, wordArray) = try orca.synthesize(text: "${TEXT}")

// Save the generated audio to a WAV file directly
let wordArray = try orca.synthesizeToFile(text: "${TEXT}", outputPath: "${OUTPUT_PATH}")
```

Replace `${TEXT}` with the text to be synthesized and `${OUTPUT_PATH}` with the path to save the generated audio as a
single-channel 16-bit PCM WAV file.
In single synthesis mode, Orca returns metadata of the synthesized audio in the form of an array of `OrcaWord`
objects.
When done, resources have to be released explicitly:

```swift
orca.delete()
```

### Text input

Orca accepts the 26 lowercase (a-z) and 26 uppercase (A-Z) letters of the English alphabet, numbers,
basic symbols, as well as common punctuation marks. You can get a list of all supported characters by calling the
`validCharacters()` method provided in the Orca SDK you are using.
Pronunciations of characters or words not supported by this list can be achieved with
[custom pronunciations](#custom-pronunciations).

### Custom pronunciations

Orca allows to embed custom pronunciations in the text via the syntax: `{word|pronunciation}`.\
The pronunciation is expressed in [ARPAbet](https://en.wikipedia.org/wiki/ARPABET) phonemes, for example:

- "This is a {custom|K AH S T AH M} pronunciation"
- "{read|R IY D} this as {read|R EH D}, please."
- "I {live|L IH V} in {Sevilla|S EH V IY Y AH}. We have great {live|L AY V} sports!"

### Voices

Orca can synthesize speech with various voices, each of which is characterized by a model file located
in [lib/common](https://github.com/Picovoice/orca/tree/main/lib/common).
To create an instance of the engine with a specific voice, use:

```swift
import Orca

do {
    let orca = try Orca(accessKey: accessKey, modelPath: "${MODEL_FILE_PATH}")
    // or
    let orca = try Orca(accessKey: accessKey, modelURL: "${MODEL_FILE_URL}")
} catch { }
```

and replace `${MODEL_FILE_PATH}` or `${MODEL_FILE_URL}` with the path to the model file with the desired voice.

### Speech control

Orca allows for keyword arguments to control the synthesized speech. They can be provided to the `streamOopen`
method or the single synthesis methods `synthesize` and `synthesizeToFile`:

- `speechRate`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher (lower) value
  produces speech that is faster (slower). The default is `1.0`.
- `randomState`: Sets the random state for sampling during synthesis. This can be used to ensure that the synthesized
  speech is deterministic across different runs.

```swift
let pcm = orca.synthesize(
    text: "${TEXT}",
    speechRate: 1.0,
    randomState: 1)
```

### Orca properties

To obtain the set of valid characters, call `Orca.validCharacters`.
To retrieve the maximum number of characters allowed, call `Orca.maxCharacterLimit`.
The sample rate of Orca is `Orca.sampleRate`.

### Alignment Metadata

Along with the raw PCM or saved audio file, Orca returns metadata for the synthesized audio in single synthesis mode.
The `OrcaWord` object has the following properties:

- **Word:** String representation of the word.
- **Start Time:** Indicates when the word started in the synthesized audio. Value is in seconds.
- **End Time:** Indicates when the word ended in the synthesized audio. Value is in seconds.
- **Phonemes:** An array of `OrcaPhoneme` objects.

The `OrcaPhoneme` object has the following properties:

- **Phoneme:** String representation of the phoneme.
- **Start Time:** Indicates when the phoneme started in the synthesized audio. Value is in seconds.
- **End Time:** Indicates when the phoneme ended in the synthesized audio. Value is in seconds.

## Running Unit Tests

Copy your `AccessKey` into the `accessKey` variable
in [`OrcaAppTestUITests.swift`](OrcaAppTest/OrcaAppTestUITests/OrcaAppTestUITests.swift). Open `OrcaAppTest.xcworkspace`
with XCode and run the tests with `Product > Test`.

## Demo App

For example usage refer to our [iOS demo application](../../demo/ios).
