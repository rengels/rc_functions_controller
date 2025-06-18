/** Javascript module for handling HW overview and config statistics
 *
 *  For the rc functions controller.
 */

import {defProcs} from './def_procs.js';
import {getConfigFromTable} from "./procs.js";

/** Proc Names by section 
 *
 *  looks like this: {"Input": ["INPUT_DEMO", "INPU...] }
 */
let procsBySection = [];
let sectionName = "";
for (let i = 0; i < defProcs.length; i++) {
  const proc = defProcs[i];
  if ("name" in proc) {
    procsBySection[sectionName].push(proc["name"]);
  } else if ("description" in proc) {
    sectionName = proc["description"];
    procsBySection[sectionName] = [];
  }
}


/** Produce a warning if the pin with the pinNumber is not suitable for output.
 *
 *  @param {Array} warnings A list of strings containing warnings about the configuration.
 *  @param {Array} notes A list of strings containing notes about the configuration.
 */
function checkPinOutput(configName, pinNumber, warnings, notes) {
  const INPUT_ONLY = [34, 35, 36, 37, 38, 39];
  if (INPUT_ONLY.includes(pinNumber)) {
    warnings.push(`Pin ${pinNumber} can only be used for input for ${configName}.`);
  }

  const UNAVAILABLE = [1, 3, 20, 24, 28, 29, 30, 31];
  if (UNAVAILABLE.includes(pinNumber)) {
    warnings.push(`Pin ${pinNumber} is unavailable for ${configName}.`);
  }

  const SIDE_EFFECT = [0, 12, 14, 15];
  if (SIDE_EFFECT.includes(pinNumber)) {
    notes.push(`Pin ${pinNumber} has side effects for ${configName}.`);
  }
}


/** Produce a warning if a proc is there more than once.
 *
 *  @param {Array} config A list of proc configurations of the same type.
 *  @param {Array} warnings A list of strings containing warnings about the configuration.
 *  @param {Array} notes A list of strings containing notes about the configuration.
 *  @param {Array} pinFunctions A list of lists containing strings describing pin functions.
 */
function checkOnlyOne(configs, warnings, notes, pinFunctions) {
  if (configs.length > 1) {
    warnings.push("You should not have the proc " +
      niceName(configs[0]["name"]) + " more than once.");
  }
}

/** Checks InputAdc
 *  @param {Array} config A list of proc configurations of the same type.
 *  @param {Array} warnings A list of strings containing warnings about the configuration.
 *  @param {Array} notes A list of strings containing notes about the configuration.
 *  @param {Array} pinFunctions A list of lists containing strings describing pin functions.
 */
function checkADC(configs, warnings, notes, pinFunctions) {
  for (let i = 0; i < configs.length; i++) {
    const conf = configs[i];
    const type = conf["types"]["type"]?.[0];
    const pin = Number(conf["values"]["pin"]?.[0]);
    if (type != "ST_NONE") {
      if ((pin < 32) || (pin > 39)) {
        warnings.push("Pin " + pin + " not available as input for " +
          niceName(configs[0]["name"]));
      } else {
        if (pin < pinFunctions.length) {
          pinFunctions[pin].push(niceName(conf["name"]) + " / " + niceName(type));
        }
      }
    }
  }
}

/** Checks InputPin and InputPwm (currently they use the same ports. */
function checkPin(configs, warnings, notes, pinFunctions) {
  const pinMapping = [12, 13, 14, 27, 34, 35];

  for (let i = 0; i < configs.length; i++) {
    const conf = configs[i];
    const types = conf["types"]["types"];
    for (let j = 0; j < types.length && j < pinMapping.length; j++) {
      if (types[j] != "ST_NONE") {
        pinFunctions[pinMapping[j]].push(niceName(conf["name"]) + " / " + niceName(types[j]));
      }
    }
  }
}


/** Checks InputPPM and InputSBUS */
function checkPpm(configs, warnings, notes, pinFunctions) {
  for (let i = 0; i < configs.length; i++) {
    const conf = configs[i];
    const pin = Number(conf["values"]["pin"]?.[0]);
    if (pin < pinFunctions.length) {
      pinFunctions[pin].push(niceName(conf["name"]));
    }
  }
}


