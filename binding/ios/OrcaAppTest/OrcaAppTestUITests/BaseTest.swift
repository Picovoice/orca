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
    var wer_threshold: Float32
}

struct TestSentences: Decodable {
    var text: String
    var text_no_punctuation: String
    var text_custom_pronunciation: String
}

extension String {
    subscript(index: Int) -> Character {
        return self[self.index(self.startIndex, offsetBy: index)]
    }
}

extension String {
    public func levenshtein(_ other: String) -> Int {
        let sCount = self.count
        let oCount = other.count

        guard sCount != 0 else {
            return oCount
        }

        guard oCount != 0 else {
            return sCount
        }

        let line: [Int]  = Array(repeating: 0, count: oCount + 1)
        var mat: [[Int]] = Array(repeating: line, count: sCount + 1)

        for i in 0...sCount {
            mat[i][0] = i
        }

        for j in 0...oCount {
            mat[0][j] = j
        }

        for j in 1...oCount {
            for i in 1...sCount {
                if self[i - 1] == other[j - 1] {
                    mat[i][j] = mat[i - 1][j - 1]       // no operation
                } else {
                    let del = mat[i - 1][j] + 1         // deletion
                    let ins = mat[i][j - 1] + 1         // insertion
                    let sub = mat[i - 1][j - 1] + 1     // substitution
                    mat[i][j] = min(min(del, ins), sub)
                }
            }
        }

        return mat[sCount][oCount]
    }
}

class BaseTest: XCTestCase {
    let params: [String] = [
        "male",
        "female"
    ]
    
    let accessKey = "Tw4jothrMMLyRYQ793yD/XF3DeithcbeNVsYlNN0Dc1vY26suWNOkg=="
    var orcas: [Orca] = []
    var testData: TestData?
    
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
    
    func characterErrorRate(transcript: String, expectedTranscript: String) -> Float {
        return Float(transcript.levenshtein(expectedTranscript)) / Float(expectedTranscript.count)
    }
}
