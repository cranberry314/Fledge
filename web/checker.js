// checker.js: fly a program on paper, before the drone flies it for real.
//
// It goes through the commands one by one, keeping track of where the
// drone would be, which way it would face and how long the program has
// lasted. A move that would take the drone too far stops the program
// with a Problem before the drone even takes off. When the program
// reaches longestFlight seconds, the drone lands by itself.
//
// Where the drone is, in seconds of flying from where it took off:
//   x        to the right (less than 0: to the left)
//   y        ahead (less than 0: behind); ahead is the way it faced at the start
//   height   above where it hovers after take_off()
//   heading  which way it faces, in degrees: 0 is ahead, 90 is to the right

const MOST_WORK = 100000; // stops a giant repeat of moves that take no time
const TINY = 0.001; // so 2.9999999 seconds counts as 3

function startingState() {
  return { x: 0, y: 0, height: 0, heading: 0, flying: false, time: 0 };
}

// Fly the commands, starting from `state` and changing it as the drone
// moves. Gives back the steps for the preview to play, and notes to show.
function flyOnPaper(commands, state, landAtTheEnd) {
  const flight = { state: state, steps: [], notes: [], over: false, work: 0 };
  flyCommands(commands, flight, "");
  if (landAtTheEnd && !flight.over && state.flying) {
    flight.notes.push("The program ended with the drone still flying, so it landed by itself. Tip: end with land()");
    land(flight, { name: "land", byItself: true });
  }
  return flight;
}

// Land right now, wherever the drone is: for Stop, and for the command line.
function landNow(state, note) {
  const flight = { state: state, steps: [], notes: [note], over: true, work: 0 };
  if (state.flying) land(flight, { name: "land", byItself: true });
  return flight;
}

function flyCommands(commands, flight, round) {
  for (const command of commands) {
    if (flight.over) return;
    if (command.name === "repeat") {
      for (let count = 1; count <= command.times && !flight.over; count++) {
        countWork(flight, command);
        flyCommands(command.inside, flight, `The ${ordinal(count)} time around: `);
      }
    } else {
      flyCommand(command, flight, round);
    }
  }
}

function flyCommand(command, flight, round) {
  const state = flight.state;

  if (state.time >= longestFlight - TINY) {
    flight.notes.push(`The program reached ${longestFlight} seconds before line ${command.line}, so it stopped there.`);
    if (state.flying) land(flight, { name: "land", line: command.line, byItself: true });
    flight.over = true;
    return;
  }

  if (command.name === "take_off") {
    if (state.flying) throw new Problem(command.line, `${round}The drone is already flying.`);
    takeStep(flight, command, takeOffTime, () => { state.flying = true; });
    return;
  }
  if (!state.flying) {
    throw new Problem(command.line, `${round}The drone is on the ground. Put take_off() before ${show(command)}`);
  }
  if (command.name === "land") {
    land(flight, command);
    return;
  }

  // Everything else is a move. How long does it take?
  let seconds = command.number;
  if (command.name === "rotate") seconds = Math.abs(command.number) / turnSpeed;

  // Only do as much of it as there's time for.
  let part = 1;
  const timeLeft = longestFlight - state.time;
  if (seconds > timeLeft) {
    part = timeLeft / seconds;
    seconds = timeLeft;
  }

  takeStep(flight, command, seconds, () => move(state, command.name, command.number * part));
  checkStillInside(state, command, round);

  if (part < 1) {
    flight.notes.push(`The program reached ${longestFlight} seconds during ${show(command)} on line ${command.line}, so the drone landed by itself.`);
    land(flight, { name: "land", line: command.line, byItself: true });
    flight.over = true;
  }
}

function land(flight, command) {
  const state = flight.state;
  takeStep(flight, command, landTime, () => {
    state.flying = false;
    state.height = 0;
  });
}

// Change the state, and remember the step: where it started and ended.
function takeStep(flight, command, seconds, change) {
  countWork(flight, command);
  const from = { ...flight.state };
  change();
  flight.state.time += seconds;
  flight.steps.push({ command: command, seconds: seconds, from: from, to: { ...flight.state } });
}

function countWork(flight, command) {
  flight.work++;
  if (flight.work > MOST_WORK) {
    throw new Problem(command.line, "This program has too many moves for me to check! Try a smaller number in repeat.");
  }
}

function move(state, name, amount) {
  const facing = state.heading * Math.PI / 180;
  // Where one second of forward, and of right, takes the drone.
  const ahead = { x: Math.sin(facing), y: Math.cos(facing) };
  const toTheRight = { x: Math.cos(facing), y: -Math.sin(facing) };

  if (name === "forward") {
    state.x += amount * ahead.x;
    state.y += amount * ahead.y;
  } else if (name === "back") {
    state.x -= amount * ahead.x;
    state.y -= amount * ahead.y;
  } else if (name === "right") {
    state.x += amount * toTheRight.x;
    state.y += amount * toTheRight.y;
  } else if (name === "left") {
    state.x -= amount * toTheRight.x;
    state.y -= amount * toTheRight.y;
  } else if (name === "up") {
    state.height += amount;
  } else if (name === "down") {
    state.height = Math.max(0, state.height - amount); // it can't sink below its hover
  } else if (name === "rotate") {
    state.heading += amount;
  }
  // wait() doesn't move at all.
}

function checkStillInside(state, command, round) {
  const edge = farthestAway + TINY;
  let tooFar = null;
  if (state.y > edge) tooFar = `${secondsText(state.y)} ahead of`;
  else if (state.y < -edge) tooFar = `${secondsText(-state.y)} behind`;
  else if (state.x > edge) tooFar = `${secondsText(state.x)} to the right of`;
  else if (state.x < -edge) tooFar = `${secondsText(-state.x)} to the left of`;

  if (tooFar) {
    throw new Problem(command.line, `${round}${show(command)} would take the drone ${tooFar} where it took off. The farthest it can go is ${farthestAway} seconds.`);
  }
  if (state.height > highest + TINY) {
    throw new Problem(command.line, `${round}${show(command)} would take the drone ${secondsText(state.height)} higher than where it hovers. The highest it can go is ${highest} seconds.`);
  }
}

// forward(1), or take_off()
function show(command) {
  if (command.number === undefined) return `${command.name}()`;
  return `${command.name}(${command.number})`;
}

function secondsText(seconds) {
  const rounded = Math.round(seconds * 10) / 10;
  return rounded === 1 ? "1 second" : `${rounded} seconds`;
}

// 1st, 2nd, 3rd, 4th ... 11th, 12th, 13th ... 21st
function ordinal(n) {
  const lastTwo = n % 100;
  const last = n % 10;
  if (lastTwo >= 11 && lastTwo <= 13) return n + "th";
  if (last === 1) return n + "st";
  if (last === 2) return n + "nd";
  if (last === 3) return n + "rd";
  return n + "th";
}
