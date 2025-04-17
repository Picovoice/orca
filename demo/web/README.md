# orca-web-demo

This is a basic demo to show how to use Orca for web browsers, using the IIFE version of the library (i.e. an HTML
script tag). It instantiates an Orca worker engine and uses it to perform text-to-speech.

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Install & run

Use `yarn` or `npm` to install the dependencies, and the `start` script to start a local web server hosting the demo.

```console
yarn
yarn start ${LANGUAGE} ${GENDER}
```

(or)

```console
npm install
npm run start ${LANGUAGE} ${GENDER}
```

Replace `${LANGUAGE}` and `${GENDER}` with the language and gender you would like to run the demo in. Available languages are `en`, `es`, `de`, `fr`, `ko`, `ja`, `it`, `pt`, and available genders are `male` and `female`.

Open `localhost:5000` in your web browser, as hinted at in the output:

```console
Available on:
  http://localhost:5000
Hit CTRL-C to stop the server
```

Copy in your AccessKey from Picovoice Console, and click "Start Orca".

## Usage

Orca supports two modes of operation: streaming and single synthesis.
In the streaming synthesis mode, Orca processes an incoming text stream in real-time and generates audio in parallel.
In the single synthesis mode, the complete text needs to be known in advance and is synthesized in a single call to
the Orca engine.

Click on either "Streaming Synthesis" or "Single Synthesis" to continue.

### Streaming Synthesis

1. Choose desired speech rate (or keep the default)
2. Click "Open Stream"
3. Type in any text (in English only).
4. When you're done, click "Run Streaming Synthesis" to run streaming synthesis on a simulated text stream.

### Single Synthesis

1. Type in any text (in English only)
2. Change the speech rate (or keep the default)
3. Click "Synthesize"
4. Click "Play" and listen for the generated speech.

**Optional**: If you wish, you may replace the model file in the `index.html` with the male model file for a male
voice:

```html
<script type="application/javascript" src="orca_params_en_male.js"></script>
```
