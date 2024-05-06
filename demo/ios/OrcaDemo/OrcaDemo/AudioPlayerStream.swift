import Foundation
import AVFoundation

class AudioPlayerStream {
    private let audioEngine = AVAudioEngine()
    private let audioPlayerNode = AVAudioPlayerNode()
    private let audioMixerNode = AVAudioMixerNode()
    
    private var pcmBuffers = [[Int16]]()
    private var isPlaying = false
    
    init(sampleRate: Double) {
        let audioSession = AVAudioSession.sharedInstance()
        do {
            try audioSession.setCategory(.playAndRecord, mode: .default, options: .defaultToSpeaker)
            try audioSession.setActive(true)
        } catch {
            print(error)
        }
        
        let format = AVAudioFormat(commonFormat: .pcmFormatFloat32, sampleRate: sampleRate, channels: AVAudioChannelCount(1), interleaved: false)
        
        audioEngine.attach(audioMixerNode)
        audioEngine.connect(audioMixerNode, to: audioEngine.outputNode, format: format)
        
        audioEngine.attach(audioPlayerNode)
        audioEngine.connect(audioPlayerNode, to: audioMixerNode, format: format)
        
        do {
            try audioEngine.start()
        } catch {
            print("Error starting AVAudioEngine: \(error.localizedDescription)")
        }
    }
    
    func playPCM(_ pcmData: [Int16]) {
        pcmBuffers.append(pcmData)
        if !isPlaying {
            playNextPCMBuffer()
        }
    }
    
    private func playNextPCMBuffer() {
        guard let pcmData = pcmBuffers.first, !pcmData.isEmpty else {
            isPlaying = false
            return
        }
        pcmBuffers.removeFirst()
        
        let audioBuffer = AVAudioPCMBuffer(pcmFormat: audioPlayerNode.outputFormat(forBus: 0), frameCapacity: AVAudioFrameCount(pcmData.count))!
        
        audioBuffer.frameLength = audioBuffer.frameCapacity
        let buf = audioBuffer.floatChannelData![0]
        for (index, sample) in pcmData.enumerated() {
            buf[index] = Float32(sample) / 32767.0
        }

        audioPlayerNode.scheduleBuffer(audioBuffer) { [weak self] in
            self?.playNextPCMBuffer()
        }

        audioPlayerNode.play()
        isPlaying = true
    }
}
