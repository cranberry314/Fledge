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

    // Flipping: press 🤸, then tip the phone the way to flip. Afterwards
    // the drone stays still until the phone is level again, so it doesn't
    // fly off in the direction of the flip.
    @State private var flipReady = false
    @State private var flipReadyAt = Date()
    @State private var levelAfterFlip = false

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

                    Button {
                        flipReady.toggle()
                        flipReadyAt = Date()
                    } label: {
                        Text(flipButtonEmoji)
                            .font(.system(size: 40))
                            .rotationEffect(.degrees(spins))
                            .frame(width: 80, height: 80)
                            .background(flipButtonColor.opacity(flipReady ? 1 : 0.6), in: Circle())
                            .overlay(Circle().stroke(.primary, lineWidth: flipReady ? 4 : 0))
                    }
                    .buttonStyle(.plain)
                    .disabled(!flying)
                }
            }
        }
        .onAppear { motion.start() }
        .onDisappear { motion.stop() }
        .task {
            // 20 times a second: check for a flip, tell the relay what to
            // do, and move our guess of which way the drone faces.
            while !Task.isCancelled {
                checkFlip()
                let flight = flight()
                relay.send(flight, buttons)
                facing += flight.turn / 100 * droneTurnSpeed / 20
                try? await Task.sleep(for: .milliseconds(50))
            }
        }
    }

    // What the drone should do right now.
    func flight() -> Flight {
        if !flying || flipReady || levelAfterFlip {
            return Flight()  // stay still
        }
        return Flight(from: motion, facing: facing, target: target, slider: slider)
    }

    // A message instead of the flight words, when there is one.
    func note() -> String? {
        if !flying { return "✋ stopped" }
        if flipReady { return "🤸 tip the phone to flip!" }
        if levelAfterFlip { return "🤸 now hold the phone level" }
        return nil
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
        flipReady = false
    }

    func motorsOff() {
        buttons.give(.motorsOff)
        flying = false
        flipReady = false
    }

    func checkFlip() {
        let tipForward = motion.tiltForward - motion.holdForward
        let tipRight = motion.tiltRight - motion.holdRight

        if levelAfterFlip {
            if abs(tipForward) < tiltDeadZone && abs(tipRight) < tiltDeadZone {
                levelAfterFlip = false
            }
            return
        }
        if !flipReady {
            return
        }
        if Date().timeIntervalSince(flipReadyAt) > 5 {
            flipReady = false  // waited too long: no flip
            return
        }
        if max(abs(tipForward), abs(tipRight)) < flipTip {
            return
        }

        // Flip the way the phone is tipped most.
        if abs(tipForward) >= abs(tipRight) {
            buttons.give(tipForward > 0 ? .flipForward : .flipBack)
        } else {
            buttons.give(tipRight > 0 ? .flipRight : .flipLeft)
        }
        flipReady = false
        levelAfterFlip = true
        withAnimation(.spring(duration: 0.6)) { spins += 360 }
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
            Button("🛫") { buttons.give(.takeOffOrLand) }

            // Lights on / off.
            Button("💡") { buttons.lights.toggle() }
                .opacity(buttons.lights ? 1 : 0.3)

            // Slow / fast.
            Button(buttons.fast ? "🐇" : "🐢") { buttons.fast.toggle() }

            // Level calibration: only on the ground, so only while stopped.
            Button("📐") { buttons.give(.calibrate) }
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
