//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

import PvOrca

public struct OrcaPhoneme {

    /// Synthesized phoneme.
    public let phoneme: String

    /// Start of phoneme in seconds.
    public let startSec: Float

    /// End of phoneme in seconds.
    public let endSec: Float

    /// Constructor.
    ///
    /// - Parameters:
    ///   - phoneme: Synthesized phoneme.
    ///   - startSec: Start of phoneme in seconds.
    ///   - endSec: End of phoneme in seconds.
    public init(
        phoneme: String,
        startSec: Float,
        endSec: Float) {
        self.phoneme = phoneme
        self.startSec = startSec
        self.endSec = endSec
    }
}

public struct OrcaWord {

    /// Synthesized word.
    public let word: String

    /// Start of word in seconds.
    public let startSec: Float

    /// End of word in seconds.
    public let endSec: Float

    /// Array of phonemes.
    public let phonemeArray: [OrcaPhoneme]

    /// Constructor.
    ///
    /// - Parameters:
    ///   - word: Synthesized word.
    ///   - startSec: Start of word in seconds.
    ///   - endSec: End of word in seconds.
    ///   - phonemeArray: Array of phonemes.
    public init(
        word: String,
        startSec: Float,
        endSec: Float,
        phonemeArray: [OrcaPhoneme]) {
        self.word = word
        self.startSec = startSec
        self.endSec = endSec
        self.phonemeArray = phonemeArray
    }
}

/// iOS (Swift) binding for Orca Text-to-Speech engine. Provides a Swift interface to the Orca library.
public class Orca {

    private var handle: OpaquePointer?

    private var stream: OpaquePointer?
    /// Orca valid symbols
    private var _validCharacters: Set<String>?
    /// Orca sample rate
    private var _sampleRate: Int32?
    /// Maximum number of characters allowed in a single synthesis request.
    private var _maxCharacterLimit: Int32?
    /// Orca version string
    public static let version = String(cString: pv_orca_version())

    private static var sdk = "ios"

    /// OrcaStream object that converts a stream of text to a stream of audio.
    public class OrcaStream {

        private var orca: Orca

        private var stream: OpaquePointer?

        /// Adds a chunk of text to the OrcaStream object and generates audio if enough text has been added.
        /// This function is expected to be called multiple times with consecutive chunks of text from a text stream.
        /// The incoming text is buffered as it arrives until the length is long enough to convert a chunk of the
        /// buffered text into audio. The caller needs to use `OrcaStream.flush()` to generate the audio chunk
        /// for the remaining text that has not yet been synthesized.
        ///
        /// - Parameters:
        ///   - text: A chunk of text from a text input stream, comprised of valid characters.
        ///     Valid characters can be retrieved by calling `.validCharacters`.
        ///     Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
        ///     They need to be added in a single call to this function.
        ///     The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
        /// - Returns: The generated audio as a sequence of 16-bit linearly-encoded integers, `nil` if no
        ///   audio chunk has been produced.
        /// - Throws: OrcaError
        public func synthesize(text: String) throws -> [Int16]? {
            if orca == nil {
                throw OrcaInvalidStateError("Unable to synthesize - orca has been released")
            }

            if stream == nil {
                throw OrcaInvalidStateError("Unable to synthesize - stream not open")
            }

            var cNumSamples: Int32 = 0
            var cPcm: UnsafeMutablePointer<Int16>?

            let status = pv_orca_stream_synthesize(
                stream,
                text,
                &cNumSamples,
                &cPcm)
            if status != PV_STATUS_SUCCESS {
                let messageStack = try orca.getMessageStack()
                throw orca.pvStatusToOrcaError(status, "Unable to synthesize streaming speech", messageStack)
            }

            let buffer = UnsafeBufferPointer(start: cPcm, count: Int(cNumSamples))
            let pcm = Array(buffer)

            pv_orca_pcm_delete(cPcm)

            return pcm.isEmpty ? nil : pcm
        }

