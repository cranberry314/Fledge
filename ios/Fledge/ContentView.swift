import SwiftUI

// The main screen: the up/down slider, the bubble level, the compass, the
// turning ring and the flip button.
struct ContentView: View {
    @State private var motion = Motion()
    @State private var relay = Relay()
    @State private var spins = 0.0

    // The turning ring: where the finger touched, and where we think the
    // drone faces. Both in degrees: 0 = the way it faced at "Zero Level".
    @State private var target = 0.0
    @State private var facing = 0.0

    // The up/down slider: -100 (down) to 100 (up).
    @State private var slider = 0.0

    var body: some View {
        let flight = Flight(from: motion, facing: facing, target: target, slider: slider)
        let raised = motion.height - motion.holdHeight

        VStack(spacing: 12) {
            Text(relay.status)
                .font(.headline)

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
                    FlightText(flight: flight)
                    Button("🎯 Zero Level") {
                        motion.zeroLevel()
                        target = 0
                        facing = 0
                    }
                    .buttonStyle(.bordered)
                }

                CompassView(motion: motion)

                VStack(spacing: 12) {
                    TurnRing(target: $target, facing: facing)

                    Button {
                        withAnimation(.spring(duration: 0.6)) { spins += 360 }
                    } label: {
                        Text(flipButtonEmoji)
                            .font(.system(size: 40))
                            .rotationEffect(.degrees(spins))
                            .frame(width: 80, height: 80)
                            .background(flipButtonColor, in: Circle())
                    }
                    .buttonStyle(.plain)
                }
            }
        }
        .onAppear { motion.start() }
        .onDisappear { motion.stop() }
        .task {
            // 20 times a second: tell the relay what to do, and move our
            // guess of which way the drone faces.
            while !Task.isCancelled {
                let flight = Flight(from: motion, facing: facing, target: target, slider: slider)
                relay.send(flight)
                facing += flight.turn / 100 * droneTurnSpeed / 20
                try? await Task.sleep(for: .milliseconds(50))
            }
        }
    }
}

// What the drone would be told to do, in words.
struct FlightText: View {
    var flight: Flight

    var body: some View {
        Text(words())
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
            return "🛑 staying still"
        }
        return parts.joined(separator: "  ")
    }
}

#Preview {
    ContentView()
}
