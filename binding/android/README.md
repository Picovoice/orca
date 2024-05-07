# Orca Binding for Android

## Orca Text-to-Speech Engine

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

Orca is an on-device text-to-speech engine producing high-quality, realistic, spoken audio with zero latency. Orca is:

- Private; All voice processing runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64)
    - Android and iOS
    - Chrome, Safari, Firefox, and Edge
    - Raspberry Pi (5, 4, 3) and NVIDIA Jetson Nano

## Compatibility

- Android 5.0+ (API 21+)

## Installation

Orca can be found on Maven Central. To include the package in your Android project, ensure you have
included `mavenCentral()` in your top-level `build.gradle` file and then add the following to your app's `build.gradle`:

```groovy
dependencies {
    // ...
    implementation 'ai.picovoice:orca-android:${LATEST_VERSION}'
}
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs. You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Permissions

To enable AccessKey validation, you must add the following line to your `AndroidManifest.xml` file:

```xml

<uses-permission android:name="android.permission.INTERNET"/>
```

## Usage

Orca supports two modes of operation: streaming and single synthesis. In the streaming synthesis mode, Orca processes an
incoming text stream in real-time and generates audio in parallel. In the single synthesis mode, a complete text is
synthesized in a single call to the Orca engine.

Create an instance of the engine with the Orca Builder class by passing in the accessKey, modelPath and Android app
context:

```java
import ai.picovoice.orca.*;

final String accessKey = "${ACCESS_KEY}"; // AccessKey provided by Picovoice Console (https://console.picovoice.ai/)
final String modelPath = "${MODEL_FILE_PATH}"; // path relative to the assets folder or absolute path to file (`.pv`) on device
try {
    Orca orca = new Orca.Builder()
        .setAccessKey(accessKey)
        .setModelPath(modelPath)
        .build(appContext);
} catch (OrcaException ex) { }
```

To synthesize a text stream, create an `OrcaStream` object and add text to it one-by-one:

```java
Orca.OrcaStream orcaStream = orca.streamOpen(new OrcaSynthesizeParams.Builder().build());

for (String textChunk : textGenerator()) {
  short[] pcm = orcaStream.synthesize(textChunk);
  if (pcm != null) {
    // handle pcm
  }
}

short[] flushedPcm = orcaStream.flush();
if (flushedPcm != null) {
  // handle pcm
}
```

The `textGenerator()` function can be any stream generating text, for example an LLM response.
Orca produces audio chunks in parallel to the incoming text stream, and returns the raw PCM whenever enough context has
been added via `orcaStream.synthesize()`.
To ensure smooth transitions between chunks, the `orcaStream.synthesize()` function returns an audio chunk that only
includes the audio for a portion of the text that has been added.
To generate the audio for the remaining text, `orcaStream.flush()` needs to be invoked.
When done with streaming text synthesis, the `OrcaStream` object needs to be closed:

```java
orcaStream.close();
```

If the complete text is known before synthesis, single synthesis mode can be used to generate speech in a single call to
Orca:

```java
OrcaSynthesizeParams params = new OrcaSynthesizeParams.Builder().build();

// Return raw PCM and alignments
OrcaAudio audio = orca.synthesize("${TEXT}", params);

// Save the generated audio to a WAV file directly
OrcaWord[] orcaWords = orca.synthesizeToFile("${TEXT}", "${OUTPUT_PATH}", params);
```

Replace `${TEXT}` with the text to be synthesized and `${OUTPUT_PATH}` with the path to save the generated audio as a
single-channel 16-bit PCM WAV file.
In single synthesis mode, Orca returns metadata of the synthesized audio in the form of an array of `OrcaWord`
objects.

When done make sure to explicitly release the resources using:

```java
orca.delete()
```

### Text input

Orca accepts the 26 lowercase (a-z) and 26 uppercase (A-Z) letters of the English alphabet, numbers,
basic symbols, as well as common punctuation marks. You can get a list of all supported characters by calling the
`getValidCharacters()` method provided in the Orca SDK you are using.
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
in [lib/common](../../lib/common).

To add the Orca model file to your Android application:

- Download the desired voice model from the [Orca GitHub repository](../../lib/common).
- Add the model file as a bundled resource by placing it under the assets directory of your Android
  project (`src/main/assets/`).

### Additional Synthesis Controls

Orca allows you to control the synthesized speech via the `OrcaSynthesizeParams` class. You can pass in additional
settings by using the nested Builder class:

```java
import ai.picovoice.orca.*;

OrcaSynthesizeParams params = new OrcaSynthesizeParams.Builder()
        .setSpeechRate(1.2f)
        .setRandomState(1)
        .build();
```

- `setSpeechRate()`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher value
  produces speech that is faster. The default is `1.0`.
- `setRandomState()`: Sets the random state for sampling during synthesis. This can be used to ensure that the
  synthesized speech is deterministic across different runs.

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

## Demos

To see Orca used in an app, refer to our [Android demo app](../../demo/android/OrcaDemo).
