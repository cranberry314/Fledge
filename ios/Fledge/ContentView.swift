import SwiftUI

// The main screen: the bubble level, the compass and the flip button.
// The drone controls come later.
struct ContentView: View {
    @State private var motion = Motion()
    @State private var spins = 0.0

    var body: some View {
        // Held sideways: the level, the compass and the flip button in a row.
        HStack(spacing: 40) {
            VStack(spacing: 16) {
                LevelView(motion: motion)
                FlightText(flight: Flight(from: motion))
                Button("📍 Start here") {
                    motion.startHere()
                }
                .buttonStyle(.bordered)
            }

            CompassView(motion: motion)

            Button {
                withAnimation(.spring(duration: 0.6)) { spins += 360 }
            } label: {
                Text(flipButtonEmoji)
                    .font(.system(size: 50))
                    .rotationEffect(.degrees(spins))
                    .frame(width: 100, height: 100)
                    .background(flipButtonColor, in: Circle())
            }
            .buttonStyle(.plain)
        }
        .onAppear { motion.start() }
        .onDisappear { motion.stop() }
    }
}

// What the drone would be told to do, in words.
struct FlightText: View {
    var flight: Flight

    var body: some View {
        Text(words())
            .font(.title3.bold())
    }

    func words() -> String {
        var parts: [String] = []
        if flight.forward > 0 { parts.append("⬆️ forward \(Int(flight.forward))%") }
        if flight.forward < 0 { parts.append("⬇️ back \(Int(-flight.forward))%") }
        if flight.right > 0 { parts.append("➡️ right \(Int(flight.right))%") }
        if flight.right < 0 { parts.append("⬅️ left \(Int(-flight.right))%") }
        if parts.isEmpty {
            return "🛑 staying still"
        }
        return parts.joined(separator: "  ")
    }
}

#Preview {
    ContentView()
}
