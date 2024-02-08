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
    var tests: TestDataTests
}

struct TestDataTests: Decodable {
    var testSentences: [TestSentences]
    var werThreshold: Float32
}

struct TestSentences: Decodable {
    var text: String
    var textNoPunctuation: String
    var textCustomPunctuation: String
}

class BaseTest: XCTestCase {
    private let params: [String] = [
        "male",
        "female"
    ]
    
    let accessKey = "{TESTING_ACCESS_KEY_HERE}"
    var orcas: [Orca] = []
    var testData: TestData
    
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
