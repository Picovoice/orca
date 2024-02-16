//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

import Foundation
import XCTest

import Orca

class PerformanceTest: BaseTest {
    let iterationString: String = "{NUM_TEST_ITERATIONS}"
    let procThresholdString: String = "{PROC_PERFORMANCE_THRESHOLD_SEC}"

    override func setUp() {
        super.setUp()
        continueAfterFailure = false
    }

    func testPerformance() throws {
        try XCTSkipIf(procThresholdString == "{PROC_PERFORMANCE_THRESHOLD_SEC}")

        let numTestIterations = Int(iterationString) ?? 30
        let procPerformanceThresholdSec = Double(procThresholdString)

        try XCTSkipIf(procPerformanceThresholdSec == nil)

        let bundle = Bundle(for: type(of: self))

        for param in params {
            let modelPath: String = bundle.path(
                    forResource: "orca_params_\(param)",
                    ofType: "pv",
                    inDirectory: "test_resources/model_files")!
            var results: [Double] = []

            for i in 0...numTestIterations {
                var totalNSec = 0.0
                let orca = try? Orca(accessKey: accessKey, modelPath: modelPath)

                let before = CFAbsoluteTimeGetCurrent()
                let pcm = try? orca?.synthesize(text: self.testData!.test_sentences.text)
                let after = CFAbsoluteTimeGetCurrent()
                totalNSec += (after - before)

                // throw away first run to account for cold start
                if i > 0 {
                    results.append(totalNSec)
                }
                orca?.delete()
            }

            let avgNSec = results.reduce(0.0, +) / Double(numTestIterations)
            let avgSec = Double(round(avgNSec * 1000) / 1000)
            XCTAssertLessThanOrEqual(avgSec, procPerformanceThresholdSec!)
        }
    }
}
