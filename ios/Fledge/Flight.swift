// Turns the way the phone is tipped into what the drone should do.
//
//   tip forward  -> fly forward      tip back  -> fly backward
//   tip right    -> fly right        tip left  -> fly left
//
// Each is a percentage from -100 to 100: + forward / right, - back / left.
// Tips are measured from the way the phone was held when "Start here" was
// pressed. The settings are in Tuning.swift.
struct Flight {
    var forward = 0.0
    var right = 0.0

    init(from motion: Motion) {
        forward = speed(tip: motion.tiltForward - motion.holdForward)
        right = speed(tip: motion.tiltRight - motion.holdRight)
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
