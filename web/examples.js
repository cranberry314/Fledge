// examples.js: the programs in the Examples menu.
// To add one, copy a block below, change the name and the program,
// and reload the page. It appears in the menu in the same order.

const EXAMPLES = [
  {
    name: "🐸 Hop",
    program: `# 🐸 Hop: the smallest program there is.
take_off()
wait(2)
land()

# Try: wait longer before landing.
`,
  },
  {
    name: "↔️ Back and forth",
    program: `# ↔️ Back and forth
# Where does it end up?
take_off()
forward(1)
back(2)
forward(1)
land()

# Try: use left() and right().
`,
  },
  {
    name: "🌀 Spin show",
    program: `# 🌀 Spin show: a whole turn right,
# then a whole turn left.
take_off()
rotate(360)
rotate(-360)
land()

# Try: spin half a turn.
# What number is half of 360?
`,
  },
  {
    name: "🛗 Elevator",
    program: `# 🛗 Elevator: up and down, 3 times.
take_off()
repeat 3:
    up(1)
    down(1)
land()

# Try: change 3 to 5.
# What else could go inside?
`,
  },
  {
    name: "🟥 Square",
    program: `# 🟥 Square
take_off()
repeat 4:
    forward(1)
    rotate(90)
land()

# Try: turn the other way,
# with rotate(-90).
`,
  },
  {
    name: "🔺 Triangle",
    program: `# 🔺 Triangle
take_off()
repeat 3:
    forward(1.5)
    rotate(120)
land()

# Try: a shape with 6 sides.
# How many degrees for each turn?
`,
  },
  {
    name: "🛑 Almost a circle",
    program: `# 🛑 Almost a circle: 8 short sides,
# like a stop sign.
take_off()
repeat 8:
    forward(0.5)
    rotate(45)
land()

# Try: more sides, shorter ones.
# Does it look more like a circle?
`,
  },
  {
    name: "🪜 Staircase",
    program: `# 🪜 Staircase: climb 3 steps,
# then come back.
take_off()
repeat 3:
    forward(1)
    up(1)
back(3)
down(3)
land()

# Try: smaller steps, with
# forward(0.5) and up(0.5).
`,
  },
  {
    name: "⚡ Zigzag",
    program: `# ⚡ Zigzag: right() slides sideways,
# without turning.
take_off()
repeat 2:
    forward(1)
    right(1)
back(2)
left(2)
land()

# Try: zigzag to the left instead.
`,
  },
  {
    name: "🟦 Two squares, sliding",
    program: `# 🟦 Two squares: a repeat in a repeat.
# It slides right and turns, so it
# always faces the middle.
take_off()
repeat 2:
    repeat 4:
        right(1)
        rotate(90)
    wait(1)
land()

# Try: how many times does
# rotate(90) happen? Count, then fly!
`,
  },
  {
    name: "😵 Dizzy drone",
    program: `# 😵 Dizzy drone: what happens
# when the time runs out?
take_off()
rotate(9000000)
land()
`,
  },
  {
    name: "🚧 Too far (fix me!)",
    program: `# 🚧 Too far: this one has a problem.
# Press Fly to see it.
take_off()
repeat 5:
    forward(1)
land()

# Try: change one number so it flies.
`,
  },
];
