import Foundation
import AVFoundation

class AudioPlayerStream {
    private let engine = AVAudioEngine()
    private let playerNode = AVAudioPlayerNode()
    private let mixerNode = AVAudioMixerNode()

    private var pcmBuffers = [[Int16]]()
    private var isPlaying = false

    private let lock = NSLock()
    private var buffersScheduled = 0

    init(sampleRate: Double) throws {
        let audioSession = AVAudioSession.sharedInstance()
        try audioSession.setCategory(.playback, mode: .default)
        try audioSession.setActive(true)

        let format = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: sampleRate,
            channels: AVAudioChannelCount(1),
            interleaved: false)

        engine.attach(mixerNode)
        engine.connect(mixerNode, to: engine.outputNode, format: format)

        engine.attach(playerNode)
        engine.connect(playerNode, to: mixerNode, format: format)

        try engine.start()
    }

    func playStreamPCM(_ pcmData: [Int16], completion: @escaping (Bool) -> Void) {

        schedulePCM(pcm: pcmData, completion: completion)

        if !isPlaying {
            playerNode.play()
            isPlaying = true
            completion(true)
        }
    }

    private func schedulePCM(pcm: [Int16], completion: @escaping (Bool) -> Void) {
        let audioBuffer = AVAudioPCMBuffer(
            pcmFormat: playerNode.outputFormat(forBus: 0), frameCapacity: AVAudioFrameCount(pcm.count))!

        audioBuffer.frameLength = audioBuffer.frameCapacity
        let buf = audioBuffer.floatChannelData![0]
        for (index, sample) in pcm.enumerated() {
            buf[index] = Float32(sample) / Float32(Int16.max)
        }

        lock.lock()
        self.buffersScheduled += 1
        lock.unlock()

        playerNode.scheduleBuffer(audioBuffer) { [weak self] in
            self?.lock.lock()
            self?.buffersScheduled -= 1
            self?.lock.unlock()

            if self?.buffersScheduled == 0 {
                self?.isPlaying = false
                completion(false)
                self?.buffersScheduled = 0
            }
        }
    }

    func stopStreamPCM() {
        playerNode.stop()
        engine.stop()
        self.buffersScheduled = 0
    }
}
