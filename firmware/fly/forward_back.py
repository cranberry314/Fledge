# 🛫⏩⏪🛬 TAKE OFF, A LITTLE FORWARD, A LITTLE BACK, LAND
#
# ▶️ Run me:   python3 forward_back.py

import drone

drone.pair()           # 🔗 then switch the drone on
drone.take_off()       # 🛫
drone.forward(1.25)    # ⏩ seconds
drone.back(1.25)       # ⏪ seconds
drone.land()           # 🛬