function checkEngine(configs, warnings, notes, pinFunctions) {
  for (let i = 0; i < configs.length; i++) {
    const conf = configs[i];
    const engineType = Number(conf["values"]["engineType"][0]);
    const idleManager = conf["values"]["idleManager"][0];
    const rpmIdle = Number(idleManager["rpmIdleRunning"][0]);
    const wheelDiameter = Number(conf["values"]["wheelDiameter"][0]);

    const gears = conf["values"]?.["fullGears"]?.[0];
    if (gears == null) {
      gears = conf["values"]?.["gears"]?.[0];
    }
    if (gears == null) {
      gears = [];
    }

    const rpmMax = Number(conf["values"]["rpmMax"][0]);

    if (rpmIdle < 0.0) {
      warnings.push("Negative idle RPM won't work for proc " +
        niceName(configs[0]["name"]) + ".");
    }
    if (rpmMax < 0.0) {
      warnings.push("Negative max RPM won't work for proc " +
        niceName(configs[0]["name"]) + ".");
    }
    if (rpmMax < rpmIdle) {
      warnings.push("Max RPM should be bigger than idle RPM for proc " +
        niceName(configs[0]["name"]) + ".");
    }
    if ((rpmIdle == 0.0) &&
      (engineType != 0 && engineType != 4)) {
      warnings.push("Zero idle RPM only works for electric or steam engines for proc " +
        niceName(configs[0]["name"]) + ".");
    }
  }
}


/** Marks the pins down in for an pwm and esc proc. */
function checkEsc(configs, warnings, notes, pinFunctions) {

  for (let i = 0; i < configs.length; i++) {
    const conf = configs[i];
    const types = conf["types"]["types"];
    const pins = conf["values"]?.["pins"];
    const pins2 = conf["values"]?.["pins2"];
    for (let j = 0; j < types.length; j++) {
      if (types[j] != "ST_NONE") {
        const configName = niceName(conf["name"]) + " / " + niceName(types[j]);
        if (pins && pins?.[j]) {
          const pinNum = Number(pins[j]);
          pinFunctions[pinNum].push(configName);
          checkPinOutput(configName, pinNum, warnings, notes);
        }
        if (pins2 && pins2?.[j]) {
          const pinNum = Number(pins2[j]);
          pinFunctions[pinNum].push(configName);
          checkPinOutput(configName, pinNum, warnings, notes);
        }
      }
    }
  }
}


/** Marks the pins down in for an led proc. */
function checkLed(configs, warnings, notes, pinFunctions) {
  const pinMapping = [2, 3, 4, 5, 15, 16, 17, 18, 19, 21, 22, 23, 32];

  for (let i = 0; i < configs.length; i++) {
    const conf = configs[i];
    const types = conf["types"]["types"];
    for (let j = 0; j < types.length && j < pinMapping.length; j++) {
      if (types[j] != "ST_NONE") {
        pinFunctions[pinMapping[j]].push(niceName(conf["name"]) + " / " + niceName(types[j]));
      }
    }
  }
}


/** A map of proc-check functions.
 *
 * Each checker function get's the proc configs itself (as an array) and three arrays
 * with warnings, notes and pin functions.
 */
