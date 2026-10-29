// preview.js: show the flight on the screen. On the left, the room seen
// from above; on the right, how high the drone is. playSteps() plays a
// flight in real time: a 10-second program takes 10 seconds to watch.

const canvas = document.getElementById("preview");
const pen = canvas.getContext("2d");
let trail = []; // the places the drone has been, for the line behind it
let playing = 0; // goes up by one whenever playing starts or stops
let lastDrawn = restingAt(startingState()); // where the drone is on the screen

// A state from checker.js, as something to draw.
function restingAt(state) {
  return { ...state, lift: state.flying ? 1 : 0 };
}

// Where the drone is, part of the way through a step (0 = start, 1 = end).
function partWay(step, part) {
  const a = step.from;
  const b = step.to;
  let lift = b.flying ? 1 : 0;
  if (step.command.name === "take_off") lift = part;
  if (step.command.name === "land") lift = 1 - part;
  return {
    x: a.x + (b.x - a.x) * part,
    y: a.y + (b.y - a.y) * part,
    height: a.height + (b.height - a.height) * part,
    heading: a.heading + (b.heading - a.heading) * part,
    time: a.time + (b.time - a.time) * part,
    flying: lift > 0,
    lift: lift,
  };
}

function startTrail(state) {
  trail = [{ x: state.x, y: state.y }];
}

function addToTrail(state) {
  const last = trail[trail.length - 1];
  if (last && last.x === state.x && last.y === state.y) return;
  trail.push({ x: state.x, y: state.y });
}

// Play the steps. onStep(step) runs as each one starts; onDone() at the end.
function playSteps(steps, onStep, onDone) {
  const me = ++playing;
  let index = 0;
  let stepStart = performance.now();
  if (steps.length === 0) {
    onDone();
    return;
  }
  onStep(steps[0]);

  function frame(now) {
    if (me !== playing) return; // stopped, or something else started playing

    // Move on past every step that has finished.
    while (now - stepStart >= steps[index].seconds * 1000) {
      stepStart += steps[index].seconds * 1000;
      addToTrail(steps[index].to);
      index++;
      if (index === steps.length) {
        drawPreview(restingAt(steps[index - 1].to));
        playing++;
        onDone();
        return;
      }
      onStep(steps[index]);
    }

    const step = steps[index];
    drawPreview(partWay(step, (now - stepStart) / (step.seconds * 1000)));
    requestAnimationFrame(frame);
  }
  requestAnimationFrame(frame);
}

function stopPlaying() {
  playing++;
}

function color(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}

function drawPreview(where) {
  lastDrawn = where;

  // Match the canvas to its size on the screen, so it stays sharp.
  const sharp = window.devicePixelRatio || 1;
  const width = canvas.clientWidth;
  const height = canvas.clientHeight;
  if (canvas.width !== Math.round(width * sharp) || canvas.height !== Math.round(height * sharp)) {
    canvas.width = Math.round(width * sharp);
    canvas.height = Math.round(height * sharp);
  }
  pen.setTransform(sharp, 0, 0, sharp, 0, 0);
  pen.clearRect(0, 0, width, height);

  const barWidth = 70;
  drawRoom(where, width - barWidth, height);
  drawHeightBar(where, width - barWidth, barWidth, height);
  drawClock(where);
}

