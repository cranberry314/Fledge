import CoreMotion
import Observation

// Reads the phone's motion sensors: how it is tilted (for the bubble
// level) and which way it points (for the compass). Screens only read the
// numbers below; nothing else needs to open this file.
//
// The app is used sideways (landscape) with the charging port on the
// right. The sensors measure along the phone's own edges as if it stood
// upright, so: the screen's right side is the phone's bottom edge, and the
// screen's far (top) edge is the phone's right edge.
@Observable
class Motion {
    // How far the phone is tipped, in degrees. 0 when it lies flat.
    var tiltRight = 0.0    // + when the screen's right side is lower
    var tiltForward = 0.0  // + when the screen's far edge is lower

    // Which way the far edge of the screen points: 0 = north, 90 = east.
    var heading = 0.0

    // How far the phone has turned since "Start here", in degrees:
    // + to the right (clockwise), - to the left.
    var turn = 0.0

    private let manager = CMMotionManager()
    private var startHeading: Double?

    func start() {
        guard manager.isDeviceMotionAvailable else { return }
        manager.deviceMotionUpdateInterval = 1.0 / 30.0
        // Measure turning from magnetic north, so the compass knows north.
        manager.startDeviceMotionUpdates(using: .xMagneticNorthZVertical, to: .main) { data, _ in
            guard let data else { return }
            MainActor.assumeIsolated { self.update(data) }
        }
    }

    func stop() {
        manager.stopDeviceMotionUpdates()
    }

    // Count turning from the way the phone points now.
    func startHere() {
        startHeading = heading
    }

    private func update(_ data: CMDeviceMotion) {
        let gravity = data.gravity
        tiltRight = degrees(asin(min(max(-gravity.y, -1), 1)))
        tiltForward = degrees(asin(min(max(gravity.x, -1), 1)))

        // The sensors say how far the phone has turned from "the phone's
        // right edge points north", counting anticlockwise. Held sideways,
        // that edge is the screen's far edge, the way we point.
        heading = wrapTo360(-degrees(data.attitude.yaw))

        if startHeading == nil {
            startHeading = heading
        }
        turn = wrapTo180(heading - (startHeading ?? heading))
    }
}

private func degrees(_ radians: Double) -> Double {
    radians * 180 / .pi
}

// An angle from 0 up to 360.
private func wrapTo360(_ angle: Double) -> Double {
    let wrapped = angle.truncatingRemainder(dividingBy: 360)
    return wrapped < 0 ? wrapped + 360 : wrapped
}

// An angle from -180 up to 180.
private func wrapTo180(_ angle: Double) -> Double {
    let wrapped = wrapTo360(angle)
    return wrapped > 180 ? wrapped - 360 : wrapped
}
