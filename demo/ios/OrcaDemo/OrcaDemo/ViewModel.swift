//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

import Combine
import Foundation
import Orca

enum UIState {
    case INIT
    case READY
    case PROCESSING
    case SYNTHESIZED
    case PLAYING
    case ERROR
}

class ViewModel: ObservableObject {
    private let ACCESS_KEY = "PI5vbMlGH3K4+HgfM8hk9eIDF6i7CfZMQsn/9MmPrczi7sCfVioy6w==" // Obtained from Picovoice Console (https://console.picovoice.ai)

    private var orca: Orca!
    private var player: AudioPlayer = AudioPlayer()
    private var previousText = ""
    private var subscriptions = Set<AnyCancellable>()

    private let audioFilePath = "temp.wav"
    private var audioFile: URL!

    @Published var errorMessage = ""
    @Published var state = UIState.INIT
    @Published var maxCharacterLimit = Orca.maxCharacterLimit
    @Published var invalidTextMessage = ""

    init() {
        initialize()
    }

    public func initialize() {
        state = UIState.INIT
        do {
            try orca = Orca(accessKey: ACCESS_KEY, modelPath: "orca_params_female.pv")
            state = UIState.READY

            let audioDir = try FileManager.default.url(
                            for: .documentDirectory,
                            in: .userDomainMask,
                            appropriateFor: nil,
                            create: false)
            audioFile = audioDir.appendingPathComponent(audioFilePath)

            return
        } catch is OrcaActivationError {
            errorMessage = "ACCESS_KEY activation error"
        } catch is OrcaActivationRefusedError {
            errorMessage = "ACCESS_KEY activation refused"
        } catch is OrcaActivationLimitError {
            errorMessage = "ACCESS_KEY reached its limit"
        } catch is OrcaActivationThrottledError {
            errorMessage = "ACCESS_KEY is throttled"
        } catch {
            errorMessage = "\(error.localizedDescription)"
        }

        state = UIState.ERROR
    }

    public func destroy() {
        orca.delete()
    }

    public func toggleSynthesize(text: String) {
        if state == UIState.PLAYING {
            toggleSynthesizeOff()
        } else {
            toggleSynthesizeOn(text: text)
        }
    }

    public func toggleSynthesizeOff() {
        player.stop()
        state = UIState.READY
    }

    public func toggleSynthesizeOn(text: String) {
        state = UIState.PROCESSING

        let _: () = Future<Void, Error> { promise in
            DispatchQueue.global().async {
                do {
                    if self.previousText != text {
                        try self.orca.synthesizeToFile(text: text, outputURL: self.audioFile)
                        self.previousText = text
                    }
                    promise(.success(()))
                } catch {
                    promise(.failure(error))
                }
            }
        }
            .receive(on: DispatchQueue.main)
            .sink(receiveCompletion: { completion in
                switch completion {
                case .failure(let error):
                    self.errorMessage = "\(error.localizedDescription)"
                    self.state = UIState.ERROR
                case .finished:
                    break
                }
            }, receiveValue: { _ in
                do {
                    try self.player.play(audioFile: self.audioFile) { _ in
                        self.state = UIState.READY
                    }

                    self.state = UIState.PLAYING
                } catch {
                    self.errorMessage = "\(error.localizedDescription)"
                    self.state = UIState.ERROR
                }
            })
            .store(in: &subscriptions)
    }

    public func isValid(text: String) {
        do {
            let characters = try orca.validCharacters
            let regex = try NSRegularExpression(
                pattern: "[^\(characters.joined(separator: ""))\\s{}|']",
                options: .caseInsensitive)
            let range = NSRange(text.startIndex..<text.endIndex, in: text)
            let matches = regex.matches(in: text, range: range)

            let unexpectedCharacters = NSOrderedSet(array: matches.map {
                String(text[Range($0.range, in: text)!])
            })

            if unexpectedCharacters.count > 0 {
                let characterString = unexpectedCharacters.array.map { "\($0)" }.joined(separator: ", ")
                self.invalidTextMessage = "Text contains the following invalid characters: `\(characterString)`"
            } else {
                self.invalidTextMessage = ""
            }
        } catch {
            self.errorMessage = "\(error.localizedDescription)"
            self.state = UIState.ERROR
        }
    }
}
