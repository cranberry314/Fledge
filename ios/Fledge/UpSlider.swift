import SwiftUI

// A slider for up and down: drag up to climb, down to sink. Let go and it
// springs back to the middle, where the drone holds its height, just like
// the left stick on the remote.
struct UpSlider: View {
    @Binding var up: Double  // -100 (down) to 100 (up)

    let width = 50.0
    let height = 160.0

    var body: some View {
        let reach = (height - width) / 2

        ZStack {
            Capsule()
                .fill(upSliderColor.opacity(0.35))
            Circle()
                .fill(upSliderColor)
                .frame(width: width, height: width)
                .offset(y: -up / 100 * reach)
        }
        .frame(width: width, height: height)
        .contentShape(Rectangle())
        .gesture(
            DragGesture(minimumDistance: 0)
                .onChanged { drag in
                    // How far the finger has moved since it touched, so
                    // touching alone never makes the drone climb.
                    let moved = -drag.translation.height / reach * 100
                    up = min(max(moved, -100), 100)
                }
                .onEnded { _ in
                    withAnimation(.spring(duration: 0.3)) { up = 0 }
                }
        )
    }
}

#Preview {
    UpSlider(up: .constant(40))
}
