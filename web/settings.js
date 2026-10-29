// ✈️ FLEDGE CODE SETTINGS
// Change a number, save, and reload the page!

// ⏱️ how many seconds a whole program can last; then the drone lands by itself
const longestFlight = 45;

// 📦 how far the drone can get from where it took off, in seconds of flying
//    (ahead, behind, left or right)
const farthestAway = 3;

// ⬆️ how high the drone can climb above where it hovers, in seconds of up()
const highest = 3;

// ➡️ how hard forward, back, left and right push (0 to 100): small is slow
const movePower = 30;

// 🚀 how hard up() pushes (0 to 100)
const upPower = 30;

// 🪂 how hard down() pushes (0 to 100). The drone sinks slower than it
//    climbs, so this is bigger: get it right and down(1) undoes up(1)
const downPower = 100;

// 🌀 how hard rotate() pushes (0 to 100)
const turnPower = 100;

// 🧭 how many degrees the drone turns in one second at turnPower
//    (get it right and rotate(180) is exactly half a turn)
const turnSpeed = 90;

// 🛫 how many seconds taking off takes
const takeOffTime = 2;

// 🛬 how many seconds landing takes
const landTime = 3;
