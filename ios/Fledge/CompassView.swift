import SwiftUI

// A compass: the dial turns so N always points north, and the arrow at the
// top shows which way the phone points. Below it: how far the phone has
// turned since "Start here". That turn is what will turn the drone.
struct CompassView: View {
    var motion: Motion

    let size = 180.0
    let letters = ["N", "E", "S", "W"]

    var body: some View {
        VStack(spacing: 8) {
            ZStack {
                // The dial, turned the opposite way to the phone.
                ZStack {
                    Circle()
                        .stroke(.secondary, lineWidth: 3)
                    ForEach(0..<4) { i in
                        Text(letters[i])
                            .font(.title2.bold())
                            .foregroundStyle(i == 0 ? northColor : .primary)
                            .offset(y: -size / 2 + 20)
                            .rotationEffect(.degrees(Double(i) * 90))
                    }
                }
                .rotationEffect(.degrees(-motion.heading))

                // The way the phone points: always straight up the screen.
                Image(systemName: "location.north.fill")
                    .font(.largeTitle)
            }
            .frame(width: size, height: size)

            Text("Pointing \(Int(motion.heading))° \(directionName(motion.heading))")
                .font(.headline)

            HStack {
                Text(turnText(motion.turn))
                Button("Start here") {
                    motion.startHere()
                }
                .buttonStyle(.bordered)
            }
        }
    }

    // 0 -> "N", 45 -> "NE", 90 -> "E", and so on.
    func directionName(_ heading: Double) -> String {
        let names = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]
        return names[Int((heading + 22.5) / 45) % 8]
    }

    func turnText(_ turn: Double) -> String {
        if turn > 2 {
            return "↻ turned \(Int(turn))° right"
        }
        if turn < -2 {
            return "↺ turned \(Int(-turn))° left"
        }
        return "↑ facing the start"
    }
}

#Preview {
    CompassView(motion: Motion())
}
