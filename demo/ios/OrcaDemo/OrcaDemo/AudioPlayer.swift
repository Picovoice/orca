//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

import AVFoundation
import Foundation
import Orca

public class AudioPlayerError: LocalizedError {
    private let message: String

    public init (_ message: String) {
        self.message = message
    }

    public var errorDescription: String? {
        return message
    }

    public var name: String {
        get {
            return String(describing: type(of: self))
        }
    }
}

public class AudioPlayer: NSObject, AVAudioPlayerDelegate {
    var player: AVAudioPlayer?
    var onComplete: ((Bool) -> Void)?
    
    func play(audioFile: URL, onComplete: ((Bool) -> Void)? = nil) throws {
        self.onComplete = onComplete
        try playAudioFile(audioFile: audioFile)
    }

    func stop() {
        player?.stop()
    }

    private func playAudioFile(audioFile: URL) throws {
        let audioSession = AVAudioSession.sharedInstance()
        try audioSession.setCategory(AVAudioSession.Category.playback)
        try audioSession.setActive(true)

        player = try AVAudioPlayer(contentsOf: audioFile)
        player?.delegate = self
        player?.setVolume(1.0, fadeDuration: 0.05)
        player?.play()
    }

    public func audioPlayerDidFinishPlaying(_ player: AVAudioPlayer, successfully flag: Bool) {
        self.onComplete?(flag)
    }
}