        /// Generates audio for all the buffered text that was added to the OrcaStream object
        /// via `OrcaStream.synthesize()`.
        ///
        /// - Returns: The generated audio as a sequence of 16-bit linearly-encoded integers, `nil` if no
        ///   audio chunk has been produced.
        /// - Throws: OrcaError
        public func flush() throws -> [Int16]? {
            if orca == nil {
                throw OrcaInvalidStateError("Unable to flush - orca has been released")
            }

            if stream == nil {
                throw OrcaInvalidStateError("Unable to flush - stream not open")
            }

            var cNumSamples: Int32 = 0
            var cPcm: UnsafeMutablePointer<Int16>?

            let status = pv_orca_stream_flush(
                stream,
                &cNumSamples,
                &cPcm)
            if status != PV_STATUS_SUCCESS {
                let messageStack = try orca.getMessageStack()
                throw orca.pvStatusToOrcaError(status, "Unable to flush streaming speech", messageStack)
            }

            let buffer = UnsafeBufferPointer(start: cPcm, count: Int(cNumSamples))
            let pcm = Array(buffer)

            pv_orca_pcm_delete(cPcm)

            return pcm.isEmpty ? nil : pcm
        }

        /// Releases the resources acquired by the OrcaStream object.
        public func close() {
            if stream != nil {
                pv_orca_stream_close(stream)
                stream = nil
            }
        }

        public init(orca: Orca, stream: OpaquePointer) {
            self.orca = orca
            self.stream = stream
        }
    }

    public static func setSdk(sdk: String) {
        self.sdk = sdk
    }

    /// Set of characters supported by Orca.
    public var validCharacters: Set<String> {
        get throws {
            if _validCharacters == nil {
                var cNumCharacters: Int32 = 0
                var cCharacters: UnsafeMutablePointer<UnsafePointer<Int8>?>?
                let status = pv_orca_valid_characters(handle, &cNumCharacters, &cCharacters)
                if status != PV_STATUS_SUCCESS {
                    let messageStack = try getMessageStack()
                    throw pvStatusToOrcaError(status, "Unable to get Orca valid characters", messageStack)
                }

                var characters: Set<String> = []
                for i in 0..<cNumCharacters {
                    if let cString = cCharacters?.advanced(by: Int(i)).pointee {
                        let swiftString = String(cString: cString)
                        characters.insert(swiftString)
                    }
                }

                pv_orca_valid_characters_delete(cCharacters)

                _validCharacters = characters
            }
            return _validCharacters!
        }
    }

    /// Audio sample rate of generated audio.
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

