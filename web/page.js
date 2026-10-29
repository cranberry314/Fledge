// page.js: the boxes and buttons on the page.
//
// Two ways to fly:
//   ▶️ Fly     checks the whole program on the left, then flies it.
//   fly>       flies one command at a time, as you type them.

const editor = document.getElementById("program");
const lineNumbers = document.getElementById("line-numbers");
const flyButton = document.getElementById("fly");
const stopButton = document.getElementById("stop");
const commandBox = document.getElementById("command");
const doing = document.getElementById("doing");
const messages = document.getElementById("messages");
const connectButton = document.getElementById("connect");
const pairButton = document.getElementById("pair");
const relayStatus = document.getElementById("relay-status");
const examplesMenu = document.getElementById("examples");

const STARTING_PROGRAM = `# ✈️ My first program
# The drone does these lines, from top to bottom.

take_off()
up(1)
repeat 4:
    forward(1)
    rotate(90)
land()
`;

let busy = false; // true while the drone is doing something
let flyingFrom = "program"; // or "command line": which one the drone is doing
let live = startingState(); // the drone, as flown from the command line
let liveTakeOffAt = 0; // when the command line's take_off() started
const typed = []; // the commands typed so far, for the ↑ key
let typedPlace = 0;

// ---- Messages ----

function say(text, kind) {
  const message = document.createElement("p");
  message.textContent = text;
  message.className = kind || "";
  messages.append(message);
}

function clearMessages() {
  messages.textContent = "";
  doing.textContent = "";
  showLineNumbers(0);
}

function showProblem(problem, where) {
  if (where === "program") {
    say(`❌ Line ${problem.line}: ${problem.message}`, "problem");
    showLineNumbers(problem.line);
    selectLine(problem.line);
  } else {
    say(`❌ ${problem.message}`, "problem");
  }
}

function showStep(step) {
  if (step.command.byItself) {
    doing.textContent = "🛬 Landing by itself";
  } else if (flyingFrom === "program") {
    doing.textContent = `Line ${step.command.line}: ${show(step.command)}`;
  } else {
    doing.textContent = show(step.command);
  }
}

function updateButtons() {
  flyButton.disabled = busy;
  stopButton.disabled = !busy && !live.flying;
}

// ---- Flying ----

function play(flight, doneMessage) {
  busy = true;
  updateButtons();
  const doStep = (step) => {
    showStep(step);
    flyStep(step); // the real drone, if one is connected
  };
  playSteps(flight.steps, doStep, () => {
    busy = false;
    doing.textContent = "";
    for (const note of flight.notes) say("💡 " + note, "note");
    if (doneMessage) say(doneMessage);
    updateButtons();
  });
}

function flyProgram() {
  if (busy) return;
  clearMessages();
  if (live.flying) {
    say("The drone is flying from the command line. Type land() there first.", "problem");
    return;
  }

  let flight;
  try {
    flight = flyOnPaper(readProgram(editor.value), startingState(), true);
  } catch (problem) {
    if (!(problem instanceof Problem)) throw problem;
    showProblem(problem, "program");
    return;
  }

  flyingFrom = "program";
  startTrail(startingState());
  live = flight.state; // where the program leaves the drone, landed
  play(flight, "✅ Done!");
}

function flyTypedCommand() {
  const text = commandBox.value.trim();
  if (text === "") return;
  if (busy) {
    say("Wait for the drone to finish first.", "note");
    return;
  }
  typed.push(text);
  typedPlace = typed.length;
  commandBox.value = "";
  clearMessages();
  say("fly> " + text, "typed");

  if (/^repeat\b/.test(text)) {
    say("A repeat needs more than one line, so write it in the program on the left.", "problem");
    return;
  }

  // Try it on a copy, so a problem leaves the drone where it is.
  const trial = { ...live };
  trial.time = live.flying ? (performance.now() - liveTakeOffAt) / 1000 : 0;
  let flight;
  try {
    flight = flyOnPaper(readProgram(text), trial, false);
  } catch (problem) {
    if (!(problem instanceof Problem)) throw problem;
    showProblem(problem, "command line");
    return;
  }

  if (!live.flying && trial.flying) {
    liveTakeOffAt = performance.now();
    startTrail(live);
  }
  live = trial;
  flyingFrom = "command line";
  play(flight);
}

// ✋ Stop: land right where the drone is now.
function stop() {
  stopPlaying();
  const here = { ...lastDrawn };
  delete here.lift;
  live = here;
  clearMessages();
  play(landNow(live, "Stopped."));
}

// Chrome slows down a page that's hidden (another tab, the lid closed),
// and then the relay stops hearing from it. Land before that happens.
document.addEventListener("visibilitychange", () => {
  if (document.hidden && (busy || live.flying)) stop();
});

// A command-line flight lands by itself when its time is up.
setInterval(() => {
  if (busy || !live.flying) return;
  if ((performance.now() - liveTakeOffAt) / 1000 >= longestFlight) {
    live.time = longestFlight;
    play(landNow(live, `That's ${longestFlight} seconds, so the drone landed by itself.`));
  }
}, 250);

// ---- The program editor ----

