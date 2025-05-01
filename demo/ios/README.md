# Orca iOS Demo

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca SDKs.
You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Running the Demo

1. Open [OrcaDemo.xcodeproj](./OrcaDemo/OrcaDemo.xcodeproj/) in XCode.
2. Replace `${YOUR_ACCESS_KEY_HERE}` in the file [`ViewModel.swift`](./OrcaDemo/OrcaDemo/ViewModel.swift) with your `AccessKey`.
3. Go to `Product > Scheme` and select the scheme for the language and gender you would like to run the demo in (e.g. `enFemaleDemo` -> English Demo with a female voice, `deMaleDemo` -> German Demo with a male voice).
4. Run the demo with a simulator or connected iOS device.
5. Once the demo app has started, enter the text you wish to synthesize in the text box area, and press the `Synthesize` button to synthesize the text and play audio.
