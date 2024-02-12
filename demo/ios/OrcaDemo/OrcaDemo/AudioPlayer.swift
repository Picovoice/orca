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
    let audioFilePath = "temp.wav"
    let sampleRate = 16000.0
    
    var audioFile: URL?
    var player: AVAudioPlayer?
    var onComplete: ((Bool) -> Void)?
    
    func play(pcm: [Int16]? = nil, onComplete: ((Bool) -> Void)? = nil) throws {
        let audioDir = try FileManager.default.url(
                        for: .documentDirectory,
                        in: .userDomainMask,
                        appropriateFor: nil,
                        create: false)
        audioFile = audioDir.appendingPathComponent(audioFilePath)
        self.onComplete = onComplete
        
        if pcm != nil {
            try savePCMToAudioFile(pcm: pcm!, sampleRate: sampleRate)
        }
        try playAudioFile()
    }
    
    func stop() {
        player?.stop()
    }
    
    private func savePCMToAudioFile(pcm: [Int16], sampleRate: Double) throws {
        guard let format = AVAudioFormat(commonFormat: .pcmFormatInt16,
                                         sampleRate: sampleRate,
                                         channels: 1,
                                         interleaved: false) else {
            throw AudioPlayerError("Failed to get Audio Format")
        }
        
        let audioWav = try openOutputWav(fileUrl: audioFile!, format: format)
        try writePcmToWav(pcm: pcm, format: format, audioWav: audioWav)
    }
    
    private func playAudioFile() throws {
        player = try AVAudioPlayer(contentsOf: audioFile!)
        player?.delegate = self
        player?.play()
    }
    
    private func writePcmToWav(pcm: [Int16], format: AVAudioFormat, audioWav: AVAudioFile?) throws {
        let writeBuffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: AVAudioFrameCount(pcm.count))!
        memcpy(writeBuffer.int16ChannelData![0], pcm, pcm.count * 2)
        writeBuffer.frameLength = UInt32(pcm.count)

        do {
            try audioWav?.write(from: writeBuffer)
        } catch {
            throw AudioPlayerError("\(error.localizedDescription)")
        }
    }
    
    private func openOutputWav(fileUrl: URL, format: AVAudioFormat) throws -> AVAudioFile? {
        do {
            if FileManager.default.fileExists(atPath: fileUrl.path) {
                try FileManager.default.removeItem(at: fileUrl)
            }
            return try AVAudioFile(
                forWriting: fileUrl,
                settings: format.settings,
                commonFormat: .pcmFormatInt16,
                interleaved: true)
        } catch {
            throw AudioPlayerError("\(error.localizedDescription)")
        }
    }
    
    public func audioPlayerDidFinishPlaying(_ player: AVAudioPlayer, successfully flag: Bool) {
        self.onComplete?(flag)
    }
}
