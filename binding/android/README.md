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

Orca can be found on Maven Central. To include the package in your Android project, ensure you have included `mavenCentral()` in your top-level `build.gradle` file and then add the following to your app's `build.gradle`:

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
<uses-permission android:name="android.permission.INTERNET" />
```

## Usage

Create an instance of the engine with the Orca Builder class by passing in the accessKey, modelPath and Android app context:

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

You can synthesize speech by calling one of the available `synthesize` methods:

```java
OrcaSynthesizeParams params = new OrcaSynthesizeParams.Builder().build();

// Return raw PCM
short[] pcm = orca.synthesize("${TEXT}", params);

// Save the generated audio to a WAV file directly
orca.synthesizeToFile("${OUTPUT_PATH}", "${TEXT}", params);
```

Replace `${TEXT}` with the text to be synthesized (must be fewer characters than `.getMaxCharacterLimit()`). When using `synthesize`, the generated pcm has a sample rate equal to the one returned by `getSampleRate()`. When using `synthesizeToFile`, replace `${OUTPUT_PATH}` with the path to save the generated audio as a single-channel 16-bit PCM WAV file. When done make sure to explicitly release the resources with `orca.delete()`.

### Text Input

Orca accepts any character found in the list returned by the `getValidCharacters()` method.
Pronunciations of characters or words not supported by this list can be achieved by embedding custom pronunciations in the text via the syntax: `{word|pronunciation}`. The pronunciation is expressed in [ARPAbet](https://en.wikipedia.org/wiki/ARPABET) phonemes, for example:

- "This is a {custom|K AH S T AH M} pronunciation"
- "{read|R IY D} this as {read|R EH D}, please."
- "I {live|L IH V} in {Sevilla|S EH V IY Y AH}. We have great {live|L AY V} sports!"

### Voices

Orca can synthesize speech with various voices, each of which is characterized by a model file located
in [lib/common](../../lib/common).

To add the Orca model file to your Android application:

- Download the desired voice model from the [Orca GitHub repository](../../lib/common).
- Add the model file as a bundled resource by placing it under the assets directory of your Android project (`src/main/assets/`).

### Additional Synthesis Controls

Orca allows you to control the synthesized speech via the `OrcaSynthesizeParams` class. You can pass in additional settings by using the nested Builder class:

```java
import ai.picovoice.orca.*;

OrcaSynthesizeParams params = new OrcaSynthesizeParams.Builder()
        .setSpeechRate(1.2f)
        .build();
```

- `setSpeechRate()`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher value
  produces speech that is faster. The default is `1.0`.

## Demos

To see Orca used in an app, refer to our [Android demo app](../../demo/android).