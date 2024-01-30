# Orca Binding for Python

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

- Python 3.7+
- Runs on Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64), Raspberry Pi (5, 4, 3), and NVIDIA Jetson Nano.

## Installation

```console
pip3 install pvorca
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs. You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Usage

Create an instance of the Orca engine:

```python
import pvorca

orca = pvorca.create(access_key='${ACCESS_KEY}')
```

Replace the `${ACCESS_KEY}` with your AccessKey obtained from [Picovoice Console](https://console.picovoice.ai/).

You can synthesize speech by calling one of the `synthesize` methods:

```python
# Return raw PCM
pcm = orca.synthesize(text='${TEXT}')

# Save the generated audio to a WAV file directly
orca.synthesize_to_file(text='${TEXT}', path='${OUTPUT_PATH}')
```

Replace `${TEXT}` with the text to be synthesized and `${OUTPUT_PATH}` with the path to save the generated audio as a
single-channel 16-bit PCM WAV file.\
When done make sure to explicitly release the resources with `orca.delete()`.

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

```python
orca = pvorca.create(access_key='${ACCESS_KEY}', model_path='${MODEL_PATH}')
```

and replace `${MODEL_PATH}` with the path to the model file with the desired voice.

### Speech control

Orca allows for keyword arguments to be provided to the `synthesize` methods to control the synthesized speech:

- `speech_rate`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher (lower) value
  produces speech that is faster (slower). The default is `1.0`.

### Orca properties

To obtain the set of valid punctuation symbols, call `orca.valid_punctuation_symbols`.\
To retrieve the maximum number of characters allowed, call `orca.max_character_limit`.\
The sample rate of Orca is `orca.sample_rate`.

## Demos

[pvorcademo](https://pypi.org/project/pvorcademo/) provides command-line utilities for synthesizing audio using
Orca.

