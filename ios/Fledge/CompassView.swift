import SwiftUI

// A compass: the dial turns so N always points north, and the bird above
// it points the way the phone points. Below it: how far the phone has
// turned since "Zero Level".
struct CompassView: View {
    var motion: Motion

    let size = 160.0
    let letters = ["N", "E", "S", "W"]

    var body: some View {
        VStack(spacing: 8) {
            // The way the phone points: always straight up the screen.
            Text(compassBird)
                .font(.system(size: 40))
                .rotationEffect(.degrees(compassBirdTurn))

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
            .frame(width: size, height: size)

            Text("Pointing \(Int(motion.heading))° \(directionName(motion.heading))")
                .font(.headline)

            Text(turnText(motion.turn))
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
