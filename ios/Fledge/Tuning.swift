import SwiftUI

// ✈️ FLEDGE TUNING
// Change a number, press Build, and fly again!

// 🐢 small = gentle      🐇 big = twitchy
let tiltSensitivity = 1.0

// 😴 big = phone must tip a lot      ⚡️ small = moves at a tiny tip
let tiltDeadZone = 5.0

// 🌀 how many degrees the drone spins in one second (make the ring match the drone)
let droneTurnSpeed = 90.0

// 📏 lift or lower the phone more than this (in cm) and the drone follows
let phoneLiftDeadZone = 20.0

// 🪶 how fast lifting the phone can make the drone climb (0 to 100, 0 = off)
let phoneLiftMax = 30.0

// 🎚️ the colour of the up/down slider
let upSliderColor = Color.teal

// 🪜 the ring jumps in steps this big, in degrees (0 = no jumps)
let ringStep = 90.0

// 🍩 the colour of the turning ring
let ringColor = Color.blue

// 🛸 the picture on the ring that shows which way the drone faces
let ringDrone = "🛸"

// 🚀 how fast the propellers can go (0 to 100)
let maxThrottle = 80

// 🎨 the colour of the flip button
let flipButtonColor = Color.purple

// 🤸 the picture on the flip button
let flipButtonEmoji = "🤸"

// 🫧 the colour of the bubble in the level
let bubbleColor = Color.orange

// ✅ the bubble's colour when the phone is level
let levelColor = Color.green

// 🧭 the colour of the N on the compass
let northColor = Color.red

// 🐣 the bird in the compass: it points the way you're facing
let compassBird = "🐣"

// 🔄 turn the bird until its beak points up (in degrees, clockwise)
let compassBirdTurn = 90.0
