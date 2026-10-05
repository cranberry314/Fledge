import SwiftUI

// The main screen: the buttons along the top, then the up/down slider,
// the bubble level, the compass, the turning ring and the flip button.
struct ContentView: View {
    @State private var motion = Motion()
    @State private var relay = Relay()
    @State private var buttons = Buttons()
    @State private var spins = 0.0

    // Fly / Stop. While stopped, the drone is told to stay still, whatever
    // the phone is doing.
    @State private var flying = false

    // The turning ring: where the finger touched, and where we think the
    // drone faces. Both in degrees: 0 = the way it faced at "Zero Level".
    @State private var target = 0.0
    @State private var facing = 0.0

    // The up/down slider: -100 (down) to 100 (up).
    @State private var slider = 0.0

    // What the flip button just did, shown for 2 seconds.
    @State private var flipMessage: String?

    var body: some View {
        let raised = motion.height - motion.holdHeight

        VStack(spacing: 12) {
            ButtonBar(flying: flying, buttons: buttons, status: relay.status,
                      fly: fly, motorsOff: motorsOff)

            // Held sideways: everything in a row. Up/down under the left
            // thumb and turning under the right, like the remote.
            HStack(spacing: 24) {
                VStack(spacing: 6) {
                    Text("⬆️")
                    UpSlider(up: $slider)
                    Text("⬇️")
                    Text("📏 \(Int(raised)) cm")
                        .font(.caption)
                }

                VStack(spacing: 16) {
                    LevelView(motion: motion)
                    FlightText(flight: flight(), note: note())
                    Button("🎯 Zero Level") {
                        zeroLevel()
                    }
                    .buttonStyle(.bordered)
                }

                CompassView(motion: motion)

                VStack(spacing: 12) {
                    TurnRing(target: $target, facing: facing)

                    Button(action: flip) {
                        Text(flipButtonEmoji)
                            .font(.system(size: 40))
                            .rotationEffect(.degrees(spins))
                            .frame(width: 80, height: 80)
                            .background(flipButtonColor.opacity(flying ? 1 : 0.4), in: Circle())
                    }
                    .buttonStyle(.plain)
                    .disabled(!flying)
                }
            }
        }
        .onAppear { motion.start() }
        .onDisappear { motion.stop() }
        .task {
            // 20 times a second: tell the relay what to do, and move our
            // guess of which way the drone faces.
            while !Task.isCancelled {
                let flight = flight()
                relay.send(flight, buttons)
                facing += flight.turn / 100 * droneTurnSpeed / 20
                try? await Task.sleep(for: .milliseconds(50))
            }
        }
    }

    // What the drone should do right now.
    func flight() -> Flight {
        if !flying {
            return Flight()  // stay still
        }
        return Flight(from: motion, facing: facing, target: target, slider: slider)
    }

    // A message instead of the flight words, when there is one.
    func note() -> String? {
        if !flying { return "✋ stopped" }
        return flipMessage
    }

    func zeroLevel() {
        motion.zeroLevel()
        target = 0
        facing = 0
    }

    // ✈️ Fly starts from however the phone is held right now.
    func fly() {
        if !flying {
            zeroLevel()
        }
        flying.toggle()
    }

    func motorsOff() {
        buttons.give(.motorsOff)
        flying = false
    }

    // 🤸 Flip the way the drone is flying: the way the phone is tipped
    // most. If it isn't tipped, flip forward.
    func flip() {
        let flight = flight()
        var way = ""
        if abs(flight.right) > abs(flight.forward) {
            buttons.give(flight.right > 0 ? .flipRight : .flipLeft)
            way = flight.right > 0 ? "right ➡️" : "left ⬅️"
        } else {
            buttons.give(flight.forward < 0 ? .flipBack : .flipForward)
            way = flight.forward < 0 ? "back ⬇️" : "forward ⬆️"
        }
        withAnimation(.spring(duration: 0.6)) { spins += 360 }

        flipMessage = "🤸 flipping \(way)!"
        Task {
            try? await Task.sleep(for: .seconds(2))
            flipMessage = nil
        }
    }
}

// The row of buttons along the top.
struct ButtonBar: View {
    var flying: Bool
    var buttons: Buttons
    var status: String
    var fly: () -> Void
    var motorsOff: () -> Void

    var body: some View {
        HStack(spacing: 10) {
            Button(action: fly) {
                Text(flying ? "✈️ Flying" : "✋ Stopped")
                    .font(.headline)
                    .foregroundStyle(.primary)
                    .padding(.horizontal, 16)
                    .padding(.vertical, 8)
                    .background(flying ? flyColor : Color.gray.opacity(0.3), in: Capsule())
            }

            // Take off / land.
            Button { buttons.give(.takeOffOrLand) } label: {
                Labeled(emoji: "🛫", words: "Take off\nor land")
            }

            // Lights on / off.
            Button { buttons.lights.toggle() } label: {
                Labeled(emoji: "💡", words: buttons.lights ? "Lights on" : "Lights off")
            }
            .opacity(buttons.lights ? 1 : 0.3)

            // Slow / fast.
            Button { buttons.fast.toggle() } label: {
                Labeled(emoji: buttons.fast ? "🐇" : "🐢", words: buttons.fast ? "Fast" : "Slow")
            }

            // The drone's level calibration (not the phone's: that is
            // "Zero Level"). The drone must sit flat on the ground, so
            // this only works while stopped.
            Button { buttons.give(.calibrate) } label: {
                Labeled(emoji: "📐", words: "Calibrate\ndrone")
            }
            .disabled(flying)
            .opacity(flying ? 0.3 : 1)

            Text(status)
                .font(.caption)
                .frame(maxWidth: .infinity)

            // The emergency stop: the motors stop and the drone falls.
            Button(action: motorsOff) {
                Text("🛑 Motors off")
                    .font(.headline)
                    .foregroundStyle(.white)
                    .padding(.horizontal, 16)
                    .padding(.vertical, 8)
                    .background(.red, in: Capsule())
            }
        }
        .buttonStyle(.plain)
        .font(.title2)
        .padding(.horizontal)
    }
}

// A picture with a word or two under it, so you know what it does.
struct Labeled: View {
    var emoji: String
    var words: String

    var body: some View {
        VStack(spacing: 2) {
            Text(emoji)
            Text(words)
                .font(.caption2)
                .multilineTextAlignment(.center)
        }
    }
}

// What the drone would be told to do, in words.
struct FlightText: View {
    var flight: Flight
    var note: String?

    var body: some View {
        Text(note ?? words())
            .font(.title3.bold())
            .multilineTextAlignment(.center)
            .frame(width: 220)
    }

    func words() -> String {
        var parts: [String] = []
        if flight.forward > 0 { parts.append("⬆️ forward \(Int(flight.forward))%") }
        if flight.forward < 0 { parts.append("⬇️ back \(Int(-flight.forward))%") }
        if flight.right > 0 { parts.append("➡️ right \(Int(flight.right))%") }
        if flight.right < 0 { parts.append("⬅️ left \(Int(-flight.right))%") }
        if flight.turn > 0 { parts.append("↻ spin \(Int(flight.turn))%") }
        if flight.turn < 0 { parts.append("↺ spin \(Int(-flight.turn))%") }
        if flight.up > 0 { parts.append("⤴️ up \(Int(flight.up))%") }
        if flight.up < 0 { parts.append("⤵️ down \(Int(-flight.up))%") }
        if parts.isEmpty {
            return "😌 staying still"
        }
        return parts.joined(separator: "  ")
    }
}

#Preview {
    ContentView()
}
