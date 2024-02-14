# Orca

[![GitHub](https://img.shields.io/github/license/Picovoice/picovoice)](https://github.com/Picovoice/picovoice/)

[![PyPI](https://img.shields.io/pypi/v/pvorca)](https://pypi.org/project/pvorca/)

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

<!-- markdown-link-check-disable -->
[![Twitter URL](https://img.shields.io/twitter/url?label=%40AiPicovoice&style=social&url=https%3A%2F%2Ftwitter.com%2FAiPicovoice)](https://twitter.com/AiPicovoice)
<!-- markdown-link-check-enable -->
[![YouTube Channel Views](https://img.shields.io/youtube/channel/views/UCAdi9sTCXLosG1XeqDwLx7w?label=YouTube&style=social)](https://www.youtube.com/channel/UCAdi9sTCXLosG1XeqDwLx7w)

Orca is an on-device text-to-speech engine producing high-quality, realistic, spoken audio with zero latency. Orca is:

- Private; All voice processing runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64)
    - Raspberry Pi (5, 4, 3) and NVIDIA Jetson Nano

**Please note that Orca is currently in development. While we prioritize stability and compatibility, certain aspects of
Orca may undergo changes as we continually enhance and refine the engine to provide the best user experience possible.**

## Table of Contents

- [Orca](#orca)
    - [Table of Contents](#table-of-contents)
    - [Overview](#overview)
        - [Custom pronunciations](#custom-pronunciations)
        - [Voices](#voices)
        - [Speech control](#speech-control)
        - [Audio output](#audio-output)
    - [Demos](#demos)
        - [Python](#python-demos)
        - [iOS](#ios-demo)
        - [C](#c-demos)
        - [Android](#android-demo)
    - [SDKs](#sdks)
        - [Python](#python)
        - [iOS](#ios)
        - [C](#c)
        - [Android](#android)
    - [AccessKey](#accesskey)
    - [Releases](#releases)
    - [FAQ](#faq)

## Overview

### Text input

Orca accepts the 26 lowercase (a-z) and 26 uppercase (A-Z) letters of the English alphabet, as well as
common punctuation marks. You can get a list of all supported characters by calling the
`valid_characters()` method provided in the Orca SDK you are using.
Pronunciations of characters or words not supported by this list can be achieved with
[custom pronunciations](#custom-pronunciations).

### Custom pronunciations

Orca supports custom pronunciations via a specific syntax embedded within the input text.
This feature allows users to define unique pronunciations for words using the following format: `{word|pronunciation}`.
The pronunciation is expressed in [ARPAbet](https://en.wikipedia.org/wiki/ARPABET) phonemes.
The following are examples of sentences using custom pronunciations:

- "This is a {custom|K AH S T AH M} pronunciation"
- "{read|R IY D} this as {read|R EH D}, please."
- "I {live|L IH V} in {Sevilla|S EH V IY Y AH}. We have great {live|L AY V} sports!"

### Voices

Orca can synthesize speech with various voices, each of which is characterized by a model file located
in  [lib/common](./lib/common).
To synthesize speech with a specific voice, provide the associated model file as an argument to the orca init function.
The following are the voices currently available:

|                        Model name                         | Sample rate (Hz) |
|:---------------------------------------------------------:|:----------------:|
| [orca_params_female.pv](lib/common/orca_params_female.pv) |      22050       |
|   [orca_params_male.pv](lib/common/orca_params_male.pv)   |      22050       |

### Speech control

Orca provides a set of parameters to control the synthesized speech. The following table lists the available parameters:

|  Parameter  | Default |                                                        Description                                                         |
|:-----------:|:-------:|:--------------------------------------------------------------------------------------------------------------------------:|
| speech rate |   1.0   | Speed of generated speech. Valid values are within [0.7, 1.3]. <br/>Higher (lower) values generate faster (slower) speech. |

### Audio output

Orca's synthesized speech is delivered as either raw audio data or a WAV file. Output audio will be in single-channel
16-bit PCM format and can be directly fed into a playback audio system.

## AccessKey

AccessKey is your authentication and authorization token for deploying Picovoice SDKs, including Orca. Anyone who is
using Picovoice needs to have a valid AccessKey. You must keep your AccessKey secret. You will need internet
connectivity to validate your AccessKey with Picovoice license servers even though the text-to-speech engine is running
100% offline.

AccessKey also verifies that your usage is within the limits of your account. Everyone who signs up for
[Picovoice Console](https://console.picovoice.ai/) receives the `Free Tier` usage rights described
[here](https://picovoice.ai/pricing/). If you wish to increase your limits, you can purchase a subscription plan.

## Demos

### Python Demos

To run the Python demo, run the following in the console:

```console
pip3 install pvorcademo
```

```console
orca_demo --access_key ${ACCESS_KEY} --text ${TEXT} --output_path ${WAV_OUTPUT_PATH}
```

Replace `${ACCESS_KEY}` with yours obtained from Picovoice Console, `${TEXT}` with the text to be synthesized, and
`${WAV_OUTPUT_PATH}` with a path to an output WAV file.

### iOS Demo

Run the following from [demo/ios](demo/ios) to install the Orca-iOS CocoaPod:

```console
pod install
```

Replace `let ACCESS_KEY = "..."` inside [ViewModel.swift](demo/ios/OrcaDemo/OrcaDemo/ViewModel.swift) with yours
obtained from [Picovoice Console](https://console.picovoice.ai/).

Then, using Xcode, open the generated OrcaDemo.xcworkspace and run the application.

For more information about iOS demos go to [demo/ios](demo/ios).

### C Demos

Build the demo:

```console
cmake -S demo/c/ -B demo/c/build && cmake --build demo/c/build --target orca_demo
```

Run the demo:

```console
./demo/c/build/orca_demo -l ${LIBRARY_PATH} -m ${MODEL_PATH} -a ${ACCESS_KEY} -t ${TEXT} -o ${OUTPUT_PATH}
```

### Android Demo

Using Android Studio, open [demo/android/OrcaDemo](./demo/android/OrcaDemo) as an Android project and then run the application.

Replace `"${YOUR_ACCESS_KEY_HERE}"` in the file [MainActivity.java](./demo/android/OrcaDemo/orca-demo-app/src/main/java/ai/picovoice/orcademo/MainActivity.java) with your `AccessKey`.

## SDKs

### Python

Install the Python SDK:

```console
pip3 install pvorca
```

Create an instance of the engine and generate speech:

```python
import pvorca

orca = pvorca.create(access_key='${ACCESS_KEY}')
pcm = orca.synthesize('${TEXT}')
```

Replace `${ACCESS_KEY}` with yours obtained from [Picovoice Console](https://console.picovoice.ai/) and `${TEXT}` with
the text to be synthesized including potential [custom pronunciations](#custom-pronunciations).

Finally, when done make sure to explicitly release the resources:

```python
orca.delete()
```

For more details see [Python SDK](./binding/python/README.md).

### iOS

Create an instance of the engine and synthesize:

```swift
import Orca

let modelPath = Bundle(for: type(of: self)).path(
        forResource: "${MODEL_FILE}", // Name of the model file name for Orca
        ofType: "pv")!

do {
  let orca = try Orca(accessKey: "${ACCESS_KEY}", modelPath: modelPath)
} catch {}

do {
    let pcm = try orca.synthesize(text: "${TEXT}")
} catch {}
```

Replace `${ACCESS_KEY}` with yours obtained from [Picovoice Console](https://console.picovoice.ai/), `${MODEL_FILE}` with the model file name for Orca and `${TEXT}` with
the text to be synthesized including potential [custom pronunciations](#custom-pronunciations).

When done be sure to explicitly release the resources using `orca.delete()`.

### C

The header file [include/pv_orca.h](./include/pv_orca.h) contains relevant information on Orca's C SDK.

Build an instance of the object:

```c
pv_orca_t *handle = NULL;
const char *model_path = "${MODEL_PATH}";
pv_status_t status = pv_orca_init("${ACCESS_KEY}", model_path, &handle);
if (status != PV_STATUS_SUCCESS) {
    // error handling logic
}
```

Replace `${ACCESS_KEY}` with the AccessKey obtained from [Picovoice Console](https://console.picovoice.ai/),
and `${MODEL_PATH}` with the path to the model file available under [lib/common](./lib/common).

Create a `synthesize_params` object to control the synthesized speech:

```c
pv_orca_synthesize_params_t *synthesize_params = NULL;
status = pv_orca_synthesize_params_init(&synthesize_params);
// change the default parameters of synthesize_params as desired
```

Now, the `handle` and `synthesize_params` object can be used to synthesize speech:

```c
int32_t num_samples = 0;
int16_t *synthesized_pcm = NULL;
status = pv_orca_synthesize(
    handle,
    "${TEXT}",
    synthesize_params,
    &num_samples,
    &synthesized_pcm);
```

Replace `${TEXT}` with the text to be synthesized including potential [custom pronunciations](#custom-pronunciations).

Finally, when done make sure to release the acquired resources:

```c
pv_orca_delete_pcm(pcm);
pv_orca_synthesize_params_delete(synthesize_params);
pv_orca_delete(handle);
```

### Android

To include the Orca package in your Android project, ensure you have included `mavenCentral()` in your top-level `build.gradle` file and then add the following to your app's `build.gradle`:

```groovy
dependencies {
    implementation 'ai.picovoice:orca-android:${LATEST_VERSION}'
}
```

Create an instance of the engine and generate speech:

```java
import ai.picovoice.orca.*;

final String accessKey = "${ACCESS_KEY}"; // AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)
final String modelPath = "${MODEL_FILE_PATH}";
try {
    Orca orca = new Orca.Builder()
        .setAccessKey(accessKey)
        .setModelPath(modelPath)
        .build(appContext);

    short[] pcm = orca.synthesize(
        "${TEXT}",
        new OrcaSynthesizeParams.Builder().build());

} catch (OrcaException ex) { }
```

Replace `${ACCESS_KEY}` with yours obtained from Picovoice Console, `${MODEL_FILE_PATH}` with an Orca [voice model file](./lib/common) and `${TEXT}` with the text to be synthesized including potential [custom pronunciations](#custom-pronunciations).

Finally, when done make sure to explicitly release the resources:

```java
orca.delete()
```

For more details, see the [Android SDK](./binding/android/README.md).

## Releases

### v0.1.0 - January 24th, 2024

- Beta release

## FAQ

You can find the FAQ [here](https://picovoice.ai/docs/faq/general/).
