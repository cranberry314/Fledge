// Turns the way the phone is tipped, and the turning ring, into what the
// drone should do.
//
//   tip forward  -> fly forward      tip back  -> fly backward
//   tip right    -> fly right        tip left  -> fly left
//   touch the ring -> turn to face that way (big turns)
//   turn the phone -> turn the same amount  (small turns)
//   push the slider up / down -> climb / sink
//   lift / lower the phone    -> climb / sink, gently
//
// Each is a percentage from -100 to 100: + forward / right / spin right /
// up, - back / left / spin left / down. Tips and lifts are measured from
// the way the phone was held when "Zero Level" was pressed. The settings
// are in Tuning.swift.
struct Flight {
    var forward = 0.0
    var right = 0.0
    var turn = 0.0
    var up = 0.0

    // facing: where we think the drone faces. target: where the finger
    // touched the ring. Both in degrees: 0 = the start, 90 = right.
    // slider: the up/down slider, -100 to 100.
    init(from motion: Motion, facing: Double, target: Double, slider: Double) {
        forward = speed(tip: motion.tiltForward - motion.holdForward)
        right = speed(tip: motion.tiltRight - motion.holdRight)
        // The drone should face the ring's spot, plus however far the
        // phone has turned since "Zero Level".
        let goal = target + motion.turn
        turn = turnSpeed(toGo: wrapTo180(goal - facing))
        let lift = liftSpeed(raised: motion.height - motion.holdHeight)
        up = min(max(slider + lift, -100), 100)
    }
}

// Tipping less than tiltDeadZone does nothing. After that, 45 more degrees
// is full speed (at tiltSensitivity 1).
private func speed(tip: Double) -> Double {
    let past = abs(tip) - tiltDeadZone
    if past <= 0 {
        return 0
    }
    let percent = min(past / 45 * 100 * tiltSensitivity, 100)
    return tip > 0 ? percent : -percent
}

// Turn at full speed toward the finger, slowing down over the last 30
// degrees so the drone doesn't overshoot. Within 2 degrees: stop.
private func turnSpeed(toGo: Double) -> Double {
    if abs(toGo) < 2 {
        return 0
    }
    let percent = min(abs(toGo) / 30 * 100, 100)
    return toGo > 0 ? percent : -percent
}

// Moving the phone up or down less than phoneLiftDeadZone does nothing.
// After that, each centimetre more is 1% more, up to phoneLiftMax.
private func liftSpeed(raised: Double) -> Double {
    let past = abs(raised) - phoneLiftDeadZone
    if past <= 0 {
        return 0
    }
    let percent = min(past, phoneLiftMax)
    return raised > 0 ? percent : -percent
}
