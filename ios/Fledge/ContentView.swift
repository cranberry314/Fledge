import SwiftUI

// The main screen: the bubble level, the compass and the flip button.
// The drone controls come later.
struct ContentView: View {
    @State private var motion = Motion()
    @State private var spins = 0.0

    var body: some View {
        VStack(spacing: 24) {
            Text("🐣 Fledge")
                .font(.largeTitle.bold())

            LevelView(motion: motion)

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

#Preview {
    ContentView()
}
