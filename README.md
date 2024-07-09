# Orca

[![GitHub](https://img.shields.io/github/license/Picovoice/orca)](https://github.com/Picovoice/orca/)

[![Maven Central](https://img.shields.io/maven-central/v/ai.picovoice/orca-android?label=maven-central%20%5Bandroid%5D)](https://repo1.maven.org/maven2/ai/picovoice/orca-android/)
[![npm](https://img.shields.io/npm/v/@picovoice/orca-node?label=npm%20%5Bnode%5D)](https://www.npmjs.com/package/@picovoice/orca-node)
[![npm](https://img.shields.io/npm/v/@picovoice/orca-web?label=npm%20%5Bweb%5D)](https://www.npmjs.com/package/@picovoice/orca-web)
[![CocoaPods](https://img.shields.io/cocoapods/v/Orca-iOS)](https://cocoapods.org/pods/Orca-iOS)<!-- markdown-link-check-disable-line -->
[![PyPI](https://img.shields.io/pypi/v/pvorca)](https://pypi.org/project/pvorca/)

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

[![Twitter URL](https://img.shields.io/twitter/url?label=%40AiPicovoice&style=social&url=https%3A%2F%2Ftwitter.com%2FAiPicovoice)](https://twitter.com/AiPicovoice)<!-- markdown-link-check-disable-line -->
[![YouTube Channel Views](https://img.shields.io/youtube/channel/views/UCAdi9sTCXLosG1XeqDwLx7w?label=YouTube&style=social)](https://www.youtube.com/channel/UCAdi9sTCXLosG1XeqDwLx7w)

Orca is an on-device streaming text-to-speech engine that is designed for use with LLMs, enabling zero-latency
voice assistants. Orca is:

- Private; All voice processing runs locally.
- Cross-Platform:
    - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64)
    - Android and iOS
    - Raspberry Pi (5, 4, 3) and NVIDIA Jetson Nano
    - Chrome, Safari, Firefox, and Edge

**Please note that Orca is currently in development. While we prioritize stability and compatibility, certain aspects of
Orca may undergo changes as we continually enhance and refine the engine to provide the best user experience possible.**

## Table of Contents

- [Orca](#orca)
    - [Table of Contents](#table-of-contents)
    - [Overview](#overview)
        - [Orca streaming text synthesis](#orca-input-and-output-streaming-synthesis)
        - [Text input](#text-input)
        - [Custom pronunciations](#custom-pronunciations)
        - [Voices](#voices)
        - [Speech control](#speech-control)
        - [Audio output](#audio-output)
    - [AccessKey](#accesskey)
    - [Demos](#demos)
        - [Python Demos](#python-demos)
        - [iOS Demo](#ios-demo)
        - [C Demos](#c-demos)
        - [Web Demos](#web-demos)
        - [Android Demo](#android-demo)
        - [Node.js Demo](#nodejs-demos)
    - [SDKs](#sdks)
        - [Python](#python)
        - [iOS](#ios)
        - [C](#c)
        - [Web](#web)
        - [Android](#android)
        - [Node.js](#nodejs)
    - [Releases](#releases)
    - [FAQ](#faq)

## Language Support

- Orca Text-to-Speech currently supports English only.
- Support for [additional languages is available for commercial customers](https://picovoice.ai/consulting/) on a
  case-by-case basis.

## Overview

### Orca input and output streaming synthesis

Orca is a text-to-speech engine designed specifically for LLMs. It can process
incoming text streams in real-time, generating audio continuously, i.e., as the LLM produces tokens,
Orca generates speech in parallel.
This enables seamless conversations with voice assistants, eliminating any audio delays.

![](https://github.com/Picovoice/orca/blob/main/resources/assets/orca_streaming_animation.gif)

Orca also supports single synthesis mode, where a complete text is synthesized in a single call to the Orca engine.

### Text input

Orca accepts the 26 lowercase (a-z) and 26 uppercase (A-Z) letters of the English alphabet, numbers,
basic symbols, as well as common punctuation marks. You can get a list of all supported characters by calling the
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

|  Parameter   | Default |                                                                        Description                                                                        |
|:------------:|:-------:|:---------------------------------------------------------------------------------------------------------------------------------------------------------:|
| speech rate  |   1.0   |                Speed of generated speech. Valid values are within [0.7, 1.3]. <br/>Higher (lower) values generate faster (slower) speech.                 |
| random state | random  | Sets the random state for sampling during synthesis. <br/>Valid values are all non-negative integers. <br/>If not provided, a random seed will be chosen. |

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

Install the demo package:

```console
pip3 install pvorcademo
```

Run the streaming demo:

```console
orca_demo_streaming --access_key ${ACCESS_KEY} --text_to_stream ${TEXT}
```

Run the single synthesis demo:

```console
orca_demo --access_key ${ACCESS_KEY} --text ${TEXT} --output_path ${WAV_OUTPUT_PATH}
```

Replace `${ACCESS_KEY}` with yours obtained from Picovoice Console, `${TEXT}` with the text to be synthesized, and
`${WAV_OUTPUT_PATH}` with a path to an output WAV file.

For more information about Python demos go to [demo/python](demo/python).

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

Build the streaming demo:

```console
cmake -S demo/c/ -B demo/c/build && cmake --build demo/c/build --target orca_demo_streaming
```

Run the demo:

```console
./demo/c/build/orca_demo_streaming -l ${LIBRARY_PATH} -m ${MODEL_PATH} -a ${ACCESS_KEY} -t ${TEXT} -o ${OUTPUT_PATH}
```

### Web Demos

From [demo/web](./demo/web) run the following in the terminal:

```console
yarn
yarn start
```

(or)

```console
npm install
npm run start
```

Open `http://localhost:5000` in your browser to try the demo.

### Android Demo

Using Android Studio, open [demo/android/OrcaDemo](./demo/android/OrcaDemo) as an Android project and then run the
application.

Replace `"${YOUR_ACCESS_KEY_HERE}"` in the
file [MainActivity.java](./demo/android/OrcaDemo/orca-demo-app/src/main/java/ai/picovoice/orcademo/MainActivity.java)
with your `AccessKey`.

### Node.js Demos

Install the demo package:

```console
yarn global add @picovoice/orca-node-demo
```

Run the streaming demo:

```console
orca-streaming-demo --access_key ${ACCESS_KEY} --text_to_stream ${TEXT}
```

Run the file demo:

```console
orca-file-demo --access_key ${ACCESS_KEY} --text ${TEXT} --output_path ${AUDIO_PATH}
```

Replace `${ACCESS_KEY}` with yours obtained from Picovoice Console, `${TEXT}` with the text to be synthesized, and
`${AUDIO_PATH}` with a path to an output WAV file.

For more information about Node.js demos go to [demo/nodejs](./demo/nodejs).

## SDKs

### Python

Install the Python SDK:

```console
pip3 install pvorca
```

Create an instance of the engine:

```python
import pvorca

orca = pvorca.create(access_key='${ACCESS_KEY}')
```

Replace `${ACCESS_KEY}` with yours obtained from [Picovoice Console](https://console.picovoice.ai/).

#### Streaming synthesis

To synthesize a text stream, create an Orca Stream object and add text to it one-by-one:

```python
stream = orca.stream_open()

for text_chunk in text_generator():
    pcm = stream.synthesize(text_chunk)
    if pcm is not None:
        # handle pcm

pcm = stream.flush()
if pcm is not None:
    # handle pcm
```

The `text_generator()` function can be any stream generating text, for example an LLM response.
When done with streaming text synthesis, the stream object needs to be closed:

```python
stream.close()
```

#### Single synthesis

Use single synthesis mode if the complete text is known in advance:

```python
pcm, alignments = orca.synthesize('${TEXT}')
```

Replace `${TEXT}` with the text to be synthesized including potential [custom pronunciations](#custom-pronunciations).

Finally, when done make sure to explicitly release the resources:

```python
orca.delete()
```

For more details see [Python SDK](./binding/python/README.md).

### iOS

Create an instance of the engine:

```swift
import Orca

let modelPath = Bundle(for: type(of: self)).path(
        forResource: "${MODEL_FILE}", // Name of the model file name for Orca
        ofType: "pv")!

do {
  let orca = try Orca(accessKey: "${ACCESS_KEY}", modelPath: modelPath)
} catch {}
```

Replace `${ACCESS_KEY}` with yours obtained from [Picovoice Console](https://console.picovoice.ai/) and `${MODEL_FILE}`
with the model file name for Orca.

#### Streaming synthesis

To synthesize a text stream, create an `OrcaStream` object and add text to it one-by-one via the `synthesize` method.
Call `flush` to synthesize any remaining text, and `close` to delete the `OrcaStream` object:

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

orcaStream.close()
```

`textGenerator()` can be any stream generating text, for example an LLM response.

#### Single synthesis

```swift
do {
    let (pcm, wordArray) = try orca.synthesize(text: "${TEXT}")
} catch {}
```

Replace `${TEXT}` with the text to be synthesized including potential [custom pronunciations](#custom-pronunciations).

#### Release resources

When done be sure to explicitly release the resources using `orca.delete()`.

For more details, see the [iOS SDK](./binding/ios/).

### C

The header file [include/pv_orca.h](./include/pv_orca.h) contains relevant information on Orca's C SDK.

Build an instance of the object:

```c
pv_orca_t *orca = NULL;
const char *model_path = "${MODEL_PATH}";
pv_status_t status = pv_orca_init("${ACCESS_KEY}", model_path, &orca);
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

#### Streaming synthesis

To synthesize a text stream, create an `orca_stream` object using `synthesize_params`:

```c
pv_orca_stream_t *orca_stream = NULL;
status = pv_orca_stream_open(orca, synthesize_params, &orca_stream);
if (status != PV_STATUS_SUCCESS) {
    // error handling logic
}
```

Add text to `orca_stream` one-by-one and handle the synthesized audio:

```c
extern char *get_next_text_chunk(void);

int32_t num_samples_chunk = 0;
int16_t *pcm_chunk = NULL;
status = pv_orca_stream_synthesize(
    orca_stream, 
    get_next_text_chunk(), 
    &num_samples_chunk, 
    &pcm_chunk);
if (status != PV_STATUS_SUCCESS) {
    // error handling logic
}
if (num_samples_chunk > 0) {
    // handle pcm_chunk
}
```

Once the text stream is complete, call the flush method to synthesize the remaining text:

```c
status = pv_orca_stream_flush(orca_stream, &num_samples_chunk, &pcm_chunk);
if (status != PV_STATUS_SUCCESS) {
    // error handling logic
}
if (num_samples_chunk > 0) {
    // handle pcm_chunk
}
```

Once the PCM chunks are handled, make sure to release the acquired resources for each chunk with:

```c
pv_orca_pcm_delete(pcm_chunk);
```

Finally, when done make sure to close the stream:

```c
pv_orca_stream_close(orca_stream);
```

#### Single synthesis

If the text is known in advance, single synthesis mode can be used:

```c
int32_t num_samples = 0;
int16_t *synthesized_pcm = NULL;
int32_t num_alignments = 0;
pv_orca_word_alignment_t **alignments = NULL;
status = pv_orca_synthesize(
    orca,
    "${TEXT}",
    synthesize_params,
    &num_samples,
    &synthesized_pcm,
    &num_alignments,
    &alignments);
```

Replace `${TEXT}` with the text to be synthesized including potential [custom pronunciations](#custom-pronunciations).

Print the metadata of the synthesized audio:

```c
for (int32_t i = 0; i < num_alignments; i++) {
    fprintf(
            stdout,
            "[%s]\t.start_sec = %.2f .end_sec = %.2f\n",
            alignments[i].word,
            alignments[i].start_sec,
            alignments[i].end_sec);
    for (int32_t j = 0; j < alignments[i].num_phonemes; j++) {
        fprintf(
                stdout,
                "\t[%s]\t.start_sec = %.2f .end_sec = %.2f\n",
                alignments[i].phonemes[j].phoneme,
                alignments[i].phonemes[j].start_sec,
                alignments[i].phonemes[j].end_sec);
    
    }
}
```

Finally, when done make sure to release the acquired resources:

```c
pv_orca_word_alignments_delete(num_alignments, alignments);
pv_orca_pcm_delete(pcm);
pv_orca_synthesize_params_delete(synthesize_params);
pv_orca_delete(orca);
```

### Web

Install the web SDK using yarn:

```console
yarn add @picovoice/orca-web
```

or using npm:

```console
npm install --save @picovoice/orca-web
```

Create an instance of the engine using `OrcaWorker` and synthesize speech:

```typescript
import { OrcaWorker } from "@picovoice/orca-web";
import orcaParams from "${PATH_TO_BASE64_ORCA_PARAMS}";

const orca = await OrcaWorker.create(
  "${ACCESS_KEY}",
  { base64: orcaParams }
);
```

Replace `${ACCESS_KEY}` with yours obtained from [Picovoice Console](https://console.picovoice.ai/).

#### Streaming synthesis

To synthesize a text stream, create an `OrcaStream` object and add text to it one-by-one via the `synthesize` method.
Call `flush` to synthesize any remaining text, and `close` to delete the `OrcaStream` object:

```typescript
const orcaStream = await orca.streamOpen();

function* textStream(): IterableIterator<string> {
... // yield text chunks e.g. from an LLM response
}

for (const textChunk of textStream()) {
  const pcm = await orcaStream.synthesize(textChunk);
  if (pcm !== null) {
    // handle pcm
  }
}

const flushedPcm = await orcaStream.flush();
if (flushedPcm !== null) {
  // handle pcm
}

orcaStream.close();
```

#### Single synthesis

```typescript
const { pcm, alignments } = await orca.synthesize("${TEXT}")
```

Finally, when done release the resources using `orca.release()`.

For more details, see the [Web SDK](./binding/web/).

### Android

To include the Orca package in your Android project, ensure you have included `mavenCentral()` in your
top-level `build.gradle` file and then add the following to your app's `build.gradle`:

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
} catch (OrcaException ex) { }
```

Replace `${ACCESS_KEY}` with yours obtained from Picovoice Console, `${MODEL_FILE_PATH}` with an
Orca [voice model file](./lib/common) and `${TEXT}` with the text to be synthesized including
potential [custom pronunciations](#custom-pronunciations).

#### Streaming synthesis

To synthesize a text stream, create an `OrcaStream` object and add text to it one-by-one via the `synthesize`.
Call `flush` to synthesize any remaining text, and `close` to delete the `OrcaStream` object:

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

#### Single synthesis

```java
OrcaAudio audio = orca.synthesize(
    "${TEXT}",
    new OrcaSynthesizeParams.Builder().build());
```

Finally, when done make sure to explicitly release the resources:

```java
orca.delete()
```

For more details, see the [Android SDK](./binding/android/README.md).

### Node.js

Install the Node.js SDK:

```console
yarn add @picovoice/orca-node
```

Create instances of the Orca class:

```typescript
const Orca = require("@picovoice/orca-node");
const accessKey = "${ACCESS_KEY}" // Obtained from the Picovoice Console (https://console.picovoice.ai/)
let orca = new Orca(accessKey);
```

Replace `${ACCESS_KEY}` with yours obtained from [Picovoice Console](https://console.picovoice.ai/).

#### Streaming synthesis

To synthesize a text stream, create an `OrcaStream` object and add text to it one-by-one via the `synthesize` method.
Call `flush` to synthesize any remaining text, and `close` to delete the `OrcaStream` object:

```typescript
const orcaStream = orca.streamOpen();

function* textStream(): IterableIterator<string> {
  // ... yield text chunks e.g. from an LLM response
}

for (const textChunk of textStream()) {
  const pcm = orcaStream.synthesize(textChunk);
  if (pcm !== null) {
    // handle pcm
  }
}

const flushedPcm = orcaStream.flush();
if (flushedPcm !== null) {
  // handle pcm
}

orcaStream.close();
```

#### Single synthesis

```typescript
const { pcm, alignments } = orca.synthesize("${TEXT}")
```

Finally, when done release the resources using `orca.release()`.

For more details, see the [Node.js SDK](./binding/nodejs/).

## Releases

### v0.2.0 - May 3rd, 2024

- Support for streaming synthesis
- Reduced model size
- Improved performance
- Support for word alignments
- Improved naturalness and pronunciations

### v0.1.0 - January 24th, 2024

- Beta release

## FAQ

You can find the FAQ [here](https://picovoice.ai/docs/faq/general/).