const procCheckers = {
  "INPUT_DEMO": (c, w, n, p) => checkOnlyOne(c, w, n, p),
  "INPUT_ADC": (c, w, n, p) => {
    checkADC(c, w, n, p);
  },
  "INPUT_PIN": (c, w, n, p) => {
    checkPin(c, w, n, p);
  },
  "INPUT_PWM": (c, w, n, p) => {
    checkPin(c, w, n, p);
  },
  "INPUT_PPM": (c, w, n, p) => {
    checkPpm(c, w, n, p);
  },
  "INPUT_SBUS": (c, w, n, p) => {
    checkPpm(c, w, n, p);
  },
  "INPUT_SRXL": (c, w, n, p) => {
    checkPpm(c, w, n, p);
  },

  "ENGINE_REVERSE": (c, w, n, p) => {
    checkOnlyOne(c, w, n, p);
    checkEngine(c, w, n, p);
  },
  "ENGINE_BRAKE": (c, w, n, p) => {
    checkOnlyOne(c, w, n, p);
    checkEngine(c, w, n, p);
  },
  "ENGINE_GEAR": (c, w, n, p) => {
    checkOnlyOne(c, w, n, p);
    checkEngine(c, w, n, p);
  },
  "ENGINE_SIMPLE": (c, w, n, p) => {
    checkOnlyOne(c, w, n, p);
    checkEngine(c, w, n, p);
  },

  "OUTPUT_AUDIO": (c, w, n, p) => {
    checkOnlyOne(c, w, n, p);
    p[25].push(niceName(c[0]["name"]) + " / Channel 1");
    p[26].push(niceName(c[0]["name"]) + " / Channel 2");
  },
  "OUTPUT_PWM": (c, w, n, p) => {
    checkEsc(c, w, n, p);
  },
  "OUTPUT_ESC": (c, w, n, p) => {
    checkEsc(c, w, n, p);
  },
  "OUTPUT_LED": (c, w, n, p) => {
    checkOnlyOne(c, w, n, p);
    checkLed(c, w, n, p);
  },
};


/** Returns a "nice", more human readable name for
 *  proc and signal names.
 */
function niceName(name) {
  if (name == null) {
    name = "";
  }
  name = name.replace(/^PROC_/, "");
  name = name.replace(/^ST_/, "");
  name = name.replaceAll("_", " ");
  name = name.toLowerCase();
  return name;
}


/** pinout of the esp32 30pin dev board */
const pinout30 = [
  ["3V3", "3V3"],
  ["GND", "GND"],
  ["D15", "IO15", 15],
  ["D2", "IO2", 2],
  ["D4", "IO4", 4],
  ["D16", "IO16", 16],
  ["D17", "IO17", 17],
  ["D5", "IO5", 5],
  ["D18", "IO18", 18],
  ["D19", "IO19", 19],
  ["D21", "IO21", 21],
  ["D3", "IO3", 3],
  ["D1", "IO1", 1],
  ["D22", "IO22", 22],
  ["D23", "IO23", 23],

  ["VIN", "VIN"],
  ["GND", "GND"],
  ["D13", "IO13", 13],
  ["D12", "IO12", 12],
  ["D14", "IO14", 14],
  ["D27", "IO27", 27],
  ["D26", "IO26", 26],
  ["D25", "IO25", 25],
  ["D33", "IO33", 33],
  ["D32", "IO32", 32],
  ["D35", "IO35", 35],
  ["D34", "IO34", 34],
  ["D39", "VN", 39],
  ["D36", "VP", 36],
  ["EN", "EN"],
]

/** Returns three table cell elements for a pin.
 *
 *  First one contains the pin text, second the line, third the inside cpu text
 *
 *  @param i Index of the pin from the top
 */
function getPinCells(i, onePinout, pinFunctions) {

  // the cell for the pin text
  let elTd1 = document.createElement('td');

  let pinoutText = "";
  // text for functionality
  if (onePinout.length > 2) {
    const gpio = onePinout[2];
    if (pinFunctions[gpio].length > 0) {
      for (let i = 0; i < pinFunctions[gpio].length; i++) {
        pinoutText += " " + pinFunctions[gpio][i];
      }
      if (pinFunctions[gpio].length > 1) {
        elTd1.style.color = "red";
      }
    } else {
      pinoutText = onePinout[0];
    }
  }
  elTd1.textContent = pinoutText;

  // the cell for the pin line
  let elTd2 = document.createElement('td');
  elTd2.style.padding = "0";
  let elHr = document.createElement('hr');
  elTd2.appendChild(elHr);

  // the cell for the inside cpu pin text
  let elTd3 = document.createElement('td');
  elTd3.textContent = onePinout[1];

  // every second pin is a little longer
  // to keep the descriptions separated and make it look
  // nice.
  if ((i % 2) > 0) {
    elTd1.setAttribute("colspan", 2);
  } else {
    elTd2.setAttribute("colspan", 2);
  }

  return [elTd1, elTd2, elTd3];
}

