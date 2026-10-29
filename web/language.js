// language.js: read a program and turn it into a list of commands.
//
// This program:
//
//     take_off()
//     repeat 4:
//         forward(1)
//     land()
//
// becomes this list:
//
//     { name: "take_off", line: 1 }
//     { name: "repeat", times: 4, line: 2, inside: [
//         { name: "forward", number: 1, line: 3 } ] }
//     { name: "land", line: 4 }
//
// If a line can't be read, it stops with a Problem: the line number,
// and a sentence that says how to fix it.

// Every command, and what goes in its brackets.
const COMMANDS = {
  take_off: "nothing",
  land: "nothing",
  up: "seconds",
  down: "seconds",
  forward: "seconds",
  back: "seconds",
  left: "seconds",
  right: "seconds",
  wait: "seconds",
  rotate: "degrees",
};

// The other way, for someone who types forward(-1).
const OPPOSITES = {
  up: "down",
  down: "up",
  forward: "back",
  back: "forward",
  left: "right",
  right: "left",
};

// Words people try, and the command they mean.
const SAME_AS = {
  backward: "back",
  backwards: "back",
  turn: "rotate",
  spin: "rotate",
};

class Problem extends Error {
  constructor(line, message) {
    super(message);
    this.line = line;
  }
}

function readProgram(text) {
  const program = [];
  // Where the next command goes: into the program, or inside a repeat.
  // Each place remembers how many spaces its lines start with.
  const places = [{ spaces: 0, commands: program }];
  let newRepeat = null; // a repeat that is waiting for its first line

  const lines = text.split(/\r?\n/);
  for (let i = 0; i < lines.length; i++) {
    const line = i + 1;
    const code = lines[i].split("#")[0]; // after a # is a comment, for people
    if (code.trim() === "") continue;

    const front = code.match(/^\s*/)[0];
    if (front.includes("\t")) {
      throw new Problem(line, "Use spaces at the start of a line, not Tab. (In this editor, the Tab key types 4 spaces.)");
    }
    const spaces = front.length;
    let place = places[places.length - 1];

    if (newRepeat) {
      if (spaces <= place.spaces) {
        throw new Problem(line, `The lines inside a repeat need 4 more spaces in front than the repeat (line ${newRepeat.line}).`);
      }
      place = { spaces: spaces, commands: newRepeat.inside };
      places.push(place);
      newRepeat = null;
    } else if (spaces > place.spaces) {
      throw new Problem(line, "This line has spaces in front, but it isn't inside a repeat. Take the spaces away.");
    } else {
      while (spaces < place.spaces) {
        places.pop();
        place = places[places.length - 1];
      }
      if (spaces !== place.spaces) {
        throw new Problem(line, "The spaces in front of this line don't line up with the lines above it.");
      }
    }

    const command = readLine(code.trim(), line);
    place.commands.push(command);
    if (command.name === "repeat") newRepeat = command;
  }

  if (newRepeat) {
    throw new Problem(newRepeat.line, "This repeat has nothing inside it. Put the lines to repeat under it, with 4 spaces in front.");
  }
  return program;
}

// One line, like "forward(1)" or "repeat 4:".
function readLine(code, line) {
  if (/^repeat\b/.test(code)) return readRepeat(code, line);

  const start = code.match(/^[A-Za-z_]\w*/);
  if (!start) throw new Problem(line, "I can't read this line. A command looks like forward(1).");
  const name = checkName(start[0], line);
  const rest = code.slice(name.length);

  const brackets = rest.match(/^\((.*)\)$/);
  if (!brackets) throw new Problem(line, whatsWrong(name, rest));

  const kind = COMMANDS[name];
  const inside = brackets[1].trim();
  if (kind === "nothing") {
    if (inside !== "") throw new Problem(line, `${name}() doesn't need anything in its brackets: ${name}()`);
    return { name: name, line: line };
  }

  if (inside === "") {
    const what = kind === "degrees" ? "how many degrees" : "how many seconds";
    throw new Problem(line, `${name} needs to know ${what}, like ${example(name)}`);
  }
  if (inside.includes(",") && isNumber(inside.replace(",", "."))) {
    throw new Problem(line, `Use a dot in a decimal number: ${name}(${inside.replace(",", ".")})`);
  }
  if (!isNumber(inside)) throw new Problem(line, `"${inside}" isn't a number. Try ${example(name)}`);

  const number = Number(inside);
  if (number < 0 && kind === "seconds") {
    const other = OPPOSITES[name];
    if (other) throw new Problem(line, `Seconds can't be less than zero. To go the other way, use ${other}(${-number})`);
    throw new Problem(line, "Seconds can't be less than zero.");
  }
  return { name: name, number: number, line: line };
}

