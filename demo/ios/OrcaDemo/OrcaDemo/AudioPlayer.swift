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

public class AudioPlayer {
    var player: AVAudioPlayer?
    
    private func writePcmToWav(pcm: [Int16], format: AVAudioFormat, audioFile: AVAudioFile?) throws {
        let writeBuffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: AVAudioFrameCount(pcm.count))!
        memcpy(writeBuffer.int16ChannelData![0], pcm, pcm.count * 2)
        writeBuffer.frameLength = UInt32(pcm.count)

        do {
            try audioFile?.write(from: writeBuffer)
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
    
    func savePCMToAudioFile(pcm: [Int16], filePath: String, sampleRate: Double) throws {
        guard let format = AVAudioFormat(commonFormat: .pcmFormatInt16,
                                         sampleRate: sampleRate,
                                         channels: 1,
                                         interleaved: false) else {
            throw AudioPlayerError("Failed to get Audio Format")
        }
        
        let outputDir = try FileManager.default.url(
                        for: .documentDirectory,
                        in: .userDomainMask,
                        appropriateFor: nil,
                        create: false)
        let outputUrl = outputDir.appendingPathComponent(filePath)
        let outputWav = try openOutputWav(fileUrl: outputUrl, format: format)
        try writePcmToWav(pcm: pcm, format: format, audioFile: outputWav)
    }
    
    func playAudioFile(filePath: String) throws {
        let audioDir = try FileManager.default.url(
                        for: .documentDirectory,
                        in: .userDomainMask,
                        appropriateFor: nil,
                        create: false)
        let audioFile = audioDir.appendingPathComponent(filePath)
        
        player = try AVAudioPlayer(contentsOf: audioFile)
        player?.setVolume(1.0, fadeDuration: 0.05)
        player?.play()
    }
}