/** Returns a DOM element for the Dev-board
 *
 *  The table looks like this:
 *
 *  pin | pin2 | pin line | inside pin | spacing | inside pin | pin line2 | pin3 | pin4
 *
 *  @param pinFunctions An array of arrays for the GPIO functions as returned
 *    by the checkers.
 *
 *  @returns The DOM element (currently a table)
 */
function createPinout(pinFunctions) {

    let pinout = pinout30;
    const pinNum = Math.ceil(pinout.length / 2);  // number of pins down one side

    let elTable = document.createElement("table");

    // -- top part of the cpu drawing
    let elTrTop = document.createElement('tr');

    let elTdT1 = document.createElement('td');
    elTrTop.appendChild(elTdT1); // pin1

    let elTdT2 = document.createElement('td');
    elTdT2.style.width = "2em";
    elTrTop.appendChild(elTdT2); // pin2

    let elTdT3 = document.createElement('td');
    elTdT3.style.width = "2em";
    elTrTop.appendChild(elTdT3); // pin3

    let elTdT4 = document.createElement('td');
    elTdT4.classList.add("cpuBorderLeft");
    elTdT4.classList.add("cpuBorderTop");
    elTrTop.appendChild(elTdT4); // inside

    let elTdT5 = document.createElement('td');
    elTdT5.style.width = "2em";
    elTdT5.classList.add("cpuBorderTop");
    elTdT5.textContent = "ESP32-WROOM32 DevKit";
    elTrTop.appendChild(elTdT5); // inside

    let elTdT6 = document.createElement('td');
    elTdT6.classList.add("cpuBorderRight");
    elTdT6.classList.add("cpuBorderTop");
    elTrTop.appendChild(elTdT6); // inside

    let elTdT7 = document.createElement('td');
    elTdT7.style.width = "2em";
    elTrTop.appendChild(elTdT7); // pin4

    let elTdT8 = document.createElement('td');
    elTdT8.style.width = "2em";
    elTrTop.appendChild(elTdT8); // pin5

    let elTdT9 = document.createElement('td');
    elTrTop.appendChild(elTdT9); // pin6

    elTable.appendChild(elTrTop);

    // -- pins
    for (let i = 0; i < pinNum && i < pinout.length; i++) {
      let elTr = document.createElement('tr');

      // left
      let [elTd1, elTd2, elTd3] = getPinCells(i, pinout[i], pinFunctions);

      elTd1.style.textAlign = "right";
      elTr.appendChild(elTd1);

      elTr.appendChild(elTd2);

      elTd3.style.textAlign = "left";
      elTd3.classList.add("cpuBorderLeft");
      elTr.appendChild(elTd3);

      let elTdS = document.createElement('td'); // spacing inside
      elTr.appendChild(elTdS);

      // right
      if (i + pinNum < pinout.length) {
        let [elTd4, elTd5, elTd6] = getPinCells(i + pinNum, pinout[i + pinNum], pinFunctions);

        elTd6.style.textAlign = "left";
        elTd6.classList.add("cpuBorderRight");
        elTr.appendChild(elTd6);

        elTr.appendChild(elTd5);

        elTd4.style.textAlign = "left";
        elTr.appendChild(elTd4);
      }

      elTable.appendChild(elTr);
    }

    // -- bottom part of the cpu drawing
    let elTrBottom = document.createElement('tr');

    let elTdB1 = document.createElement('td');
    elTrBottom.appendChild(elTdB1); // pin1

    let elTdB2 = document.createElement('td');
    elTdB2.style.width = "2em";
    elTrBottom.appendChild(elTdB2); // pin2

    let elTdB3 = document.createElement('td');
    elTdB3.style.width = "2em";
    elTrBottom.appendChild(elTdB3); // pin3

    let elTdB4 = document.createElement('td');
    elTdB4.classList.add("cpuBorderLeft");
    elTdB4.classList.add("cpuBorderBottom");
    elTrBottom.appendChild(elTdB4); // inside

    let elTdB5 = document.createElement('td');
    elTdB5.style.width = "2em";
    elTdB5.classList.add("cpuBorderBottom");
    elTrBottom.appendChild(elTdB5); // inside

    let elTdB6 = document.createElement('td');
    elTdB6.classList.add("cpuBorderRight");
    elTdB6.classList.add("cpuBorderBottom");
    elTrBottom.appendChild(elTdB6); // inside

    let elTdB7 = document.createElement('td');
    elTdB7.style.width = "2em";
    elTrBottom.appendChild(elTdB7); // pin4

    let elTdB8 = document.createElement('td');
    elTdB8.style.width = "2em";
    elTrBottom.appendChild(elTdB8); // pin5

    let elTdB9 = document.createElement('td');
    elTrBottom.appendChild(elTdB9); // pin6

    elTable.appendChild(elTrBottom);

    return elTable;
}