// The room from above: the box the drone must stay in, the start, the trail.
function drawRoom(where, width, height) {
  const reach = farthestAway + 1; // show a little past the edge
  const scale = Math.min(width, height) / 2 / reach; // pixels for 1 second of flying
  const middleX = width / 2;
  const middleY = height / 2;
  const screenX = (x) => middleX + x * scale;
  const screenY = (y) => middleY - y * scale;

  // A line every second.
  pen.strokeStyle = color("--grid");
  pen.lineWidth = 1;
  for (let s = -reach; s <= reach; s++) {
    pen.beginPath();
    pen.moveTo(screenX(s), screenY(-reach));
    pen.lineTo(screenX(s), screenY(reach));
    pen.moveTo(screenX(-reach), screenY(s));
    pen.lineTo(screenX(reach), screenY(s));
    pen.stroke();
  }

  // The edge of where the drone may go.
  pen.strokeStyle = color("--edge");
  pen.lineWidth = 2;
  pen.setLineDash([8, 6]);
  pen.strokeRect(screenX(-farthestAway), screenY(farthestAway), farthestAway * 2 * scale, farthestAway * 2 * scale);
  pen.setLineDash([]);

  pen.fillStyle = color("--muted");
  pen.font = "14px system-ui, sans-serif";
  pen.textAlign = "center";
  pen.fillText("⬆ ahead", middleX, screenY(farthestAway) - 8);

  // The start.
  pen.strokeStyle = color("--muted");
  pen.beginPath();
  pen.arc(screenX(0), screenY(0), 14, 0, Math.PI * 2);
  pen.stroke();

  // The trail.
  pen.strokeStyle = color("--trail");
  pen.lineWidth = 3;
  pen.lineJoin = "round";
  pen.beginPath();
  trail.forEach((point, i) => {
    if (i === 0) pen.moveTo(screenX(point.x), screenY(point.y));
    else pen.lineTo(screenX(point.x), screenY(point.y));
  });
  pen.lineTo(screenX(where.x), screenY(where.y));
  pen.stroke();

  drawDrone(screenX(where.x), screenY(where.y), where);
}

// Four rotors and a nose, turned the way the drone faces. It looks
// bigger, with a shadow, when it's off the ground.
function drawDrone(x, y, where) {
  const size = 1 + 0.25 * where.lift;
  pen.save();
  pen.translate(x, y);

  pen.fillStyle = color("--shadow");
  pen.beginPath();
  pen.ellipse(6 * where.lift, 8 * where.lift, 20, 20, 0, 0, Math.PI * 2);
  pen.fill();

  pen.rotate(where.heading * Math.PI / 180);
  pen.scale(size, size);
  pen.fillStyle = where.flying ? color("--drone") : color("--muted");
  for (const [rx, ry] of [[-10, -10], [10, -10], [-10, 10], [10, 10]]) {
    pen.beginPath();
    pen.arc(rx, ry, 7, 0, Math.PI * 2);
    pen.fill();
  }
  pen.fillRect(-6, -6, 12, 12);
  pen.beginPath(); // the nose, pointing the way it faces
  pen.moveTo(0, -24);
  pen.lineTo(-6, -12);
  pen.lineTo(6, -12);
  pen.closePath();
  pen.fill();
  pen.restore();
}

// How high it is: the ground, where it hovers, and the highest it may go.
function drawHeightBar(where, left, width, height) {
  const ground = height - 24;
  const top = 30;
  const scale = (ground - top) / (1 + highest + 0.5); // pixels for 1 second of up()
  const middle = left + width / 2;
  const screenY = (up) => ground - up * scale;

  pen.strokeStyle = color("--grid");
  pen.lineWidth = 1;
  pen.beginPath();
  pen.moveTo(middle, top);
  pen.lineTo(middle, ground);
  pen.stroke();

  pen.strokeStyle = color("--muted");
  pen.lineWidth = 2;
  pen.beginPath();
  pen.moveTo(left + 10, ground);
  pen.lineTo(left + width - 10, ground);
  pen.stroke();

  pen.strokeStyle = color("--edge");
  pen.setLineDash([8, 6]);
  pen.beginPath();
  pen.moveTo(left + 10, screenY(1 + highest));
  pen.lineTo(left + width - 10, screenY(1 + highest));
  pen.stroke();
  pen.setLineDash([]);

  pen.fillStyle = color("--muted");
  pen.font = "14px system-ui, sans-serif";
  pen.textAlign = "center";
  pen.fillText("height", middle, top - 12);

  // Off the ground it's at its hover (1), plus however much up() it did.
  const up = where.lift * (1 + where.height);
  pen.fillStyle = where.flying ? color("--drone") : color("--muted");
  pen.beginPath();
  pen.arc(middle, screenY(up) - 8, 8, 0, Math.PI * 2);
  pen.fill();
}

function drawClock(where) {
  pen.fillStyle = color("--ink");
  pen.font = "bold 16px system-ui, sans-serif";
  pen.textAlign = "left";
  pen.fillText(`⏱️ ${where.time.toFixed(1)} of ${longestFlight} s`, 10, 22);
}

window.addEventListener("resize", () => drawPreview(lastDrawn));
