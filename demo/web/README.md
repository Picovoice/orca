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
yarn start
```

(or)

```console
npm install
npm run start
```

Open `localhost:5000` in your web browser, as hinted at in the output:

```console
Available on:
  http://localhost:5000
Hit CTRL-C to stop the server
```

Wait until Orca has initialized. Type in any text (in English only), and optionally select a desired speech rate. Click
synthesize, and once Orca has finished synthesizing your text, click play and listen for the speech.

**Optional**: If you wish, you may replace the model file in the `index.html` with a male model file for a male
voice:

```html

<script type="application/javascript" src="orca_params_male.js"></script>
```
