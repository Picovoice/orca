# Orca iOS Demo

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Setup

Copy your `AccessKey` into the `YOUR_ACCESS_KEY_HERE` variable inside [`ViewModel.swift`](./OrcaDemo/OrcaDemo/ViewModel.swift).

Open [OrcaDemo.xcodeproj](./OrcaDemo/OrcaDemo.xcodeproj/) and run the demo.

## Usage

Launch the demo on a simulator or a physical iOS device.

1. Enter the text you wish to synthesize in the text box area.
2. Press the `Synthesize` button to synthesize the text and play audio.