/** Checks if input functions are in front of all other functions.
 *
 *  @param config An array with the configuration.
 *  @param warnings An array with warnings. If a warning is produced,
 *    it should be added there.
 */
function checkInputsFirst(config, warnings) {

  let otherProcFound = false;
  for (let i = 0; i < config.length; i++) {
    const conf = config[i];

    if (conf?.["name"] == "PROC_GROUP")  {
      ;  // ignore groups
    } else if (procsBySection["Input"].includes(conf?.["name"]))  {
      if (otherProcFound) {
        warnings.push("The input proc " + niceName(conf?.["name"]) + " is after a non-input proc.");
      }
    } else {
      otherProcFound = true;
    }
  }
}


/** Checks if output functions are the last of all other functions.
 *
 *  @param config An array with the configuration.
 *  @param warnings An array with warnings. If a warning is produced,
 *    it should be added there.
 */
function checkOutputsLast(config, warnings) {
  let otherProcFound = false;

  // go reverse
  for (let i = config.length - 1; i >= 0; i--) {
    const conf = config[i];

    if (conf?.["name"] == "PROC_GROUP")  {
      ;  // ignore groups
    } else if (procsBySection["Output"].includes(conf?.["name"]))  {
      if (otherProcFound) {
        warnings.push("The output proc " + niceName(conf?.["name"]) + " is before a non-output proc.");
      }
    } else {
      otherProcFound = true;
    }
  }
}

/** Fills the different divs in the HW table.
 */
function fillTable() {

  let warnings = [];
  let notes = [];

  // a list of list with functions of the IO pins
  let pinFunctions = [];
  for (let i = 0; i < 40; i++) {
    pinFunctions.push([]);
  }


  const config = getConfigFromTable();
  checkInputsFirst(config, warnings);
  checkOutputsLast(config, warnings);


  // collect confs with the same name into a map and
  // then feed it to the checkers
  let confNames = [];
  let confsByName = {};
  for (let i = 0; i < config.length; i++) {
    const conf = config[i];
    const name = conf["name"];
    if (!(name in confsByName)) {
      confNames.push(name);
      confsByName[name] = [];
    }
    confsByName[name].push(conf);
  }
  
  // now call the checker functions if awailable
  for (let i = 0; i < confNames.length; i++) {
    const name = confNames[i];
    if (name in procCheckers) {
      procCheckers[name](confsByName[name], warnings, notes, pinFunctions);
    }
  }

  const elPinout = document.getElementById("hwPinout");
  elPinout.innerHTML = "";

  elPinout.appendChild(createPinout(pinFunctions));

  const elWarnings = document.getElementById("hwWarnings");
  elWarnings.innerHTML = "";
  if (warnings.length == 0) {
    let elText = document.createElement("div");
    elText.textContent = "none";
    elWarnings.appendChild(elText);
  } else {
    for (let i = 0; i < warnings.length; i++ ) {
      let elText = document.createElement("div");
      elText.textContent = warnings[i];
      elWarnings.appendChild(elText);
    }
  }

  const elNotes = document.getElementById("hwNotes");
  elNotes.innerHTML = "";
  if (notes.length == 0) {
    let elText = document.createElement("div");
    elText.textContent = "none";
    elNotes.appendChild(elText);
  } else {
    for (let i = 0; i < notes.length; i++ ) {
      let elText = document.createElement("div");
      elText.textContent = notes[i];
      elNotes.appendChild(elText);
    }
  }

}


export {
  fillTable,
}
