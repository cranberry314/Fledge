# ✈️ MY FLIGHT PLAN
# The drone does these moves, from top to bottom.
# Move them around, change the numbers, add more!
#
# ▶️ Run me:   python3 flight_plan.py
#
# All the moves:
#   drone.take_off()       🛫          drone.land()           🛬
#   drone.up(1)            ⬆️ seconds   drone.down(1)          ⬇️ seconds
#   drone.forward(1)       ⏩ seconds   drone.back(1)          ⏪ seconds
#   drone.left(1)          ⬅️ seconds   drone.right(1)         ➡️ seconds
#   drone.turn_left(90)    ↪️ degrees   drone.turn_right(90)   ↩️ degrees
#   drone.flip_forward()   🤸          drone.wait(1)          ⏸️ seconds
#   drone.calibrate()      📐 on flat ground, before take_off

import drone

drone.pair()           # 🔗 then switch the drone on
drone.calibrate()      # 📐 flat ground first
drone.take_off()       # 🛫
drone.turn_right(180)  # ↩️ half a turn
drone.land()           # 🛬
