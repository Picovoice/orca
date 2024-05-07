# Orca Voice Assistant Demo - Talk to ChatGPT in Real-Time

Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

This demo showcases how [Orca Streaming Text-to-Speech](https://picovoice.ai/platform/orca/) can be seamlessly integrated into LLM-applications to drastically reduce the audio latency
of voice assistants.

## Technologies

In this demo, the user can interact with a voice assistant in real-time by leveraging GenAI technologies.
It is built like the majority of voice assistant today, by chaining together a Speech-to-Text engine, an LLM, and
a Text-to-Speech engine.

The following technologies are used:

- Speech-to-Text: Picovoice's [Cheetah Streaming Speech-to-Text](https://picovoice.ai/platform/cheetah/)
- LLM: \"ChatGPT\" using `gpt-3.5-turbo`
  with OpenAI Chat Completion API.
- TTS:
    - Picovoice's [Orca Streaming Text-to-Speech](https://picovoice.ai/platform/orca/)
    - OpenAI TTS
  
## Compatibility

This demo has been tested on Linux (x86_64) and macOS (x86_64) using Python 3.10.

## Access Keys

To run all features of this demo, access keys are required for:

- Picovoice Console: Get your `AccessKey` for free by signing up or logging in
  to [Picovoice Console](https://console.picovoice.ai/).
- OpenAI API: Get your `AccessKey` from OpenAI.

## Usage

```bash
python orca_voice_assistant_demo.py --picovoice-access-key ${PV_ACCESS_KEY} --openai-access-key ${OPEN_AI_KEY}
```

Replace `${PV_ACCESS_KEY}` with your `AccessKey` obtained from Picovoice Console,
`${OPEN_AI_KEY}` with your `AccessKey` obtained from OpenAI.
You can toggle between Orca and OpenAI TTS by using the `--tts` flag, using `picovoice_orca` or `openai`, respectively.
If you don't want to use ChatGPT, set the `--llm` flag to `dummy`.
This will simulate an LLM response using example sentences that are synthesized by the TTS system.
