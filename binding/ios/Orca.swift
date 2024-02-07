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

    private var handle: OpaquePointer?

    private var _validPunctuationSymbols: Set<String>?

    /// Required audio sample rate
    public static let sampleRate = UInt32(pv_sample_rate())

    /// Orca version string
    public static let version = String(cString: pv_orca_version())

    /// Maximum number of characters allowed in a single synthesis request.
    public static let maxCharacterLimit = Uint32(pv_orca_max_character_limit())

    private static var sdk = "ios"

    public static func setSdk(sdk: String) {
        self.sdk = sdk
    }

    var validPunctuationSymbols: Set<String> {
        get {
            if _validPunctuationSymbols == nil {
                _validPunctuationSymbols = getValidPunctuationSymbols()
            }
            return _validPunctuationSymbols
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
        modelPath: String) throws {

        var modelPathArg = modelPath
        if !FileManager().fileExists(atPath: modelPathArg) {
            modelPathArg = try getResourcePath(modelPathArg)
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
    /// - Returns: The generated audio, stored as a sequence of 16-bit linearly-encoded integers.
    /// - Throws: OrcaError
    public func synthesize(text: String, speechRate: Float32) throws -> [Int16] {
        if handle == nil {
            throw OrcaInvalidStateError("Unable to synthesize - resources have been released")
        }

        if text.count > Orca.maxCharacterLimit {
            throw OrcaInvalidArgumentError(
                "Text length (\(text.count)) must be smaller than \(Orca.maxCharacterLimit)")
        }

        var cSynthesizeParams = getCSynthesizeParams(speechRate: speechRate)

        var cNumSamples: Int32 = 0
        var cPcm: UnsafeMutablePointer<Int16>?
        let status = pv_orca_synthesize(handle, text, cSynthesizeParams, &cNumSamples, &cPcm)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to synthesize speech", messageStack)
        }

        let buffer = UnsafeBufferPointer(start: cPcm, count: cNumSamples)
        let pcm = Array(buffer)

        pv_orca_delete_pcm(cPcm)
        pv_orca_synthesize_params_delete(cSynthesizeParams)

        return pcm
    }

    private func getCSynthesizeParams(speechRate: Float32? = nil) throws -> UnsafeMutablePointer<pv_orca_synthesize_params_t>? {
        var cParams: UnsafeMutablePointer<pv_orca_synthesize_params_t>?

        var status = pv_orca_synthesize_params_init(&cParams)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to create Orca synthesize params object", messageStack)
        }

        if speechRate != nil {
            status = pv_orca_synthesize_params_set_speech_rate(&cParams, speechRate)
            if status != PV_STATUS_SUCCESS {
                let messageStack = try getMessageStack()
                throw pvStatusToOrcaError(status, "Unable to set Orca speech rate", messageStack)
            }
        }

        return cParams
    }

    private func getValidPunctuationSymbols() throws -> Set<String> {
        if handle == nil {
            throw OrcaInvalidStateError("Unable to get punctuation symbols - resources have been released")
        }

        var cNumSymbols: Int32 = 0
        var cSymbols: UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>?
        let status = pv_orca_valid_punctuation_symbols(handle, &cNumSymbols, &cSymbols)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to get Orca valid punctuation symbols", messageStack)
        }

        var symbols: [String] = []
        for i in 0..<cSymbols {
            symbols.append(String(cString: cSymbols!.advanced(by: Int(i)).pointee!))
        }

        pv_orca_valid_punctuation_symbols_delete(cSymbols)

        return symbols
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
