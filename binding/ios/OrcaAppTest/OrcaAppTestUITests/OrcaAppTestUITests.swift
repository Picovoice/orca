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
    
    func testValidPunctuationSymbols() throws {
        for orca in self.orcas {
            let symbols = try orca.validPunctuationSymbols
            XCTAssertGreaterThan(symbols.count, 0)
            XCTAssertTrue(symbols.contains(","))
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
        var leopard = try Leopard.init(access_key=self.access_key)
        
        for orca in self.orcas {
            let pcm = try orca.synthesize(self.testData.tests.testSentences.text)
            XCTAssertGreaterThan(pcm.count, 0)
                
            let groundTruth = self.testData.tests.testSentences.textNoPunctuation.components(separatedBy: " ")
            let transcript, _ = try leopard.process(pcm)
            
            let wer = self.characterErrorRate(transcript: transcript, expectedTranscript: groundTruth)
            XCTAssertLessThan(wer, self.testData.tests.werThreshold)
        }
    }

    func testSynthesizeCustomPron() throws {
        for orca in self.orcas {
            let pcm = try orca.synthesize(self.testData.tests.testSentences.textCustomPunctuation)
            XCTAssertGreaterThan(pcm.count, 0)
        }
    }

    func testSynthesizeSpeechRate() throws {
        for orca in self.orcas {
            let pcmFast = try orca.synthesize(self.testData.tests.testSentences.text, speech_rate=1.3)
            let pcmSlow = try orca.synthesize(self.testData.tests.testSentences.text, speech_rate=0.7)
            
            self.assertLess(len(pcm_fast), len(pcm_slow))
            XCTAssertLessThan(pcmFast.count, pcmFast.count)
            
            do {
                let pcm = try orca.synthesize(test_sentences.text, speech_rate=9999)
                XCTAssertNil(pcm)
            } catch { }
        }
    }

    func testVersion() throws {
        XCTAssertGreaterThan(Orca.version.count, 0)
    }


    func testMessageStack() throws {
        var first_error: String = ""
        do {
            let orca: Orca = try Orca(accessKey: "invalid")
            XCTAssertNil(orca)
        } catch {
            first_error = "\(error.localizedDescription)"
            XCTAssert(first_error.count < 1024)
        }

        do {
            let orca: Orca = try Orca(accessKey: "invalid")
            XCTAssertNil(orca)
        } catch {
            XCTAssert("\(error.localizedDescription)".count == first_error.count)
        }
    }

    func testProcessMessageStack() throws {
        let orca: Orca = try Orca(accessKey: accessKey)
        orca.delete()

        do {
            
        } catch {
            XCTAssert("\(error.localizedDescription)".count > 0)
        }
    }
}
