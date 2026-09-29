import SwiftUI

// A ring for big turns: touch the ring, or drag around it, and the drone
// turns to face that way. The ring's top is the way the drone faced at
// "Zero Level". Small turns come from turning the phone (see Flight.swift).
//
// The drone has no compass, so Fledge can't ask it which way it faces.
// The marker on the ring is Fledge's guess, from how long it has been
// turning. If the real drone turns faster or slower than the marker,
// change droneTurnSpeed in Tuning.swift.
struct TurnRing: View {
    @Binding var target: Double  // where the finger touched: 0 = top, 90 = right
    var facing: Double           // where we think the drone faces

    let size = 160.0
    let thickness = 40.0

    var body: some View {
        let reach = (size - thickness) / 2

        ZStack {
            Circle()
                .stroke(ringColor.opacity(0.35), lineWidth: thickness)
                .padding(thickness / 2)

            // Where the finger touched.
            Circle()
                .stroke(ringColor, lineWidth: 4)
                .frame(width: thickness, height: thickness)
                .offset(y: -reach)
                .rotationEffect(.degrees(target))

            // Where we think the drone faces.
            Text(ringDrone)
                .font(.system(size: 26))
                .offset(y: -reach)
                .rotationEffect(.degrees(facing))
        }
        .frame(width: size, height: size)
        .contentShape(Circle())
        .gesture(
            DragGesture(minimumDistance: 0)
                .onChanged { drag in
                    // The angle from the middle to the finger, clockwise from the top.
                    let x = drag.location.x - size / 2
                    let y = drag.location.y - size / 2
                    var angle = atan2(x, -y) * 180 / .pi
                    // Jump to the nearest step (ringStep in Tuning.swift).
                    if ringStep > 0 {
                        angle = (angle / ringStep).rounded() * ringStep
                    }
                    target = angle
                }
        )
    }
}

#Preview {
    TurnRing(target: .constant(90), facing: 30)
}
