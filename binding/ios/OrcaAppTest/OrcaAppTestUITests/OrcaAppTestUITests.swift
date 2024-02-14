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
import Leopard

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
                    let pcm = try orca.synthesize(text: sentence)
                    XCTAssertNil(pcm)
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

    func testSynthesize() throws {
        let bundle = Bundle(for: type(of: self))
        let leopardModelPath = bundle.path(
            forResource: "leopard_params",
            ofType: "pv",
            inDirectory: "test_resources/model_files")!
        let leopard = try Leopard.init(accessKey: self.accessKey, modelPath: leopardModelPath)

        for orca in self.orcas {
            let pcm = try orca.synthesize(text: self.testData!.test_sentences.text)
            XCTAssertGreaterThan(pcm.count, 0)

            let groundTruth = self.testData!.test_sentences.text_no_punctuation
            let (transcript, _) = try leopard.process(pcm)

            let wer = self.characterErrorRate(transcript: transcript, expectedTranscript: groundTruth)
            XCTAssertLessThan(wer, self.testData!.wer_threshold)
        }

        leopard.delete()
    }

    func testSynthesizeCustomPron() throws {
        for orca in self.orcas {
            let pcm = try orca.synthesize(text: self.testData!.test_sentences.text_custom_pronunciation)
            XCTAssertGreaterThan(pcm.count, 0)
        }
    }

    func testSynthesizeSpeechRate() throws {
        for orca in self.orcas {
            let pcmFast = try orca.synthesize(text: self.testData!.test_sentences.text, speechRate: 1.3)
            let pcmSlow = try orca.synthesize(text: self.testData!.test_sentences.text, speechRate: 0.7)

            XCTAssertLessThan(pcmFast.count, pcmSlow.count)

            do {
                let pcm = try orca.synthesize(text: self.testData!.test_sentences.text, speechRate: 9999)
                XCTAssertNil(pcm)
            } catch { }
        }
    }

    func testVersion() throws {
        XCTAssertGreaterThan(Orca.version.count, 0)
    }

    func testMessageStack() throws {
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
