# Orca Binding for Python

## Orca Text-to-Speech Engine

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

Orca is an on-device text-to-speech engine that is designed for use with LLMs, enabling zero-latency voice assistants.
Orca is:

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

Orca supports two modes of operation: streaming and single synthesis.
In the streaming synthesis mode, Orca processes an incoming text stream in real-time and generates audio in parallel.
In the single synthesis mode, the complete text needs to be known in advance and is synthesized in a single call to the Orca engine.

Create an instance of the Orca engine:

```python
import pvorca

orca = pvorca.create(access_key='${ACCESS_KEY}')
```

Replace the `${ACCESS_KEY}` with your AccessKey obtained from [Picovoice Console](https://console.picovoice.ai/).

To synthesize a text stream, create an `Orca.Stream` object and add text to it one-by-one:

```python
stream = orca.open_stream()

for text_chunk in text_generator():
    pcm = stream.synthesize(text_chunk)
    if pcm is not None:
        # handle pcm

pcm = stream.flush()
if pcm is not None:
    # handle pcm
```

The `text_generator()` function can be any stream generating text, for example an LLM response.
Orca produces audio chunks in parallel to the LLM, and returns the raw PCM whenever enough context has been added via `stream.synthesize()`.
The `stream.synthesize()` function returns an audio chunk that only includes the audio for a portion of the text that has been added.
To generate the audio for the remaining text, `stream.flush()` needs to be invoked.
When done with streaming text synthesis, the `Orca.Stream` object needs to be closed:

```python
stream.close()
```

If the complete text is known before synthesis, single synthesis mode can be used to generate speech in a single call to Orca:

```python
# Return raw PCM
pcm, alignments = orca.synthesize(text='${TEXT}')

# Save the generated audio to a WAV file directly
alignments = orca.synthesize_to_file(text='${TEXT}', path='${OUTPUT_PATH}')
```

Replace `${TEXT}` with the text to be synthesized and `${OUTPUT_PATH}` with the path to save the generated audio as a
single-channel 16-bit PCM WAV file.
In single synthesis mode, Orca returns metadata of the synthesized audio in the form of a list of `Orca.WordAlignment` objects. 
To print the metadata run:

```python
for word in alignments:
    print(f"word=\"{word.word}\", start_sec={word.start_sec:.2f}, end_sec={word.end_sec:.2f}")
    for phoneme in word.phonemes:
        print(f"\tphoneme=\"{phoneme.phoneme}\", start_sec={phoneme.start_sec:.2f}, end_sec={phoneme.end_sec:.2f}")
```

When done make sure to explicitly release the resources with:

```python
orca.delete()
```

### Text input

Orca accepts the 26 lowercase (a-z) and 26 uppercase (A-Z) letters of the English alphabet, numbers,
basic symbols, as well as common punctuation marks. You can get a list of all supported characters by calling the
`valid_characters()` method provided in the Orca SDK you are using.
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

```python
orca = pvorca.create(access_key='${ACCESS_KEY}', model_path='${MODEL_PATH}')
```

and replace `${MODEL_PATH}` with the path to the model file with the desired voice.

### Speech control

Orca allows for keyword arguments to be provided to the `open_stream` method or the single `synthesize` methods to control the synthesized speech:

- `speech_rate`: Controls the speed of the generated speech. Valid values are within [0.7, 1.3]. A higher (lower) value
  produces speech that is faster (slower). The default is `1.0`.

### Orca properties

To obtain the set of valid characters, call `orca.valid_characters`.\
To retrieve the maximum number of characters allowed, call `orca.max_character_limit`.\
The sample rate of Orca is `orca.sample_rate`.

### Alignment Metadata

Along with the raw PCM or saved audio file, Orca returns metadata for the synthesized audio in single synthesis mode.
The `Orca.WordAlignment` object has the following properties:

- **Word:** String representation of the word.
- **Start Time:** Indicates when the word started in the synthesized audio. Value is in seconds.
- **End Time:** Indicates when the word ended in the synthesized audio. Value is in seconds.
- **Phonemes:** A list of `Orca.PhonemeAlignment` objects.

The `Orca.PhonemeAlignment` object has the following properties:

- **Phoneme:** String representation of the phoneme.
- **Start Time:** Indicates when the phoneme started in the synthesized audio. Value is in seconds.
- **End Time:** Indicates when the phoneme ended in the synthesized audio. Value is in seconds.

## Demos

[pvorcademo](https://pypi.org/project/pvorcademo/) provides command-line utilities for synthesizing audio using
Orca.

