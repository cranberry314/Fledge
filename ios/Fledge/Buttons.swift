import Observation

// The remote's buttons, as the app sends them to the relay: things that
// stay on or off (speed, lights), and one-off commands (take off, flip).
// The message format is in PROTOCOL.md at the top of the repo.

// One-off commands. The numbers are the ones in PROTOCOL.md.
enum Command: UInt8 {
    case none = 0
    case takeOffOrLand = 1
    case motorsOff = 2
    case calibrate = 3
    case flipForward = 4
    case flipBack = 5
    case flipLeft = 6
    case flipRight = 7
}

@Observable
class Buttons {
    var fast = false   // the remote starts on the slow speed
    var lights = true  // the drone's lights start on

    // The last command, and how many commands have been given. The relay
    // does a command when this count changes.
    private(set) var command = Command.none
    private(set) var count: UInt8 = 0

    func give(_ newCommand: Command) {
        command = newCommand
        count &+= 1  // after 255 comes 0
    }
}
