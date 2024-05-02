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

    func testValidCharacters() throws {
        for orca in self.orcas {
            let characters = try orca.validCharacters
            XCTAssertGreaterThan(characters.count, 0)
            XCTAssert(characters.contains(","))
        }
    }

    func testInvalidText() throws {
        for orca in self.orcas {
            for sentence in self.testData!.test_sentences.text_invalid {
                do {
                    let (pcm, wordArray) = try orca.synthesize(text: sentence)
                    XCTAssertNil(pcm)
                    XCTAssertNil(wordArray)
                } catch {
                    XCTAssertTrue(error is OrcaError)
                }
            }
        }
    }

    func testMaxCharacterLimit() throws {
        XCTAssertGreaterThan(Orca.maxCharacterLimit, 0)
    }

    func testSampleRate() throws {
        for orca in self.orcas {
            XCTAssertGreaterThan(try orca.sampleRate, 0)
        }
    }

    func testStreaming() throws {
        let orcaStream = try orca.streamOpen()

        var fullPcm = [Int16]()
        for c in text {
            if let pcm = orcaStream.synthesize(String(c)) {
                if !pcm.isEmpty {
                    fullPcm.append(contentsOf: pcm)
                }
            }
        }

        if let flushedPcm = orcaStream.flush(), !flushedPcm.isEmpty {
            fullPcm.append(contentsOf: flushedPcm)
        }

        try orcaStream.close()

        let groundTruth = self.getPcm(index == 0 ? self.testAudioMaleSingle : self.testAudioFemaleSingle)

        XCTAssertEqual(fullPcm, groundTruth, "Synthesized pcm does not match ground truth")
    }


    func testSynthesize() throws {
        for (index, orca) in self.orcas.enumerated() {
            let (pcm, wordArray) = try orca.synthesize(text: self.testData!.test_sentences.text, randomState: self.testData!.random_state)
            XCTAssertGreaterThan(pcm.count, 0)
            XCTAssertGreaterThan(wordArray.count, 0)

            let groundTruth = self.getPcm(index == 0 ? self.testAudioMaleSingle : self.testAudioFemaleSingle)
            XCTAssertEqual(pcm.count, expectedPCM.count)
            XCTAssertEqual(pcm, groundTruth, "Synthesized pcm does not match ground truth")

            // TODO: try below if above doesn't work
            // for (index, pcmValue) in pcm.enumerated() {
            //    let expectedValue = groundTruth[index]
            //    XCTAssert(abs(pcmValue - expectedValue) <= delta)
            // }
        }
    }

    func testAlignments() throws {
        for (index, orca) in self.orcas.enumerated() {
            let (pcm, wordArray) = try orca.synthesize(text: self.testData!.test_sentences.text_alignment, randomState: self.testData!.random_state)
            XCTAssertGreaterThan(pcm.count, 0)
            XCTAssertGreaterThan(wordArray.count, 0)

            let audioDir = try FileManager.default.url(
                                        for: .documentDirectory,
                                        in: .userDomainMask,
                                        appropriateFor: nil,
                                        create: false)
            let audioFile = audioDir.appendingPathComponent("test.wav")
            let synthToFileWordArray = try orca.synthesizeToFile(text: self.testData!.test_sentences.text, outputURL: audioFile)
            try FileManager().removeItem(at: audioFile)

            var synthesizeTestData = [OrcaWord]()
            for alignment in self.testData!.alignments {
                let testData = alignment.asJsonObject() ?? [:]
                let word = testData["word"].asString() ?? ""
                let startSec = testData["start_sec"].asFloat() ?? 0.0
                let endSec = testData["end_sec"].asFloat() ?? 0.0
                let phonemesJson = testData["phonemes"].asJsonArray() ?? []

                var phonemes = [OrcaPhoneme]()
                for phonemeJson in phonemesJson {
                    let phoneme = phonemeJson["phoneme"].asString() ?? ""
                    let phonemeStartSec = phonemeJson["start_sec"].asFloat() ?? 0.0
                    let phonemeEndSec = phonemeJson["end_sec"].asFloat() ?? 0.0
                    phonemes.append(OrcaPhoneme(phoneme: phoneme, startSec: phonemeStartSec, endSec: phonemeEndSec))
                }

                synthesizeTestData.append(OrcaWord(word: word, startSec: startSec, endSec: endSec, phonemeArray: phonemes))
            }

            // TODO: index == 1: Be exact if female model
            validateMetadata(wordArray, synthesizeTestData, index == 1)
            validateMetadata(synthToFileWordArray, synthesizeTestData, index == 1)
        }
    }

    func testSynthesizeCustomPron() throws {
        for orca in self.orcas {
            let (pcm, wordArray) = try orca.synthesize(text: self.testData!.test_sentences.text_custom_pronunciation)
            XCTAssertGreaterThan(pcm.count, 0)
            XCTAssertGreaterThan(wordArray.count, 0)
        }
    }

    func testSynthesizeSpeechRate() throws {
        for orca in self.orcas {
            let (pcm: pcmFast, wordArray: wordArrayFast) = try orca.synthesize(text: self.testData!.test_sentences.text, speechRate: 1.3)
            let (pcm: pcmSlow, wordArray: wordArraySlow) = try orca.synthesize(text: self.testData!.test_sentences.text, speechRate: 0.7)

            XCTAssertLessThan(pcmFast.count, pcmSlow.count)
            XCTAssertEqual(wordArrayFast.count, wordArraySlow.count)

            do {
                let (pcm, wordArray) = try orca.synthesize(text: self.testData!.test_sentences.text, speechRate: 9999)
                XCTAssertNil(pcm)
                XCTAssertNil(wordArray)
            } catch { }
        }
    }

    func testSynthesizeRandomState() throws {
        let randomState1 = try orca.synthesize(text: text, randomState: 1)
        XCTAssertGreaterThan(randomState1.audio.count, 0)
        XCTAssertGreaterThan(randomState1.wordArray.count, 0)

        let randomState2 = try orca.synthesize(text: text, randomState: 2)
        XCTAssertGreaterThan(randomState2.audio.count, 0)
        XCTAssertGreaterThan(randomState2.wordArray.count, 0)

        XCTAssertNotEqual(randomState1, randomState2)
        XCTAssertNotEqual(randomState1.wordArray, randomState2.wordArray)

        let randomStateNull = try orca.synthesize(text: text)
        XCTAssertGreaterThan(randomStateNull.audio.count, 0)
        XCTAssertGreaterThan(randomStateNull.wordArray.count, 0)
    }


    func testSynthesizeToFile() throws {
        let audioDir = try FileManager.default.url(
                                    for: .documentDirectory,
                                    in: .userDomainMask,
                                    appropriateFor: nil,
                                    create: false)
        let audioFile = audioDir.appendingPathComponent("test.wav")

        for orca in self.orcas {
            try orca.synthesizeToFile(text: self.testData!.test_sentences.text, outputURL: audioFile)
            XCTAssert(FileManager().fileExists(atPath: audioFile.path))
            try FileManager().removeItem(at: audioFile)

            try orca.synthesizeToFile(text: self.testData!.test_sentences.text, outputPath: audioFile.path)
            XCTAssert(FileManager().fileExists(atPath: audioFile.path))
            try FileManager().removeItem(at: audioFile)
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
            let pcm = try orca.synthesize(text: self.testData!.test_sentences.text)
            XCTAssertNil(pcm)
        } catch {
            XCTAssert("\(error.localizedDescription)".count > 0)
        }
    }
}
