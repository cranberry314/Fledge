import SwiftUI

// A bubble level: the bubble floats to the high side of the phone, like
// the bubble in a builder's level, and turns green when the phone is level.
struct LevelView: View {
    var motion: Motion

    let size = 160.0
    let bubbleSize = 36.0

    var body: some View {
        // Level when both tilts are inside the dead zone from Tuning.swift.
        let level = abs(motion.tiltRight) < tiltDeadZone
            && abs(motion.tiltForward) < tiltDeadZone

        // 45 degrees of tip moves the bubble to the edge (at sensitivity 1).
        var x = -motion.tiltRight / 45 * tiltSensitivity
        var y = motion.tiltForward / 45 * tiltSensitivity
        let distance = (x * x + y * y).squareRoot()
        if distance > 1 {  // keep the bubble inside the circle
            x /= distance
            y /= distance
        }
        let reach = (size - bubbleSize) / 2

        return VStack(spacing: 8) {
            ZStack {
                Circle()
                    .stroke(.secondary, lineWidth: 3)
                Circle()
                    .stroke(.secondary, lineWidth: 1)
                    .frame(width: bubbleSize + 8, height: bubbleSize + 8)
                Circle()
                    .fill(level ? levelColor : bubbleColor)
                    .frame(width: bubbleSize, height: bubbleSize)
                    .offset(x: x * reach, y: y * reach)
            }
            .frame(width: size, height: size)

            Text(level ? "Level! ✅" : "Tip: \(Int(motion.tiltRight))° right, \(Int(motion.tiltForward))° forward")
                .font(.headline)
        }
    }
}

#Preview {
    LevelView(motion: Motion())
}
