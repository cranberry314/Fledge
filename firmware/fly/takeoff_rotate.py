# 🛫↩️🛬 TAKE OFF, HALF A TURN, LAND
#
# ▶️ Run me:   python3 takeoff_rotate.py

import drone

drone.pair()           # 🔗 then switch the drone on
drone.take_off()       # 🛫
drone.turn_right(180)  # ↩️ half a turn
drone.land()           # 🛬
