# ✈️ FLEDGE FLYING TUNING
# Change a number, save, and fly again!

# 🚀 how hard up pushes (0 to 100)
up_power = 50

# 🪂 how hard down pushes (0 to 100). The drone sinks slower than it
#    climbs, so this is bigger: get it right and down(1) undoes up(1)
down_power = 100

# ➡️ how hard forward, back, left and right push (0 to 100)
move_power = 50

# 🌀 how hard the turns push (0 to 100)
turn_power = 100

# 🧭 how many degrees the drone turns in one second at turn_power 100
#    (get it right and turn_right(180) is exactly half a turn)
turn_speed = 90

# 🐇 True = zoomy      🐢 False = gentle
fast = False

# 💡 True = the drone's lights on      🌑 False = off
lights = True

# 🛫 after taking off, wait this many seconds for the drone to rise
take_off_time = 2

# 🛬 after landing, wait this many seconds for the drone to come down
land_time = 3

# ⏱️ one key press pushes for this many seconds
key_press_time = 0.3

# 🎹 which key does what
keys = {
    "t": "take off",
    "l": "land",
    "w": "up",
    "s": "down",
    "a": "turn left",
    "d": "turn right",
    "up arrow": "forward",
    "down arrow": "back",
    "left arrow": "left",
    "right arrow": "right",
    "f": "flip forward",
    "p": "pair",
}