function showLineNumbers(problemLine) {
  const count = editor.value.split("\n").length;
  lineNumbers.textContent = "";
  for (let line = 1; line <= count; line++) {
    const number = document.createElement("div");
    number.textContent = line;
    if (line === problemLine) number.className = "problem";
    lineNumbers.append(number);
  }
  lineNumbers.scrollTop = editor.scrollTop;
}

function selectLine(line) {
  const lines = editor.value.split("\n");
  const start = lines.slice(0, line - 1).join("\n").length + (line > 1 ? 1 : 0);
  editor.focus();
  editor.setSelectionRange(start, start + lines[line - 1].length);
}

// Type into the editor the way a person would, so Undo still works.
function type(text) {
  document.execCommand("insertText", false, text);
}

function lineBeforeCursor() {
  const before = editor.value.slice(0, editor.selectionStart);
  return before.slice(before.lastIndexOf("\n") + 1);
}

editor.addEventListener("keydown", (event) => {
  if (event.key === "Tab" && !event.shiftKey) {
    // Tab types 4 spaces.
    event.preventDefault();
    type("    ");
  } else if (event.key === "Enter" && (event.ctrlKey || event.metaKey)) {
    // Ctrl+Enter flies.
    event.preventDefault();
    flyProgram();
  } else if (event.key === "Enter") {
    // A new line starts with the same spaces, and 4 more after a colon.
    event.preventDefault();
    const line = lineBeforeCursor();
    let spaces = line.match(/^ */)[0];
    if (line.split("#")[0].trim().endsWith(":")) spaces += "    ";
    type("\n" + spaces);
  } else if (event.key === "Backspace" && editor.selectionStart === editor.selectionEnd) {
    // In the spaces at the start of a line, Backspace takes away 4 at once.
    const line = lineBeforeCursor();
    if (/^ +$/.test(line)) {
      event.preventDefault();
      const remove = ((line.length - 1) % 4) + 1;
      editor.setSelectionRange(editor.selectionStart - remove, editor.selectionStart);
      document.execCommand("delete");
    }
  }
});

editor.addEventListener("input", () => {
  showLineNumbers(0);
  try {
    localStorage.setItem("fledge-program", editor.value);
  } catch (error) {
    // No saving in this browser: the program just won't be there next time.
  }
});

editor.addEventListener("scroll", () => {
  lineNumbers.scrollTop = editor.scrollTop;
});

// ---- The examples menu ----

for (const example of EXAMPLES) {
  const choice = document.createElement("option");
  choice.textContent = example.name;
  examplesMenu.append(choice);
}

examplesMenu.addEventListener("change", () => {
  const example = EXAMPLES.find((e) => e.name === examplesMenu.value);
  examplesMenu.value = ""; // back to "Examples…", so the same one can be picked again
  if (!example) return;

  // Ask first, unless the program is empty or still an example nobody changed.
  const program = editor.value.trim();
  const unchanged = program === "" || program === STARTING_PROGRAM.trim() ||
    EXAMPLES.some((e) => e.program.trim() === program);
  if (!unchanged && !confirm("Swap your program for this example? (Ctrl+Z brings yours back.)")) return;

  // Typed in like a person would, so Ctrl+Z still works.
  editor.focus();
  editor.select();
  type(example.program);
  editor.setSelectionRange(0, 0);
  editor.scrollTop = 0;
  clearMessages();
});

// ---- The command line ----

commandBox.addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    flyTypedCommand();
  } else if (event.key === "ArrowUp" && typedPlace > 0) {
    // ↑ and ↓ bring back commands typed before.
    event.preventDefault();
    typedPlace--;
    commandBox.value = typed[typedPlace];
  } else if (event.key === "ArrowDown" && typedPlace < typed.length) {
    event.preventDefault();
    typedPlace++;
    commandBox.value = typed[typedPlace] || "";
  }
});

// ---- The relay ----

async function connect() {
  if (!canUseBluetooth()) {
    relayStatus.textContent = "This page can't use Bluetooth. Open it from http://localhost:8000 in Chrome (see README.md).";
    return;
  }
  try {
    await connectRelay(() => {
      relayStatus.textContent = "📴 Lost the relay. If the drone was flying, it comes down by itself.";
      pairButton.disabled = true;
      connectButton.disabled = false;
    });
  } catch (error) {
    relayStatus.textContent = "Not connected: " + error.message;
    return;
  }
  relayStatus.textContent = "✅ Connected to the relay. The drone does what the screen shows!";
  pairButton.disabled = false;
  connectButton.disabled = true;
}

function pair() {
  pairDrone();
  relayStatus.textContent = "🔗 Pairing: switch the drone on now! (The relay listens for 30 seconds.)";
}

connectButton.addEventListener("click", connect);
pairButton.addEventListener("click", pair);
flyButton.addEventListener("click", flyProgram);
stopButton.addEventListener("click", stop);

// ---- Starting up ----

let saved = null;
try {
  saved = localStorage.getItem("fledge-program");
} catch (error) {
  // No saving in this browser.
}
editor.value = saved || STARTING_PROGRAM;
showLineNumbers(0);

// The numbers on the card come from settings.js.
const SETTINGS = { longestFlight, farthestAway, highest };
for (const spot of document.querySelectorAll("[data-setting]")) {
  spot.textContent = SETTINGS[spot.dataset.setting];
}

startTrail(live);
drawPreview(restingAt(live));
updateButtons();
