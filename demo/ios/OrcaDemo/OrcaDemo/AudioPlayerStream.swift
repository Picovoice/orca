import Foundation
import AVFoundation

class AudioPlayerStream {
    private let engine = AVAudioEngine()
    private let playerNode = AVAudioPlayerNode()
    private let mixerNode = AVAudioMixerNode()
    
    private var pcmBuffers = [[Int16]]()
    private var isPlaying = false
    
    init(sampleRate: Double) throws {
        let audioSession = AVAudioSession.sharedInstance()
        try audioSession.setCategory(.playAndRecord, mode: .default, options: .defaultToSpeaker)
        try audioSession.setActive(true)
        
        let format = AVAudioFormat(commonFormat: .pcmFormatFloat32, sampleRate: sampleRate, channels: AVAudioChannelCount(1), interleaved: false)
        
        engine.attach(mixerNode)
        engine.connect(mixerNode, to: engine.outputNode, format: format)
        
        engine.attach(playerNode)
        engine.connect(playerNode, to: mixerNode, format: format)
        
        try engine.start()
    }
    
    func playStreamPCM(_ pcmData: [Int16], completion: @escaping (Bool) -> Void) {
        pcmBuffers.append(pcmData)
        if !isPlaying {
            playNextPCMBuffer(completion: completion)
        } else {
            completion(true)
        }
    }
    
    private func playNextPCMBuffer(completion: @escaping (Bool) -> Void) {
        guard let pcmData = pcmBuffers.first, !pcmData.isEmpty else {
            isPlaying = false
            completion(false)
            return
        }
        pcmBuffers.removeFirst()
        
        let audioBuffer = AVAudioPCMBuffer(pcmFormat: playerNode.outputFormat(forBus: 0), frameCapacity: AVAudioFrameCount(pcmData.count))!
        
        audioBuffer.frameLength = audioBuffer.frameCapacity
        let buf = audioBuffer.floatChannelData![0]
        for (index, sample) in pcmData.enumerated() {
            buf[index] = Float32(sample) / 32767.0
        }

        playerNode.scheduleBuffer(audioBuffer) { [weak self] in
            self?.playNextPCMBuffer(completion: completion)
        }

        playerNode.play()
        isPlaying = true
        completion(true)
    }

    func stopStreamPCM() {
        playerNode.stop()
        engine.stop()
    }
}
