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
    case SYNTHESIZE_ERROR
    case SYNTHESIZED
    case PLAYING
    case ERROR
}

class ViewModel: ObservableObject {
    private let ACCESS_KEY = "{YOUR_ACCESS_KEY_HERE}" // Obtained from Picovoice Console (https://console.picovoice.ai)

    private var orca: Orca!
    private var player: AudioPlayer = AudioPlayer()
    private var previousText = ""
    private var subscriptions = Set<AnyCancellable>()

    @Published var synthesizeError = ""
    @Published var errorMessage = ""
    @Published var state = UIState.INIT

    init() {
        initialize()
    }

    public func initialize() {
        state = UIState.INIT
        do {
            try orca = Orca(accessKey: ACCESS_KEY)
            state = UIState.READY
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
        
        let _: () = Future<[Int16]?, Error> { promise in
            DispatchQueue.global().async {
                do {
                    var pcm: [Int16]?
                    if self.previousText != text {
                        pcm = try self.orca.synthesize(text: text)
                        self.previousText = text
                    }
                    promise(.success(pcm))
                } catch {
                    promise(.failure(error))
                }
            }
        }
            .receive(on: DispatchQueue.main)
            .sink(receiveCompletion: { completion in
                switch completion {
                    case .failure(let error):
                        self.synthesizeError = "\(error.localizedDescription)"
                        self.state = UIState.SYNTHESIZE_ERROR
                    case .finished:
                        break
                }
            }, receiveValue: { value in
                do {
                    try self.player.play(pcm: value) { _ in
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
}
