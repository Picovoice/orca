//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

import XCTest
import Orca

class OrcaAppTestUITests: BaseTest {

    override func setUpWithError() throws {
        continueAfterFailure = true
    }

    func runTestSynthesize(orca: Orca, model: String, testCase: SentenceTests) throws {
        let (pcm, wordArray) = try orca.synthesize(text: testCase.text, randomState: Int64(testCase.random_state))
        XCTAssertGreaterThan(pcm.count, 0)
        XCTAssertGreaterThan(wordArray.count, 0)

        let groundTruth = try self.getPcm(fileUrl: self.getAudioFileUrl(model: model, synthesis_type: "single"))
        XCTAssertEqual(pcm.count, groundTruth.count)
        XCTAssertTrue(compareArrays(arr1: pcm, arr2: groundTruth, step: 1))
    }

    func testSynthesize() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestSynthesize(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestStreaming(orca: Orca, model: String, testCase: SentenceTests) throws {
        let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

        let orcaStream = try orca.streamOpen(randomState: Int64(testCase.random_state))

        var fullPcm = [Int16]()
        for c in testCase.text {
            if let pcm = try orcaStream.synthesize(text: String(c)) {
                if !pcm.isEmpty {
                    fullPcm.append(contentsOf: pcm)
                }
            }
        }

        if let flushedPcm = try orcaStream.flush(), !flushedPcm.isEmpty {
            fullPcm.append(contentsOf: flushedPcm)
        }

        orcaStream.close()

        let groundTruth = try self.getPcm(fileUrl: self.getAudioFileUrl(model: model, synthesis_type: "stream"))
        XCTAssertEqual(fullPcm.count, groundTruth.count)
        XCTAssertTrue(compareArrays(arr1: fullPcm, arr2: groundTruth, step: 1))

        orca.delete()
    }

    func testStreaming() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestStreaming(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestMaxCharacterLimit(orca: Orca, model: String, testCase: SentenceTests) {
        XCTAssertGreaterThan(orca.maxCharacterLimit!, 0)
    }

    func testMaxCharacterLimit() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestMaxCharacterLimit(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestSampleRate(orca: Orca, model: String, testCase: SentenceTests) throws {
        XCTAssertGreaterThan(orca.sampleRate!, 0)
    }

    func testSampleRate() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestSampleRate(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestValidCharacters(orca: Orca, model: String, testCase: SentenceTests) throws {
        XCTAssertGreaterThan(orca.validCharacters!.count, 0)
    }

    func testValidCharacters() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestValidCharacters(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestSynthesizeCustomPron(orca: Orca, model: String, testCase: SentenceTests) throws {
        let (pcm, wordArray) = try orca.synthesize(text: testCase.text_custom_pronunciation)
        XCTAssertGreaterThan(pcm.count, 0)
        XCTAssertGreaterThan(wordArray.count, 0)
    }

    func testSynthesizeCustomPron() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestSynthesizeCustomPron(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestSynthesizeSpeechRate(orca: Orca, model: String, testCase: SentenceTests) throws {
        let (pcm: pcmFast, wordArray: wordArrayFast) = try orca.synthesize(text: testCase.text, speechRate: 1.3)
        let (pcm: pcmSlow, wordArray: wordArraySlow) = try orca.synthesize(text: testCase.text, speechRate: 0.7)

        XCTAssertLessThan(pcmFast.count, pcmSlow.count)
        XCTAssertEqual(wordArrayFast.count, wordArraySlow.count)

        do {
            let (pcm, wordArray) = try orca.synthesize(text: testCase.text, speechRate: 9999)
            XCTAssertNil(pcm)
            XCTAssertNil(wordArray)
        } catch { }
    }

    func testSynthesizeSpeechRate() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestSynthesizeSpeechRate(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestSynthesizeRandomState(orca: Orca, model: String, testCase: SentenceTests) throws {
        let (pcm: pcm1, wordArray: wordArray1) = try orca.synthesize(text: testCase.text, randomState: 1)
        XCTAssertGreaterThan(pcm1.count, 0)
        XCTAssertGreaterThan(wordArray1.count, 0)

        let (pcm: pcm2, wordArray: wordArray2) = try orca.synthesize(text: testCase.text, randomState: 2)
        XCTAssertGreaterThan(pcm2.count, 0)
        XCTAssertGreaterThan(wordArray2.count, 0)

        XCTAssertNotEqual(pcm1, pcm2)
    }

    func testSynthesizeRandomState() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestSynthesizeRandomState(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestSynthesizeToFile(orca: Orca, model: String, testCase: SentenceTests) throws {
        let audioDir = try FileManager.default.url(
                                    for: .documentDirectory,
                                    in: .userDomainMask,
                                    appropriateFor: nil,
                                    create: false)
        let audioFile = audioDir.appendingPathComponent("test.wav")

        let wordArrayFromURL = try orca.synthesizeToFile(text: testCase.text, outputURL: audioFile)
        XCTAssert(FileManager().fileExists(atPath: audioFile.path))
        XCTAssertGreaterThan(wordArrayFromURL.count, 0)
        try FileManager().removeItem(at: audioFile)

        let wordArrayFromPath = try orca.synthesizeToFile(text: testCase.text, outputPath: audioFile.path)
        XCTAssert(FileManager().fileExists(atPath: audioFile.path))
        XCTAssertGreaterThan(wordArrayFromPath.count, 0)
        try FileManager().removeItem(at: audioFile)
    }

    func testSynthesizeToFile() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestSynthesizeToFile(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    let textQuotes = "iOS uses different quotation marks for ‘single’ and “double” quotes."

    func runTestSynthesizeQuotes(orca: Orca, model: String, testCase: SentenceTests) throws {
        let (pcm, wordArray) = try orca.synthesize(text: testCase.text, randomState: Int64(testCase.random_state))
        XCTAssertGreaterThan(pcm.count, 0)
        XCTAssertGreaterThan(wordArray.count, 0)

        let groundTruth = try self.getPcm(fileUrl: self.getAudioFileUrl(model: model, synthesis_type: "single"))
        XCTAssertEqual(pcm.count, groundTruth.count)
        XCTAssertTrue(compareArrays(arr1: pcm, arr2: groundTruth, step: 1))
    }

    func testSynthesizeQuotes() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestSynthesizeQuotes(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestStreamingQuotes(orca: Orca, model: String, testCase: SentenceTests) throws {
        let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

        let orcaStream = try orca.streamOpen(randomState: Int64(testCase.random_state))

        var fullPcm = [Int16]()
        for c in testCase.text {
            if let pcm = try orcaStream.synthesize(text: String(c)) {
                if !pcm.isEmpty {
                    fullPcm.append(contentsOf: pcm)
                }
            }
        }

        if let flushedPcm = try orcaStream.flush(), !flushedPcm.isEmpty {
            fullPcm.append(contentsOf: flushedPcm)
        }

        orcaStream.close()

        let groundTruth = try self.getPcm(fileUrl: self.getAudioFileUrl(model: model, synthesis_type: "stream"))
        XCTAssertEqual(fullPcm.count, groundTruth.count)
        XCTAssertTrue(compareArrays(arr1: fullPcm, arr2: groundTruth, step: 1))

        orca.delete()
    }

    func testStreamingQuotes() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestStreamingQuotes(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestSynthesizeToFileQuotes(orca: Orca, model: String, testCase: SentenceTests) throws {
        let audioDir = try FileManager.default.url(
                                    for: .documentDirectory,
                                    in: .userDomainMask,
                                    appropriateFor: nil,
                                    create: false)
        let audioFile = audioDir.appendingPathComponent("test.wav")

        let wordArrayFromURL = try orca.synthesizeToFile(text: testCase.text, outputURL: audioFile)
        XCTAssert(FileManager().fileExists(atPath: audioFile.path))
        XCTAssertGreaterThan(wordArrayFromURL.count, 0)
        try FileManager().removeItem(at: audioFile)

        let wordArrayFromPath = try orca.synthesizeToFile(text: testCase.text, outputPath: audioFile.path)
        XCTAssert(FileManager().fileExists(atPath: audioFile.path))
        XCTAssertGreaterThan(wordArrayFromPath.count, 0)
        try FileManager().removeItem(at: audioFile)
    }

    func testSynthesizeToFileQuotes() throws {
        for testCase in self.testData!.tests.sentence_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestSynthesizeToFileQuotes(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestInvalidText(orca: Orca, model: String, testCase: InvalidTests) throws {
        for sentence in testCase.text_invalid {
            do {
                let (pcm, wordArray) = try orca.synthesize(text: sentence)
                XCTAssertNil(pcm)
                XCTAssertNil(wordArray)
            } catch {
                XCTAssertTrue(error is OrcaError)
            }
        }
    }

    func testInvalidText() throws {
        for testCase in self.testData!.tests.invalid_tests {
            for model in testCase.models {
                try XCTContext.runActivity(named: "(\(testCase.language) \(model))") { _ in
                    let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: model))

                    try runTestInvalidText(orca: orca, model: model, testCase: testCase)

                    orca.delete()
                }
            }
        }
    }

    func runTestAlignments(orca: Orca, model: String, testCase: AlignmentTests) throws {
        let (pcm, wordArray) = try orca.synthesize(
                                text: testCase.text_alignment,
                                randomState: Int64(testCase.random_state))
        XCTAssertGreaterThan(pcm.count, 0)
        XCTAssertGreaterThan(wordArray.count, 0)

        let audioDir = try FileManager.default.url(
                                    for: .documentDirectory,
                                    in: .userDomainMask,
                                    appropriateFor: nil,
                                    create: false)
        let audioFile = audioDir.appendingPathComponent("test.wav")
        let synthToFileWordArray = try orca.synthesizeToFile(text: testCase.text_alignment, outputURL: audioFile)
        try FileManager().removeItem(at: audioFile)

        var synthesizeTestData = [OrcaWord]()
        for alignment in testCase.alignments {
            var phonemeArray = [OrcaPhoneme]()
            for phoneme in alignment.phonemes {
                phonemeArray.append(OrcaPhoneme(
                    phoneme: phoneme.phoneme, startSec: phoneme.start_sec, endSec: phoneme.end_sec))
            }

            synthesizeTestData.append(
                OrcaWord(
                    word: alignment.word,
                    startSec: alignment.start_sec,
                    endSec: alignment.end_sec,
                    phonemeArray: phonemeArray))
        }

        validateMetadata(words: wordArray, expectedWords: synthesizeTestData, isExpectExact: true)
        validateMetadata(words: synthToFileWordArray, expectedWords: synthesizeTestData, isExpectExact: true)
    }

    func testAlignments() throws {
        for testCase in self.testData!.tests.alignment_tests {
            try XCTContext.runActivity(named: "(\(testCase.language) \(testCase.model))") { _ in
                let orca = try Orca.init(accessKey: self.accessKey, modelPath: self.getModelPath(model: testCase.model))

                try runTestAlignments(orca: orca, model: testCase.model, testCase: testCase)

                orca.delete()
            }
        }
    }

    func testVersion() throws {
        XCTAssertGreaterThan(Orca.version.count, 0)
    }

    func testMessageStack() throws {
        let bundle = Bundle(for: type(of: self))
        let modelPath: String = bundle.path(
                forResource: "orca_params_female",
                ofType: "pv",
                inDirectory: "test_resources/model_files")!

        var first_error: String = ""
        do {
            let orca: Orca = try Orca(accessKey: "invalid", modelPath: modelPath)
            XCTAssertNil(orca)
        } catch {
            first_error = "\(error.localizedDescription)"
            XCTAssert(first_error.count < 1024)
        }

        do {
            let orca: Orca = try Orca(accessKey: "invalid", modelPath: modelPath)
            XCTAssertNil(orca)
        } catch {
            XCTAssert("\(error.localizedDescription)".count == first_error.count)
        }
    }

    func testSynthesizeMessageStack() throws {
        let bundle = Bundle(for: type(of: self))
        let modelPath: String = bundle.path(
                forResource: "orca_params_female",
                ofType: "pv",
                inDirectory: "test_resources/model_files")!

        let orca: Orca = try Orca(accessKey: accessKey, modelPath: modelPath)
        orca.delete()

        do {
            let pcm = try orca.synthesize(text: self.testData!.tests.sentence_tests[0].text)
            XCTAssertNil(pcm)
        } catch {
            XCTAssert("\(error.localizedDescription)".count > 0)
        }
    }
}
