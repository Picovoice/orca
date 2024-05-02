Pod::Spec.new do |s|
    s.name = 'Orca-iOS'
    s.module_name = 'Orca'
    s.version = '0.1.0'
    s.license = {:type => 'Apache 2.0'}
    s.summary = 'iOS binding for Picovoice\'s Orca Text-to-Speech Engine.'
    s.description =
    <<-DESC
    Made in Vancouver, Canada by [Picovoice](https://picovoice.ai)

    Orca is an on-device text-to-speech engine producing high-quality, realistic, spoken audio with zero latency. Orca is:
      - Private; All voice processing runs locally.
      - Cross-Platform:
        - Linux (x86_64), macOS (x86_64, arm64), Windows (x86_64)
        - Android and iOS
        - Chrome, Safari, Firefox, and Edge
        - Raspberry Pi (5, 4, 3) and NVIDIA Jetson Nano
    DESC
    s.homepage = 'https://github.com/Picovoice/orca/tree/main/binding/ios'
    s.author = { 'Picovoice' => 'hello@picovoice.ai' }
    s.source = { :git => "https://github.com/Picovoice/orca.git", :branch => "ios-v0.2-update" }
    s.ios.deployment_target = '13.0'
    s.swift_version = '5.0'
    s.vendored_frameworks = 'lib/ios/PvOrca.xcframework'
    s.source_files = 'binding/ios/*.{swift}'
    s.exclude_files = 'binding/ios/OrcaAppTest/**'
  end
