//
//  Copyright 2024 Picovoice Inc.
//  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
//  file accompanying this source.
//  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//  specific language governing permissions and limitations under the License.
//

import SwiftUI

struct ContentView: View {
    @StateObject var viewModel = ViewModel()
    @State private var text = ""
    @State private var isTextFocused = false

    let activeBlue = Color(red: 55 / 255, green: 125 / 255, blue: 1, opacity: 1)
    let dangerRed = Color(red: 1, green: 14 / 255, blue: 14 / 255, opacity: 1)
    let lightGray = Color(red: 247 / 255, green: 247 / 255, blue: 247 / 255, opacity: 1)

    var body: some View {
        let streamingMode = viewModel.state == .STREAM_OPEN || viewModel.state == .STREAM_PLAYING
        let interactionDisabled =
            !viewModel.errorMessage.isEmpty || viewModel.state == .PROCESSING
            || viewModel.state == .INIT || (!streamingMode && !viewModel.invalidTextMessage.isEmpty)
        let toggleDisabled = interactionDisabled || viewModel.state == .STREAM_PLAYING
        let buttonDisabled = toggleDisabled || text.isEmpty

        GeometryReader { _ in
            VStack(spacing: 10) {
                Toggle(
                    isOn: Binding(
                        get: { streamingMode },
                        set: { _ in viewModel.toggleStreaming() }
                    ),
                    label: { Text("Streaming Synthesis") }
                )
                .disabled(toggleDisabled)
                .onChange(of: streamingMode) { _ in text = "" }
                .foregroundColor(Color.black)

                if viewModel.state == .STREAM_PLAYING {
                    GeometryReader { geometry in
                        ScrollView {
                            Text(viewModel.textStream)
                                .transparentScrolling()
                                .padding()
                                .frame(minWidth: 0,
                                       maxWidth: .infinity,
                                       minHeight: geometry.size.height,
                                       maxHeight: .infinity,
                                       alignment: .topLeading)
                                .font(.title3)
                                .background(lightGray)
                                .foregroundColor(Color.black)
                        }
                    }
                } else {
                    GeometryReader { geometry in
                        VStack {
                            ScrollView {
                                ZStack(alignment: .topLeading) {
                                    TextEditor(text: $text)
                                        .transparentScrolling()
                                        .padding()
                                        .frame(minWidth: 0,
                                               maxWidth: .infinity,
                                               minHeight: geometry.size.height,
                                               maxHeight: .infinity)
                                        .font(.title3)
                                        .background(lightGray)
                                        .foregroundColor(Color.black)
                                        .onChange(of: text) { _ in
                                            text = String(text.prefix(Int(exactly: viewModel.maxCharacterLimit)!))
                                            viewModel.isValid(text: text)
                                        }
                                        .disabled(viewModel.state == .PLAYING)

                                    if text.count == 0 {
                                        Text("Enter any text to be synthesized")
                                            .padding(25)
                                            .font(.title3)
                                            .foregroundColor(Color.gray)
                                    }
                                }
                            }

                            Text("\(text.count) / \(viewModel.maxCharacterLimit)")
                                .font(.footnote)
                                .frame(maxWidth: .infinity, alignment: .trailing)
                                .foregroundColor(Color.gray)
                        }
                    }
                }

                if streamingMode {
                    if viewModel.state == .STREAM_OPEN && !viewModel.streamInvalidTextMessage.isEmpty {
                        Text(viewModel.streamInvalidTextMessage)
                            .padding()
                            .font(.body)
                            .foregroundColor(Color.gray)
                    } else {
                        Text(viewModel.streamHelperText)
                            .padding()
                            .font(.body)
                            .foregroundColor(Color.black)
                    }
                } else if viewModel.state == .INIT || viewModel.state == .READY {
                    if viewModel.invalidTextMessage.isEmpty {
                        Text("Enter text and press synthesize")
                            .padding()
                            .font(.body)
                            .foregroundColor(Color.black)
                    } else {
                        Text(viewModel.invalidTextMessage)
                            .padding()
                            .font(.body)
                            .foregroundColor(Color.red)
                    }
                } else if viewModel.state == .PROCESSING {
                    Text("Processing text...")
                        .padding()
                        .font(.body)
                        .foregroundColor(Color.black)
                } else if viewModel.state == .PLAYING {
                    Text("Playing audio")
                        .padding()
                        .font(.body)
                        .foregroundColor(Color.black)
                } else {
                    Text(viewModel.errorMessage)
                        .padding()
                        .foregroundColor(Color.white)
                        .frame(maxWidth: .infinity)
                        .background(dangerRed)
                        .font(.body)
                        .opacity(viewModel.errorMessage.isEmpty ? 0 : 1)
                        .cornerRadius(10)
                }

                Button(
                    action: {
                        viewModel.toggleSynthesize(text: text)
                    },
                    label: {
                        Text(viewModel.state == .PLAYING ? "Stop" : "Synthesize")
                        .padding()
                        .frame(minWidth: 200)
                        .background(buttonDisabled ? Color.gray : activeBlue)
                        .foregroundColor(Color.white)
                        .font(.largeTitle)
                    }
                )
                .disabled(buttonDisabled)
            }
            .onReceive(
                NotificationCenter.default.publisher(
                    for: UIApplication.willEnterForegroundNotification),
                perform: { _ in
                    viewModel.initialize()
                }
            )
            .onReceive(
                NotificationCenter.default.publisher(
                    for: UIApplication.didEnterBackgroundNotification),
                perform: { _ in
                    viewModel.destroy()
                }
            )
            .padding()
            .frame(minWidth: 0, maxWidth: .infinity, minHeight: 0)
            .background(Color.white)
            .onTapGesture {
                hideKeyboard()
            }
        }
    }
}

public extension View {
    func transparentScrolling() -> some View {
        if #available(iOS 16.0, *) {
            return scrollContentBackground(.hidden)
        } else {
            return onAppear {
                UITextView.appearance().backgroundColor = .clear
            }
        }
    }

    func hideKeyboard() {
        let resign = #selector(UIResponder.resignFirstResponder)
        UIApplication.shared.sendAction(resign, to: nil, from: nil, for: nil)
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
