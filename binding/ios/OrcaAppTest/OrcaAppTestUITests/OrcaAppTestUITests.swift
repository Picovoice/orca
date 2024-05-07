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
        for orca in self.orcas {
            XCTAssertGreaterThan(try orca.maxCharacterLimit, 0)
        }
    }

    func testSampleRate() throws {
        for orca in self.orcas {
            XCTAssertGreaterThan(try orca.sampleRate, 0)
        }
    }

    func testStreaming() throws {
        for (index, orca) in self.orcas.enumerated() {
            let orcaStream = try orca.streamOpen(randomState: self.testData!.random_state)

            var fullPcm = [Int16]()
            for c in self.testData!.test_sentences.text {
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

            let groundTruth = try self.getPcm(fileUrl: index == 0 ? self.testAudioMaleStream : self.testAudioFemaleStream)

            XCTAssertEqual(fullPcm.count, groundTruth.count)
            XCTAssertTrue(compareArrays(arr1: fullPcm, arr2: groundTruth, step: 1))
        }
    }


    func testSynthesize() throws {
        for (index, orca) in self.orcas.enumerated() {
            let (pcm, wordArray) = try orca.synthesize(text: self.testData!.test_sentences.text, randomState: self.testData!.random_state)
            XCTAssertGreaterThan(pcm.count, 0)
            XCTAssertGreaterThan(wordArray.count, 0)

            let groundTruth = try self.getPcm(fileUrl: index == 0 ? self.testAudioMaleSingle : self.testAudioFemaleSingle)
            XCTAssertEqual(pcm.count, groundTruth.count)
            XCTAssertTrue(compareArrays(arr1: pcm, arr2: groundTruth, step: 1))
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
            let synthToFileWordArray = try orca.synthesizeToFile(text: self.testData!.test_sentences.text_alignment, outputURL: audioFile)
            try FileManager().removeItem(at: audioFile)

            var synthesizeTestData = [OrcaWord]()
            for alignment in self.testData!.alignments {
                var phonemeArray = [OrcaPhoneme]()
                for phoneme in alignment.phonemes {
                    phonemeArray.append(OrcaPhoneme(phoneme: phoneme.phoneme, startSec: phoneme.start_sec, endSec: phoneme.end_sec))
                }

                synthesizeTestData.append(OrcaWord(word: alignment.word, startSec: alignment.start_sec, endSec: alignment.end_sec, phonemeArray: phonemeArray))
            }

            // TODO: index == 1: Be exact if female model
            validateMetadata(words: wordArray, expectedWords: synthesizeTestData, isExpectExact: index == 1)
            validateMetadata(words: synthToFileWordArray, expectedWords: synthesizeTestData, isExpectExact: index == 1)
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
        for orca in self.orcas {
            let (pcm: pcm1, wordArray: wordArray1) = try orca.synthesize(text: self.testData!.test_sentences.text, randomState: 1)
            XCTAssertGreaterThan(pcm1.count, 0)
            XCTAssertGreaterThan(wordArray1.count, 0)

            let (pcm: pcm2, wordArray: wordArray2) = try orca.synthesize(text: self.testData!.test_sentences.text, randomState: 2)
            XCTAssertGreaterThan(pcm2.count, 0)
            XCTAssertGreaterThan(wordArray2.count, 0)

            XCTAssertNotEqual(pcm1, pcm2)
        }
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
