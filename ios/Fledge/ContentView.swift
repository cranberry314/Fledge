import SwiftUI

// The main screen. For now it only shows the flip button, so we can check
// that the app builds, installs on the phone, and reads Tuning.swift.
// The drone controls come later.
struct ContentView: View {
    @State private var spins = 0.0

    var body: some View {
        VStack(spacing: 40) {
            Text("🐣 Fledge")
                .font(.largeTitle.bold())

            Button {
                withAnimation(.spring(duration: 0.6)) { spins += 360 }
            } label: {
                Text(flipButtonEmoji)
                    .font(.system(size: 80))
                    .rotationEffect(.degrees(spins))
                    .frame(width: 160, height: 160)
                    .background(flipButtonColor, in: Circle())
            }
            .buttonStyle(.plain)
        }
    }
}

#Preview {
    ContentView()
}
