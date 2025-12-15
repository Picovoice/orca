//
//  Copyright 2024-2025 Picovoice Inc.
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
    var tests: TestsData
    var audio_data_folder: String
}

struct TestsData: Decodable {
    var sentence_tests: [SentenceTests]
    var alignment_tests: [AlignmentTests]
    var invalid_tests: [InvalidTests]
}

struct SentenceTests: Decodable {
    var language: String
    var models: [String]
    var random_state: Int
    var text: String
    var text_no_punctuation: String
    var text_custom_pronunciation: String
}

struct AlignmentTests: Decodable {
    var language: String
    var model: String
    var random_state: Int
    var text_alignment: String
    var alignments: [TestAlignment]
}

struct TestAlignment: Decodable {
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

struct InvalidTests: Decodable {
    var language: String
    var models: [String]
    var text_invalid: [String]
}

extension String {
    subscript(index: Int) -> Character {
        return self[self.index(self.startIndex, offsetBy: index)]
    }
}

class BaseTest: XCTestCase {

    let accessKey: String = "{TESTING_ACCESS_KEY_HERE}"
    let device: String = "{TESTING_DEVICE_HERE}"

    var testData: TestData?

    override func setUp() async throws {
        try await super.setUp()

        testData = try getTestData()
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

    func getAudioFileUrl(model: String, synthesis_type: String) -> URL {
        let filename = model.replacingOccurrences(of: ".pv", with: "_\(synthesis_type)")
        return Bundle(for: type(of: self)).url(forResource: "test_resources/wav/\(filename)", withExtension: "wav")!
    }

    func getModelPath(model: String) -> String {
        let model_name = model.replacingOccurrences(of: ".pv", with: "")
        return Bundle(for: type(of: self)).path(
                forResource: model_name,
                ofType: "pv",
                inDirectory: "test_resources/model_files")!
    }

    func compareArrays(arr1: [Int16], arr2: [Int16], step: Int) -> Bool {
        for i in stride(from: 0, to: arr1.count - step, by: step) where !(abs(arr1[i] - arr2[i]) <= 12000) {
            return false
        }
        return true
    }

    func getPcm(fileUrl: URL) throws -> [Int16] {
        let data = try Data(contentsOf: fileUrl)
        let pcmData = data.withUnsafeBytes { (ptr: UnsafePointer<Int16>) -> [Int16] in
            let count = data.count / MemoryLayout<Int16>.size
            return Array(UnsafeBufferPointer(start: ptr.advanced(by: 22), count: count - 22))
        }
        return pcmData
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
