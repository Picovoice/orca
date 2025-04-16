# Orca Demo

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Setup

Replace `"${YOUR_ACCESS_KEY_HERE}"` inside [MainActivity.java](orca-demo-app/src/main/java/ai/picovoice/orcademo/MainActivity.java) with your AccessKey obtained from [Picovoice Console](https://console.picovoice.ai/).
Replace `"${MODEL_FILE_HERE}"` inside [MainActivity.java](orca-demo-app/src/main/java/ai/picovoice/orcademo/MainActivity.java) with the model file you would like to run the demo with. Available model files are in [lib/common](../../../lib/common).

1. Open the project in Android Studio
2. Build and run on an installed simulator or a connected Android device

## Usage

1. Choose between Streaming Synthesis and Single Synthesis using the switch at the top.
2. Type a phrase that you'd like to synthesize into the textbox.
3. Press the `Synthesize` button to hear the synthesized speech.

