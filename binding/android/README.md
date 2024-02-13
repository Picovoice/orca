# Orca Binding for Android

## Orca

## Compatibility

- Android 5.0+ (API 21+)

## Installation

Orca can be found on Maven Central. To include the package in your Android project, ensure you have included `mavenCentral()` in your top-level `build.gradle` file and then add the following to your app's `build.gradle`:

```groovy
dependencies {
    // ...
    implementation 'ai.picovoice:orca-android:${LATEST_VERSION}'
}
```

## AccessKey

Orca requires a valid Picovoice `AccessKey` at initialization. `AccessKey` acts as your credentials when using Orca
SDKs. You can get your `AccessKey` for free. Make sure to keep your `AccessKey` secret.
Signup or Login to [Picovoice Console](https://console.picovoice.ai/) to get your `AccessKey`.

## Permissions

To enable AccessKey validation, you must add the following line to your `AndroidManifest.xml` file:
```xml
<uses-permission android:name="android.permission.INTERNET" />
```

## Usage


## Demos

For example usage, refer to our [Android demo app](../../demo/android).