    /// Maximum number of characters allowed per call to `synthesize()`.
    public var maxCharacterLimit: Int32 {
        get throws {
            if _maxCharacterLimit == nil {
                var cMaxCharacterLimit: Int32 = 0
                let status = pv_orca_max_character_limit(handle, &cMaxCharacterLimit)
                if status != PV_STATUS_SUCCESS {
                    let messageStack = try getMessageStack()
                    throw pvStatusToOrcaError(status, "Orca failed to get max character limit", messageStack)
                }

                _maxCharacterLimit = cMaxCharacterLimit
            }

            return _maxCharacterLimit!
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
    ///    `self.maxCharacterLimit`. Allowed characters can be retrieved by calling `self.validCharacters`.
    ///    Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
    ///    The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
    ///   - speechRate: Rate of speech of the generated audio. Valid values are within [0.7, 1.3].
    ///   - randomState: Random seed for the synthesis process.
    /// - Returns: A tuple containing the generated audio as a sequence of 16-bit linearly-encoded integers
    ///   and an array of OrcaWord objects representing the word alignments.
    /// - Throws: OrcaError
    public func synthesize(text: String, speechRate: Double? = nil, randomState: Int64? = nil) throws -> (pcm: [Int16], wordArray: [OrcaWord]) {
        if handle == nil {
            throw OrcaInvalidStateError("Unable to synthesize - resources have been released")
        }

        let orcaMaxChararacterLimit = try maxCharacterLimit
        if text.count > orcaMaxChararacterLimit {
            throw OrcaInvalidArgumentError(
                "Text length (\(text.count)) must be smaller than \(orcaMaxChararacterLimit)")
        }

        let cSynthesizeParams = try getCSynthesizeParams(speechRate: speechRate, randomState: randomState)

        var cNumSamples: Int32 = 0
        var cPcm: UnsafeMutablePointer<Int16>?

        var cNumAlignments: Int32 = 0
        var cAlignments: UnsafeMutablePointer<UnsafeMutablePointer<pv_orca_word_alignment_t>?>?

        let status = pv_orca_synthesize(
            handle,
            text,
            cSynthesizeParams,
            &cNumSamples,
            &cPcm,
            &cNumAlignments,
            &cAlignments)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to synthesize speech", messageStack)
        }

        let buffer = UnsafeBufferPointer(start: cPcm, count: Int(cNumSamples))
        let pcm = Array(buffer)

        var wordArray = [OrcaWord]()
        if let cAlignments = cAlignments {
            for alignmentIndex in 0..<Int(cNumAlignments) {
                if let cAlignment = cAlignments[alignmentIndex]?.pointee {
                    var phonemeArray = [OrcaPhoneme]()
                    if let phonemesPointer = cAlignment.phonemes {
                        for phonemeIndex in 0..<Int(cAlignment.num_phonemes) {
                            if let cPhoneme = phonemesPointer.advanced(by: phonemeIndex).pointee {
                                let phoneme = OrcaPhoneme(
                                    phoneme: String(cString: cPhoneme.pointee.phoneme),
                                    startSec: Float(cPhoneme.pointee.start_sec),
                                    endSec: Float(cPhoneme.pointee.end_sec)
                                )
                                phonemeArray.append(phoneme)
                            }
                        }
                    }

                    let word = OrcaWord(
                        word: String(cString: cAlignment.word),
                        startSec: Float(cAlignment.start_sec),
                        endSec: Float(cAlignment.end_sec),
                        phonemeArray: phonemeArray
                    )
                    wordArray.append(word)
                }
            }
        }

        pv_orca_pcm_delete(cPcm)
        pv_orca_synthesize_params_delete(cSynthesizeParams)
        pv_orca_word_alignments_delete(cNumAlignments, cAlignments)

        return (pcm, wordArray)
    }

    /// Generates audio from text and saves it to a WAV file. The file contains the speech representation of the text.
    ///
    /// - Parameters:
    ///   - text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is
    ///    `self.maxCharacterLimit`. Allowed characters can be retrieved by calling `self.validCharacters`.
    ///    Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
    ///    The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
    ///   - outputPath: Absolute path to the output audio file. The output file is saved as `WAV (.wav)`
    ///     and consists of a single mono channel.
    ///   - speechRate: Rate of speech of the generated audio. Valid values are within [0.7, 1.3].
    ///   - randomState: Random seed for the synthesis process.
    /// - Returns: An array of OrcaWord objects representing the word alignments.
    /// - Throws: OrcaError
    public func synthesizeToFile(text: String, outputPath: String, speechRate: Double? = nil, randomState: Int64? = nil) throws -> [OrcaWord] {
        if handle == nil {
            throw OrcaInvalidStateError("Unable to synthesize - resources have been released")
        }

        let orcaMaxChararacterLimit = try maxCharacterLimit
        if text.count > orcaMaxChararacterLimit {
            throw OrcaInvalidArgumentError(
                "Text length (\(text.count)) must be smaller than \(orcaMaxChararacterLimit)")
        }

        let cSynthesizeParams = try getCSynthesizeParams(speechRate: speechRate, randomState: randomState)

        var cNumAlignments: Int32 = 0
        var cAlignments: UnsafeMutablePointer<UnsafeMutablePointer<pv_orca_word_alignment_t>?>?

        let status = pv_orca_synthesize_to_file(
            handle,
            text,
            cSynthesizeParams,
            outputPath,
            &cNumAlignments,
            &cAlignments)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to synthesize speech to file", messageStack)
        }

        var wordArray = [OrcaWord]()
        if let cAlignments = cAlignments {
            for alignmentIndex in 0..<Int(cNumAlignments) {
                if let cAlignment = cAlignments[alignmentIndex]?.pointee {
                    var phonemeArray = [OrcaPhoneme]()
                    if let phonemesPointer = cAlignment.phonemes {
                        for phonemeIndex in 0..<Int(cAlignment.num_phonemes) {
                            if let cPhoneme = phonemesPointer.advanced(by: phonemeIndex).pointee {
                                let phoneme = OrcaPhoneme(
                                    phoneme: String(cString: cPhoneme.pointee.phoneme),
                                    startSec: Float(cPhoneme.pointee.start_sec),
                                    endSec: Float(cPhoneme.pointee.end_sec)
                                )
                                phonemeArray.append(phoneme)
                            }
                        }
                    }

                    let word = OrcaWord(
                        word: String(cString: cAlignment.word),
                        startSec: Float(cAlignment.start_sec),
                        endSec: Float(cAlignment.end_sec),
                        phonemeArray: phonemeArray
                    )
                    wordArray.append(word)
                }
            }
        }

        pv_orca_synthesize_params_delete(cSynthesizeParams)
        pv_orca_word_alignments_delete(cNumAlignments, cAlignments)

        return wordArray
    }

