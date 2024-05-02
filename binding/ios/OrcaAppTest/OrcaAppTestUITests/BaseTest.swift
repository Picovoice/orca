//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

import AVFoundation
import XCTest

import Orca

struct TestData: Decodable {
    var test_sentences: TestSentences
    var random_state: Int64
    var alignments: [TestAlignments]
}

struct TestSentences: Decodable {
    var text: String
    var text_no_punctuation: String
    var text_custom_pronunciation: String
    var text_alignment: String
    var text_invalid: [String]
}

struct TestAlignments: Decodable {
    var word: String
    var start_sec: Float
    var end_sec: Float
    var phonemes: [TestPhonemes]
}

struct TestPhonemes: Decodable {
    var phoneme: String
    var start_sec: Float
    var end_sec: Float
}

extension String {
    subscript(index: Int) -> Character {
        return self[self.index(self.startIndex, offsetBy: index)]
    }
}

class BaseTest: XCTestCase {
    let params: [String] = [
        "male",
        "female"
    ]

    let accessKey = "{TESTING_ACCESS_KEY_HERE}"
    var orcas: [Orca] = []
    var testData: TestData?

    let testAudioMaleSingle = Bundle(for: BaseTest.self).url(forResource: "test_resources/wav/orca_params_male_single", withExtension: "wav")!
    let testAudioMaleStream = Bundle(for: BaseTest.self).url(forResource: "test_resources/wav/orca_params_male_stream", withExtension: "wav")!
    let testAudioFemaleSingle = Bundle(for: BaseTest.self).url(forResource: "test_resources/wav/orca_params_female_single", withExtension: "wav")!
    let testAudioFemaleStream = Bundle(for: BaseTest.self).url(forResource: "test_resources/wav/orca_params_female_stream", withExtension: "wav")!

    override func setUp() async throws {
        try await super.setUp()

        testData = try getTestData()

        let bundle = Bundle(for: type(of: self))
        for param in params {
            let modelPath: String = bundle.path(
                    forResource: "orca_params_\(param)",
                    ofType: "pv",
                    inDirectory: "test_resources/model_files")!
            orcas.append(
                try Orca.init(accessKey: accessKey, modelPath: modelPath)
            )
        }
    }

    override func tearDown() {
        super.tearDown()
        for orca in orcas {
            orca.delete()
        }
    }

    func getTestData() throws -> TestData {
        let bundle = Bundle(for: type(of: self))
        let testDataJsonUrl = bundle.url(
            forResource: "test_data",
            withExtension: "json",
            subdirectory: "test_resources")!

        let testDataJsonData = try Data(contentsOf: testDataJsonUrl)
        let testData = try JSONDecoder().decode(TestData.self, from: testDataJsonData)

        return testData
    }

    func getPcm(fileUrl: URL) throws -> [Int16] {
        let data = try Data(contentsOf: fileUrl)
        var pcm = [Int16](repeating: 0, count: data.count / 2)

        _ = pcm.withUnsafeMutableBytes {
            data.copyBytes(to: $0, from: 44..<data.count)
        }
        return pcm
    }

    func validateMetadata(words: [OrcaWord], expectedWords: [OrcaWord], isExpectExact: Bool) {
        XCTAssertEqual(words.count, expectedWords.count)

        for i in 0..<words.count {
            XCTAssertEqual(words[i].word, expectedWords[i].word)

            if isExpectExact {
                XCTAssertEqual(words[i].startSec, expectedWords[i].startSec, accuracy: 0.1)
                XCTAssertEqual(words[i].endSec, expectedWords[i].endSec, accuracy: 0.1)
            }

            let phonemes = words[i].phonemeArray
            let expectedPhonemes = expectedWords[i].phonemeArray
            XCTAssertEqual(phonemes.count, expectedPhonemes.count)

            for j in 0..<phonemes.count {
                XCTAssertEqual(phonemes[j].phoneme, expectedPhonemes[j].phoneme)

                if isExpectExact {
                    XCTAssertEqual(phonemes[j].startSec, expectedPhonemes[j].startSec, accuracy: 0.1)
                    XCTAssertEqual(phonemes[j].endSec, expectedPhonemes[j].endSec, accuracy: 0.1)
                }
            }
        }
    }
}
