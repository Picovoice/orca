//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

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
    private let ACCESS_KEY = "${YOUR_ACCESS_KEY_HERE}" // Obtained from Picovoice Console (https://console.picovoice.ai)

    private var orca: Orca!

    private var isSynthesizing = false
    private var isPlaying = false
    
    private var player: AudioPlayer!

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
        if isPlaying {
            toggleSynthesizeOff()
        } else {
            toggleSynthesizeOn(text: text)
        }
    }

    public func toggleSynthesizeOff() {
        state = UIState.SYNTHESIZED
    }

    public func toggleSynthesizeOn(text: String) {
        state = UIState.PROCESSING
        
        do {
            let pcm = try orca.synthesize(text: text)
            player = AudioPlayer()
            try player.savePCMToAudioFile(pcm: pcm, filePath: "temp.wav", sampleRate: Double(orca.sampleRate))
            try player.playAudioFile(filePath: "temp.wav")
            
            state = UIState.PLAYING
        } catch {
            errorMessage = "\(error.localizedDescription)"
            state = UIState.READY
        }
    }
}
