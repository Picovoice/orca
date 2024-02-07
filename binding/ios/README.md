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
The Orca iOS binding is available via [Cocoapods](https://cocoapods.org/pods/Orca-iOS). To import it into your iOS project, add the following line to your Podfile and run `pod install`:
<!-- markdown-link-check-enable -->

```ruby
pod 'Orca-iOS'
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Usage

Create an instance of the engine

```swift
import Orca

let accessKey : String = // .. accessKey provided by Picovoice Console (https://console.picovoice.ai/)
do {
    orca = try Orca(accessKey: accessKey)
} catch { }
```

You can synthesize speech by calling one of the `synthesize` methods:

```swift
// return raw pcm
pcm = orca.synthesize(text="${TEXT}")
```

Replace `${TEXT}` with the text to be synthesized. 

When done, resources have to be released explicitly:

```swift
orca.delete()
```

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
    orca = try Orca(accessKey: accessKey, modelPath="${MODEL_PATH}")
} catch { }
```

and replace `${MODEL_PATH}` with the path to the model file with the desired voice.

### Speech control

Orca allows for keyword arguments to be provided to the `synthesize` methods to control the synthesized speech:

- `speechRate`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher (lower) value
  produces speech that is faster (slower). The default is `1.0`.

```swift
pcm = orca.synthesize(
    text="${TEXT}",
    speechRate=1.0)
```

### Orca properties

To obtain the set of valid punctuation symbols, call `Orca.validPunctuationSymbols`.
To retrieve the maximum number of characters allowed, call `Orca.maxCharacterLimit`.
The sample rate of Orca is `Orca.sampleRate`.

## Running Unit Tests

Copy your `AccessKey` into the `accessKey` variable in [`OrcaAppTestUITests.swift`](OrcaAppTest/OrcaAppTestUITests/OrcaAppTestUITests.swift). Open `OrcaAppTest.xcworkspace` with XCode and run the tests with `Product > Test`.

## Demo App

For example usage refer to our [iOS demo application](../../demo/ios).