    /// Generates audio from text and saves it to a WAV file. The file contains the speech representation of the text.
    ///
    /// - Parameters:
    ///   - text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is
    ///    `self.maxCharacterLimit`. Allowed characters can be retrieved by calling `self.validCharacters`.
    ///    Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
    ///    The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
    ///   - outputURL: URL to the output audio file. The output file is saved as `WAV (.wav)`
    ///     and consists of a single mono channel.
    ///   - speechRate: Rate of speech of the generated audio. Valid values are within [0.7, 1.3].
    ///   - randomState: Random seed for the synthesis process.
    /// - Returns: An array of OrcaWord objects representing the word alignments.
    /// - Throws: OrcaError
    public func synthesizeToFile(text: String, outputURL: URL, speechRate: Double? = nil, randomState: Int64? = nil) throws -> [OrcaWord] {
        try synthesizeToFile(text: text, outputPath: outputURL.path, speechRate: speechRate, randomState: randomState)
    }

    private func getCSynthesizeParams(speechRate: Double? = nil, randomState: Int64? = nil) throws -> OpaquePointer? {
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

        if randomState != nil {
            status = pv_orca_synthesize_params_set_random_state(cParams, randomState!)
            if status != PV_STATUS_SUCCESS {
                let messageStack = try getMessageStack()
                throw pvStatusToOrcaError(status, "Unable to set Orca random state", messageStack)
            }
        }

        return cParams
    }

    /// Opens a stream for streaming text synthesis.
    ///
    /// - Parameters:
    ///   - speechRate: Rate of speech of the generated audio. Valid values are within [0.7, 1.3].
    ///   - randomState: Random seed for the synthesis process.
    /// - Returns: An instance of the OrcaStream class.
    /// - Throws: OrcaError
    public func streamOpen(speechRate: Double? = nil, randomState: Int64? = nil) throws -> OrcaStream {
        if handle == nil {
            throw OrcaInvalidStateError("Unable to synthesize - resources have been released")
        }

        let cSynthesizeParams = try getCSynthesizeParams(speechRate: speechRate, randomState: randomState)

        let status = pv_orca_stream_open(
            handle,
            cSynthesizeParams,
            &stream)
        if status != PV_STATUS_SUCCESS {
            let messageStack = try getMessageStack()
            throw pvStatusToOrcaError(status, "Unable to open stream", messageStack)
        }

        return OrcaStream(orca: self, stream: stream!)
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
