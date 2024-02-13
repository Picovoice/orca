//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

import PvOrca

/// iOS (Swift) binding for Orca Text-to-Speech engine. Provides a Swift interface to the Cheetah library.
public class Orca {

    static let resourceBundle: Bundle = {
        let myBundle = Bundle(for: Orca.self)

        guard let resourceBundleURL = myBundle.url(
             forResource: "OrcaResources", withExtension: "bundle")
        else { fatalError("OrcaResources.bundle not found") }

        guard let resourceBundle = Bundle(url: resourceBundleURL)
            else { fatalError("Could not open OrcaResources.bundle") }

        return resourceBundle
    }()

    private var handle: OpaquePointer?
    /// Orca valid symbols
    private var _validCharacters: Set<String>?
    /// Orca sample rate
    private var _sampleRate: Int32?
    /// Maximum number of characters allowed in a single synthesis request.
    public static let maxCharacterLimit = Int32(pv_orca_max_character_limit())
    /// Orca version string
    public static let version = String(cString: pv_orca_version())

    private static var sdk = "ios"

    public static func setSdk(sdk: String) {
        self.sdk = sdk
    }

    public var validCharacters: Set<String> {
        get throws {
            if _validCharacters == nil {
                _validCharacters = try getValidChracters()
            }
            return _validCharacters!
        }
    }

    public var sampleRate: Int32 {
        get throws {
            if _sampleRate == nil {
                var cSampleRate: Int32 = 0
                let status = pv_orca_sample_rate(handle, &cSampleRate)
                if status != PV_STATUS_SUCCESS {
                    let messageStack = try getMessageStack()
                    throw pvStatusToOrcaError(status, "Orca failed to get sample rate", messageStack)
                }

                _sampleRate = cSampleRate
            }

            return _sampleRate!
        }
    }

