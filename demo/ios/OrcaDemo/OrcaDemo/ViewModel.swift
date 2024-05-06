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
    case STREAM_OPEN
    case PROCESSING
    case SYNTHESIZED
    case PLAYING
    case ERROR
}

class ViewModel: ObservableObject {
    private let ACCESS_KEY = "" // Obtained from Picovoice Console (https://console.picovoice.ai)

    private var orca: Orca!
    private var orcaStream: Orca.OrcaStream!
    private var player: AudioPlayer = AudioPlayer()
    private var playerStream: AudioPlayerStream!
    private var previousText = ""
    private var subscriptions = Set<AnyCancellable>()

    private let audioFilePath = "temp.wav"
    private var audioFile: URL!
    
    private let NUM_AUDIO_WAIT_CHUNKS = 1
    
    @Published var errorMessage = ""
    @Published var state = UIState.INIT
    @Published var maxCharacterLimit: Int32 = 0
    @Published var sampleRate: Int32 = 0
    @Published var invalidTextMessage = ""
    
    init() {
        initialize()
    }

    public func initialize() {
        state = UIState.INIT
        do {
            try orca = Orca(accessKey: ACCESS_KEY, modelPath: "orca_params_female.pv")
            maxCharacterLimit = try orca.maxCharacterLimit
            sampleRate = try orca.sampleRate
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

    public func toggleStreaming() {
        if state == UIState.READY || state == UIState.STREAM_OPEN {
            if orcaStream == nil {
                do {
                    orcaStream = try orca.streamOpen()
                    self.state = UIState.STREAM_OPEN
                } catch {
                    self.errorMessage = "\(error.localizedDescription)"
                    self.state = UIState.ERROR
                }
            } else {
                orcaStream.close()
                orcaStream = nil
                self.state = UIState.READY
            }
        }
    }
    
    private func runStreamSynthesis(text: String) {
        playerStream = AudioPlayerStream(sampleRate: Double(self.sampleRate))
        
        let textStreamQueue = DispatchQueue(label: "text-stream-queue")
        let textStreamQueueConcurrent = DispatchQueue(label: "text-stream-queue-concurrent", attributes: .concurrent)
        var textStreamArray = [String]()
        let isTextStreamQueueActive = AtomicBool(false)
        
        func isTextStreamEmpty() -> Bool {
            return textStreamQueueConcurrent.sync {
                textStreamArray.isEmpty
            }
        }
        
        func getFromTextStream() -> String? {
            var word: String?
            textStreamQueueConcurrent.sync {
                if !textStreamArray.isEmpty {
                    word = textStreamArray.removeFirst()
                }
            }
            return word
        }
        
        func addToTextStream(word: String) {
            textStreamQueueConcurrent.async(flags: .barrier) {
                textStreamArray.append(word)
            }
        }
        
        let pcmStreamQueue = DispatchQueue(label: "pcm-stream-queue")
        let pcmStreamQueueConcurrent = DispatchQueue(label: "pcm-stream-queue-concurrent", attributes: .concurrent)
        var pcmStreamArray = [[Int16]]()
        let isPcmStreamQueueActive = AtomicBool(false)
        
        func isPcmStreamEmpty() -> Bool {
            return pcmStreamQueueConcurrent.sync {
                pcmStreamArray.isEmpty
            }
        }
        
        func getFromPcmStream() -> [Int16]? {
            var pcm: [Int16]?
            pcmStreamQueueConcurrent.sync {
                if !pcmStreamArray.isEmpty {
                    pcm = pcmStreamArray.removeFirst()
                }
            }
            return pcm
        }
        
        func addToPcmStream(pcm: [Int16]) {
            pcmStreamQueueConcurrent.async(flags: .barrier) {
                pcmStreamArray.append(pcm)
            }
        }
        
        let playStreamQueue = DispatchQueue(label: "play-stream-queue")
        let playStreamQueueLatch = DispatchSemaphore(value: 0)
        
        textStreamQueue.async {
            isTextStreamQueueActive.set(true)
            
            let words = text.split(separator: " ")
            for word in words {
                let wordWithSpace = String(word) + " "
                addToTextStream(word: wordWithSpace)
                usleep(100 * 1000)
            }
            
            isTextStreamQueueActive.set(false)
        }
        
        pcmStreamQueue.async {
            isPcmStreamQueueActive.set(true)
            var numIterations = 0
            var isFlipped = false
            
            while isTextStreamQueueActive.get() || !isTextStreamEmpty() {
                if !isTextStreamEmpty() {
                    do {
                        let word = getFromTextStream()
                        if word != nil {
                            let pcm = try self.orcaStream.synthesize(text: word!)
                            if pcm != nil {
                                addToPcmStream(pcm: pcm!)
                                if numIterations == self.NUM_AUDIO_WAIT_CHUNKS {
                                    playStreamQueueLatch.signal()
                                    isFlipped = true
                                }
                                numIterations += 1
                            }
                        }
                    } catch {
                        fatalError(error.localizedDescription)
                    }
                }
            }
            
            do {
                let pcm = try self.orcaStream.flush()
                if pcm != nil {
                    addToPcmStream(pcm: pcm!)
                    if !isFlipped {
                        playStreamQueueLatch.signal()
                    }
                }
            } catch {
                fatalError(error.localizedDescription)
            }
            
            isPcmStreamQueueActive.set(false)
        }
        
        playStreamQueue.async {
            playStreamQueueLatch.wait()
            
            while isPcmStreamQueueActive.get() || !isPcmStreamEmpty() {
                if !isPcmStreamEmpty() {
                    let pcm = getFromPcmStream()
                    self.playerStream.playPCM(pcm!)
                }
            }
        }
    }

    public func toggleSynthesize(text: String) {
        if state == UIState.STREAM_OPEN {
            runStreamSynthesis(text: text)
            return
        }
        
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
            
            var nonAllowedCharacters = [Character]()
            for i in 0..<text.count {
                let char = text[text.index(text.startIndex, offsetBy: i)]
                if !characters.contains(String(char)) && !nonAllowedCharacters.contains(char) {
                    nonAllowedCharacters.append(char)
                }
            }

            if nonAllowedCharacters.count > 0 {
                let characterString = nonAllowedCharacters.map { "\($0)" }.joined(separator: ", ")
                self.invalidTextMessage = "Text contains the following invalid characters: `\(characterString)`"
            } else {
                self.invalidTextMessage = ""
            }
        } catch {
            print(error.localizedDescription)
            self.errorMessage = "\(error.localizedDescription)"
            self.state = UIState.ERROR
        }
    }
}