// "repeat 4:"
function readRepeat(code, line) {
  if (/^repeat\s*\(/.test(code)) throw new Problem(line, "repeat doesn't use brackets. Write it like this: repeat 4:");
  if (!code.endsWith(":")) throw new Problem(line, `A repeat line ends with a colon: ${code}:`);

  const times = code.slice("repeat".length, -1).trim();
  if (times === "") throw new Problem(line, "repeat needs to know how many times: repeat 4:");
  if (!/^\d+$/.test(times)) {
    if (isNumber(times)) throw new Problem(line, "repeat needs a whole number, like repeat 4:");
    throw new Problem(line, `"${times}" isn't a number. Write it like this: repeat 4:`);
  }
  return { name: "repeat", times: Number(times), inside: [], line: line };
}

// Is it a command we know? If not, guess what was meant.
function checkName(name, line) {
  if (Object.hasOwn(COMMANDS, name)) return name;
  const small = name.toLowerCase();
  if (Object.hasOwn(COMMANDS, small)) throw new Problem(line, `Commands use small letters: ${small}`);
  const guess = Object.hasOwn(SAME_AS, small) ? SAME_AS[small] : closestCommand(small);
  if (guess) throw new Problem(line, `I don't know "${name}". Did you mean ${guess}?`);
  throw new Problem(line, `I don't know "${name}". All the commands are on the card below.`);
}

// The command is right, but the brackets aren't.
function whatsWrong(name, rest) {
  if (rest.endsWith(";")) return "Python doesn't need a ; at the end of a line. Take it away.";
  if (rest.endsWith(":")) return "Only repeat lines end with a colon (:).";
  if (/^\s+\(/.test(rest)) return `No space before the bracket: ${name}${rest.trim()}`;
  if (rest.startsWith("(") && !rest.includes(")")) return "There's a ( but no ) to close it.";
  if (rest.startsWith("(")) return "Something comes after the ). Each line has just one command.";
  if (rest.trim() === "") return `Put brackets after ${name}: ${COMMANDS[name] === "nothing" ? name + "()" : example(name)}`;
  return `Put brackets around the number: ${name}(${rest.trim()})`;
}

function example(name) {
  if (name === "rotate") return "rotate(90)";
  return `${name}(1)`;
}

function isNumber(text) {
  return /^-?(\d+\.?\d*|\.\d+)$/.test(text);
}

// The command spelled most like `name`, if one is close: a letter off
// for a short word, two for a longer one.
function closestCommand(name) {
  let best = null;
  let bestDistance = name.length <= 4 ? 2 : 3;
  for (const command of Object.keys(COMMANDS)) {
    const distance = lettersDifferent(name, command);
    if (distance < bestDistance) {
      best = command;
      bestDistance = distance;
    }
  }
  return best;
}

// How many letters to add, take away or change to turn a into b.
function lettersDifferent(a, b) {
  let previous = Array.from({ length: b.length + 1 }, (_, i) => i);
  for (let i = 1; i <= a.length; i++) {
    const current = [i];
    for (let j = 1; j <= b.length; j++) {
      const change = a[i - 1] === b[j - 1] ? 0 : 1;
      current.push(Math.min(previous[j] + 1, current[j - 1] + 1, previous[j - 1] + change));
    }
    previous = current;
  }
  return previous[b.length];
}
