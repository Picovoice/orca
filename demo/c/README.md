# C Demo

## Compatibility

You need a C99-compatible compiler to build these demos.

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Requirements

- The demo requires [CMake](https://cmake.org/) version 3.4 or higher.
- **For Windows Only**: [MinGW](https://www.mingw-w64.org/) is required to build the demo.

# Speech Synthesis Demos

Orca supports two modes of operation: streaming and single synthesis.
In the streaming synthesis mode, Orca processes an incoming text stream in real-time and generates audio in parallel.
This is demonstrated in the Orca streaming demo.
In the single synthesis mode, the text is synthesized in a single call to the Orca engine.

**Note**: the following commands are run from the root of the repo.

## Streaming Synthesis Demo

### Build

Use CMake to build the Orca demo target:

```console
cmake -S demo/c/ -B demo/c/build && cmake --build demo/c/build --target orca_demo_streaming
```

### Usage

Running the executable without any command-line arguments prints the usage info to the console:

```console
Usage: orca_demo_streaming [-l LIBRARY_PATH -m MODEL_PATH -a ACCESS_KEY -t TEXT -o OUTPUT_PATH]
```

To run the Orca streaming demo:

```console
./demo/c/build/orca_demo_streaming -l ${LIBRARY_PATH} -m ${MODEL_PATH} -a ${ACCESS_KEY} -t ${TEXT} -o ${OUTPUT_PATH}
```

Replace `${LIBRARY_PATH}` with the path to appropriate library available under [lib](../../lib), `${MODEL_PATH}` with
a path to any of the model files available under [lib/common](../../lib/common), `${ACCESS_KEY}` with AccessKey
obtained from [Picovoice Console](https://console.picovoice.ai/), `${TEXT}` with the text to be synthesized,
and `${WAV_OUTPUT_PATH}` with a path to a output audio file.
The audio will be stored as a single-channel 16-bit PCM `.wav` file.

## Single Synthesis Demo

### Build

Use CMake to build the Orca demo target:

```console
cmake -S demo/c/ -B demo/c/build && cmake --build demo/c/build --target orca_demo
```

### Usage

To run the Orca demo:

```console
./demo/c/build/orca_demo -l ${LIBRARY_PATH} -m ${MODEL_PATH} -a ${ACCESS_KEY} -t ${TEXT} -o ${OUTPUT_PATH}
```

Replace `${LIBRARY_PATH}` with the path to appropriate library available under [lib](../../lib), `${MODEL_PATH}` with
a path to any of the model files available under [lib/common](../../lib/common), `${ACCESS_KEY}` with AccessKey
obtained from [Picovoice Console](https://console.picovoice.ai/), `${TEXT}` with the text to be synthesized,
and `${WAV_OUTPUT_PATH}` with a path to a output audio file.
The audio will be stored as a single-channel 16-bit PCM `.wav` file.