    /// Constructor.
    ///
    /// - Parameters:
    ///   - accessKey: AccessKey obtained from the Picovoice Console (https://console.picovoice.ai/)
    ///   - modelPath: Absolute path to file containing model parameters.
    /// - Throws: OrcaError
    public init(
        accessKey: String,
        modelPath: String? = nil) throws {

        var modelPathArg = modelPath

        if modelPath == nil {
            modelPathArg  = Orca.resourceBundle.path(forResource: "orca_params_female", ofType: "pv")
            if modelPathArg == nil {
                throw OrcaIOError("Unable to find the default model path")
            }
        }

        if !FileManager().fileExists(atPath: modelPathArg!) {
            modelPathArg = try getResourcePath(modelPathArg!)
        }

        pv_set_sdk(Orca.sdk)

        let status = pv_orca_init(accessKey, modelPathArg, &handle)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Orca init failed", messageStack)
        }
    }

    /// Constructor.
    ///
    /// - Parameters:
    ///   - accessKey: The AccessKey obtained from Picovoice Console (https://console.picovoice.ai).
    ///   - modelURL: URL to file containing model parameters.
    /// - Throws: OrcaError
    public convenience init(
            accessKey: String,
            modelURL: URL) throws {

        try self.init(
                accessKey: accessKey,
                modelPath: modelURL.path)
    }

    deinit {
        self.delete()
    }

    /// Releases native resources that were allocated to Orca
    public func delete() {
        if handle != nil {
            pv_orca_delete(handle)
            handle = nil
        }
    }

    /// Generates audio from text. The returned audio contains the speech representation of the text.
    ///
    /// - Parameters:
    ///   - text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is 
    ///     `.maxCharacterLimit`. Allowed characters are lower-case and upper-case letters and punctuation marks
    ///     that can be retrieved with `.validPunctuationSymbols`.
    ///   - speechRate: Rate of speech of the generated audio.
    /// - Returns: The generated audio, stored as a sequence of 16-bit linearly-encoded integers.
    /// - Throws: OrcaError
    public func synthesize(text: String, speechRate: Double? = nil) throws -> [Int16] {
        if handle == nil {
            throw OrcaInvalidStateError("Unable to synthesize - resources have been released")
        }

        if text.count > Orca.maxCharacterLimit {
            throw OrcaInvalidArgumentError(
                "Text length (\(text.count)) must be smaller than \(Orca.maxCharacterLimit)")
        }

        let characters = try self.validCharacters
        let regex = try NSRegularExpression(pattern: "[^\(characters.joined(separator: ""))\\s{}|']", options: .caseInsensitive)
        let range = NSRange(text.startIndex..<text.endIndex, in: text)
        let matches = regex.matches(in: text, range: range)

        let unexpectedCharacters = matches.map {
            String(text[Range($0.range, in: text)!])
        }

        if unexpectedCharacters.count > 0 {
            throw OrcaInvalidArgumentError(
                "Text contains the following invalid characters: `\(unexpectedCharacters.joined(separator: ", "))`")
        }

        let cSynthesizeParams = try getCSynthesizeParams(speechRate: speechRate)

        var cNumSamples: Int32 = 0
        var cPcm: UnsafeMutablePointer<Int16>?
        let status = pv_orca_synthesize(handle, text, cSynthesizeParams, &cNumSamples, &cPcm)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to synthesize speech", messageStack)
        }

        let buffer = UnsafeBufferPointer(start: cPcm, count: Int(cNumSamples))
        let pcm = Array(buffer)

        pv_orca_delete_pcm(cPcm)
        pv_orca_synthesize_params_delete(cSynthesizeParams)

        return pcm
    }
    
    /// Generates audio from text. The returned audio contains the speech representation of the text.
    ///
    /// - Parameters:
    ///   - text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is
    ///     `.maxCharacterLimit`. Allowed characters are lower-case and upper-case letters and punctuation marks
    ///     that can be retrieved with `.validPunctuationSymbols`.
    ///   - outputPath: Absolute path to the output audio file. The output file is saved as `WAV (.wav)` and consists of a single mono channel.
    ///   - speechRate: Rate of speech of the generated audio.
    /// - Throws: OrcaError
    public func synthesizeToFile(text: String, outputPath: String, speechRate: Double? = nil) throws {
        if handle == nil {
            throw OrcaInvalidStateError("Unable to synthesize - resources have been released")
        }

        if text.count > Orca.maxCharacterLimit {
            throw OrcaInvalidArgumentError(
                "Text length (\(text.count)) must be smaller than \(Orca.maxCharacterLimit)")
        }

        let characters = try self.validCharacters
        let regex = try NSRegularExpression(pattern: "[^\(characters.joined(separator: ""))\\s{}|']", options: .caseInsensitive)
        let range = NSRange(text.startIndex..<text.endIndex, in: text)
        let matches = regex.matches(in: text, range: range)

        let unexpectedCharacters = matches.map {
            String(text[Range($0.range, in: text)!])
        }

        if unexpectedCharacters.count > 0 {
            throw OrcaInvalidArgumentError(
                "Text contains the following invalid characters: `\(unexpectedCharacters.joined(separator: ", "))`")
        }

        let cSynthesizeParams = try getCSynthesizeParams(speechRate: speechRate)
        
        let status = pv_orca_synthesize_to_file(handle, text, cSynthesizeParams, outputPath)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to synthesize speech to file", messageStack)
        }
        
        pv_orca_synthesize_params_delete(cSynthesizeParams)
    }
    
    /// Generates audio from text. The returned audio contains the speech representation of the text.
    ///
    /// - Parameters:
    ///   - text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is
    ///     `.maxCharacterLimit`. Allowed characters are lower-case and upper-case letters and punctuation marks
    ///     that can be retrieved with `.validPunctuationSymbols`.
    ///   - outputURL: URL to the output audio file. The output file is saved as `WAV (.wav)` and consists of a single mono channel.
    ///   - speechRate: Rate of speech of the generated audio.
    /// - Throws: OrcaError
    public func synthesizeToFile(text: String, outputURL: URL, speechRate: Double? = nil) throws {
        try synthesizeToFile(text: text, outputPath: outputURL.path, speechRate: speechRate)
    }

    private func getCSynthesizeParams(speechRate: Double? = nil) throws -> OpaquePointer? {
        var cParams: OpaquePointer?

        var status = pv_orca_synthesize_params_init(&cParams)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to create Orca synthesize params object", messageStack)
        }

        if speechRate != nil {
            status = pv_orca_synthesize_params_set_speech_rate(cParams, Float(speechRate!))
            if status != PV_STATUS_SUCCESS {
                let messageStack = try getMessageStack()
                throw pvStatusToOrcaError(status, "Unable to set Orca speech rate", messageStack)
            }
        }

        return cParams
    }

    private func getValidChracters() throws -> Set<String> {
        if handle == nil {
            throw OrcaInvalidStateError("Unable to get valid characters - resources have been released")
        }

        var cNumCharacters: Int32 = 0
        var cCharacters: UnsafePointer<UnsafePointer<Int8>?>?
        let status = pv_orca_valid_characters(handle, &cNumCharacters, &cCharacters)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to get Orca valid characters", messageStack)
        }

        var characters: Set<String> = []
        for i in 0..<cNumCharacters {
            characters.insert(String(cString: cCharacters!.advanced(by: Int(i)).pointee!))
        }

        pv_orca_valid_characters_delete(cCharacters)

        return characters
    }

    /// Given a path, return the full path to the resource.
        ///
        /// - Parameters:
        ///   - filePath: relative path of a file in the bundle.
        /// - Throws: OrcaIOError
        /// - Returns: The full path of the resource.
        private func getResourcePath(_ filePath: String) throws -> String {
            if let resourcePath = Bundle(for: type(of: self)).resourceURL?.appendingPathComponent(filePath).path {
                if FileManager.default.fileExists(atPath: resourcePath) {
                    return resourcePath
                }
            }

            throw OrcaIOError("Could not find file at path '\(filePath)'. " +
                "If this is a packaged asset, ensure you have added it to your xcode project.")
        }

    private func pvStatusToOrcaError(
        _ status: pv_status_t,
        _ message: String,
        _ messageStack: [String] = []) -> OrcaError {
        switch status {
        case PV_STATUS_OUT_OF_MEMORY:
            return OrcaMemoryError(message, messageStack)
        case PV_STATUS_IO_ERROR:
            return OrcaIOError(message, messageStack)
        case PV_STATUS_INVALID_ARGUMENT:
            return OrcaInvalidArgumentError(message, messageStack)
        case PV_STATUS_STOP_ITERATION:
            return OrcaStopIterationError(message, messageStack)
        case PV_STATUS_KEY_ERROR:
            return OrcaKeyError(message, messageStack)
        case PV_STATUS_INVALID_STATE:
            return OrcaInvalidStateError(message, messageStack)
        case PV_STATUS_RUNTIME_ERROR:
            return OrcaRuntimeError(message, messageStack)
        case PV_STATUS_ACTIVATION_ERROR:
            return OrcaActivationError(message, messageStack)
        case PV_STATUS_ACTIVATION_LIMIT_REACHED:
            return OrcaActivationLimitError(message, messageStack)
        case PV_STATUS_ACTIVATION_THROTTLED:
            return OrcaActivationThrottledError(message, messageStack)
        case PV_STATUS_ACTIVATION_REFUSED:
            return OrcaActivationRefusedError(message, messageStack)
        default:
            let pvStatusString = String(cString: pv_status_to_string(status))
                return OrcaError("\(pvStatusString): \(message)", messageStack)
        }
    }

    private func getMessageStack() throws -> [String] {
        var messageStackRef: UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>?
        var messageStackDepth: Int32 = 0
        let status = pv_get_error_stack(&messageStackRef, &messageStackDepth)
        if status != PV_STATUS_SUCCESS {
            throw pvStatusToOrcaError(status, "Unable to get Orca error state")
        }

        var messageStack: [String] = []
        for i in 0..<messageStackDepth {
            messageStack.append(String(cString: messageStackRef!.advanced(by: Int(i)).pointee!))
        }

        pv_free_error_stack(messageStackRef)

        return messageStack
    }
}
