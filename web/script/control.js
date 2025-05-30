/** Javascript module for handling Signals
 *
 *  For the rc functions controller.
 */

import {defSignals} from "./def_signals.js";
import {SimpleInputStream} from "./simple_stream.js";
import * as bluetooth from "./bluetooth.js";

let signalNameMapping = {}; // mapping: signal name to index
let signalIndex = 0;
for (let i = 0; i < defSignals.length; i++ ) {
    let signal = defSignals[i]

    if ("index" in signal) {
        signalIndex = signal["index"];
    }
    if ("name" in signal) {
      signalNameMapping[signal["name"]] = signalIndex;
      signal["index"] = signalIndex;
      signalIndex++;
    }
}

const NUM_SIGNALS = signalIndex;
const RCSIGNAL_INVALID = -32768;

/** The current validated gear.
 *
 *  Use setGear() to modify an set the UI at the same time.
 */
let currentGear = 0;

/** An array with the override signals that we want to send out. */
let overrideSignals = [];
for (let i = 0; i < NUM_SIGNALS; i++) {
  overrideSignals.push(RCSIGNAL_INVALID);
}
overrideSignals[signalNameMapping["ST_THROTTLE"]] = 0;
overrideSignals[signalNameMapping["ST_YAW"]] = 0;

let buttons = [
  {
    "el": document.getElementById("buttonLowbeam"),
    "signalName": "ST_LOWBEAM",
  },
  {
    "el": document.getElementById("buttonHighbeam"),
    "signalName": "ST_HIGHBEAM",
  },
  {
    "el": document.getElementById("buttonHorn"),
    "signalName": "ST_HORN",
    "toggle": false
  },
  {
    "el": document.getElementById("buttonSiren"),
    "signalName": "ST_SIREN",
    "toggle": false
  },
  {
    "el": document.getElementById("buttonHazard"),
    "signalName": "ST_LI_HAZARD",
  }
];

// install event handlers for buttons
for (let i = 0; i < buttons.length; i++) {
  let buttonEntry = buttons[i];

  // for horn and siren
  if ("toggle" in buttonEntry && buttonEntry["toggle"] == false) {
    buttonEntry["el"].addEventListener("pointerdown",
      (evt) => {buttonClicked(evt, buttonEntry["el"], buttonEntry["signalName"])});
      buttonEntry["el"].toggleAttribute("auto", true);
    buttonEntry["el"].addEventListener("pointerup",
      (evt) => {buttonClicked(evt, buttonEntry["el"], buttonEntry["signalName"])});
      buttonEntry["el"].toggleAttribute("auto", true);

  // normal button
  } else {
    buttonEntry["el"].addEventListener("click",
      (evt) => {buttonClicked(evt, buttonEntry["el"], buttonEntry["signalName"])});
      buttonEntry["el"].toggleAttribute("auto", true);
  }
}

// ----- gear switches
const elTextGear = document.getElementById("textGear");
const elSwitchGearUp = document.getElementById("switchGearUp");
const elSwitchGearDown = document.getElementById("switchGearDown");
const elSwitchGearAuto = document.getElementById("switchGearAuto");

elSwitchGearAuto.addEventListener("click", () => {
  const auto = elSwitchGearAuto.getAttribute("active");

  if (auto) {
      elSwitchGearAuto.removeAttribute("active");
      elSwitchGearUp.removeAttribute("disabled");
      elSwitchGearDown.removeAttribute("disabled");

      overrideSignals[signalNameMapping["ST_GEAR"]] = currentGear;
  } else {
      elSwitchGearAuto.setAttribute("active", true);
      elSwitchGearUp.setAttribute("disabled", true);
      elSwitchGearDown.setAttribute("disabled", true);

      overrideSignals[signalNameMapping["ST_GEAR"]] = RCSIGNAL_INVALID;
  }
});


elSwitchGearUp.addEventListener("click", () => {
  const auto = elSwitchGearAuto.getAttribute("active");
  if (!auto) {
    overrideSignals[signalNameMapping["ST_GEAR"]] = currentGear + 1;

    setGear(currentGear);
    triggerOverrideSignals();
  }
});


elSwitchGearDown.addEventListener("click", () => {
  const auto = elSwitchGearAuto.getAttribute("active");
  if (!auto) {
    overrideSignals[signalNameMapping["ST_GEAR"]] = currentGear - 1;

    setGear(currentGear);
    triggerOverrideSignals();
  }
});


/** Set's the gear and updates the UI */
function setGear(newGear) {
  if (newGear == RCSIGNAL_INVALID) {
    return;
  }

  currentGear = newGear;

  // if the manual gear selected is different from
  // the returned gear, display an arrow after the gear
  const auto = elSwitchGearAuto.getAttribute("active");
  let arrow = "\u{00A0}";
  if (!auto) {
    const manualGear = overrideSignals[signalNameMapping["ST_GEAR"]];
    if (manualGear > newGear) {
      arrow = "\u{2191}";
      arrow = "\u{21E7}";
      // arrow = "\u{2BC5}";
    } else if (manualGear < newGear) {
      arrow = "&#8595;";
      arrow = "&#8681;";
      arrow = "\u{2BC6}";
    } 
  }

  if (newGear == 0) {
    newGear = "N";
  }

  elTextGear.textContent = newGear + arrow;
}

// ------ circular progress bar

/** Fill the svg element with the values.
 */
function createRoundMeter(elSvg, bigText, smallText,
    minValue,
    maxValue,
    stepValue,
    stepNumbers) {

  let svgNS = elSvg.namespaceURI;

  // outer circle
  let elOutline = document.createElementNS(svgNS, 'circle');
  elOutline.setAttribute("class", "meterOutline");
  elOutline.setAttribute("cx", 0);
  elOutline.setAttribute("cy", 0);
  elOutline.setAttribute("r", 47);
  elSvg.appendChild(elOutline);

  // marks
  const startAngle = -120;
  const totalAngle = 300;
  let angleStep = totalAngle / ((maxValue - minValue) / stepValue);
  for (let angle = startAngle; angle < (totalAngle + startAngle); angle += angleStep) {
    let elLine = document.createElementNS(svgNS, 'line');
    elLine.setAttribute("class", "meterMark");
    elLine.setAttribute("x1", 0);
    elLine.setAttribute("y1", -40);
    elLine.setAttribute("x2", 0);
    elLine.setAttribute("y2", -45);
    elLine.setAttribute("transform", "rotate(" + angle + ")");
    elSvg.appendChild(elLine);
  }

  angleStep = totalAngle / ((maxValue - minValue) / stepNumbers);
  let number = minValue;
  for (let angle = startAngle; angle < (totalAngle + startAngle); angle += angleStep) {
    let elNumber = document.createElementNS(svgNS, 'text');
    elNumber.setAttribute("class", "meterNumbers");
    elNumber.setAttribute("x", Math.sin(angle / 180.0 * Math.PI) * 30.0);
    elNumber.setAttribute("y", Math.cos(angle / 180.0 * Math.PI) * -30.0);
    elNumber.setAttribute("dominant-baseline", "middle");
    elNumber.setAttribute("text-anchor", "middle");
    let elNumberText = document.createTextNode(number);
    number += stepNumbers;
    elNumber.appendChild(elNumberText);
    elSvg.appendChild(elNumber);
  }

  // big text
  let meterBigText = document.createElementNS(svgNS, 'text');
  meterBigText.setAttribute("class", "meterBigText");
  meterBigText.setAttribute("x", 0);
  meterBigText.setAttribute("y", 18);
  meterBigText.setAttribute("dominant-baseline", "middle");
  meterBigText.setAttribute("text-anchor", "middle");
  let elBigTextNode = document.createTextNode(bigText);
  meterBigText.appendChild(elBigTextNode);
  elSvg.appendChild(meterBigText);

  // small text
  let meterSmallText = document.createElementNS(svgNS, 'text');
  meterSmallText.setAttribute("class", "meterSmallText");
  meterSmallText.setAttribute("x", 0);
  meterSmallText.setAttribute("y", 28);
  meterSmallText.setAttribute("dominant-baseline", "middle");
  meterSmallText.setAttribute("text-anchor", "middle");
  let elSmallTextNode = document.createTextNode(smallText);
  meterSmallText.appendChild(elSmallTextNode);
  elSvg.appendChild(meterSmallText);

  // pointer
  let elPointer = document.createElementNS(svgNS, 'polygon');
  elPointer.setAttribute("class", "meterPointer");
  elPointer.setAttribute("points", "-2,7 -1,-35 1,-35 2,7");
  elPointer.setAttribute("transform", "rotate(0)");
  elSvg.appendChild(elPointer);

  // center
  let elCenter = document.createElementNS(svgNS, 'circle');
  elCenter.setAttribute("class", "meterCenter");
  elCenter.setAttribute("cx", 0);
  elCenter.setAttribute("cy", 0);
  elCenter.setAttribute("r", 5);
  elSvg.appendChild(elCenter);


  elSvg.setSignal = (value, valueStr) => {
    elPointer.setAttribute("transform",
        "rotate(" + ((totalAngle / (maxValue - minValue)) * value + startAngle) + ")");
    if (valueStr) {
      elSmallTextNode.data = valueStr;
    }
  }

  elSvg.setSignal(minValue);
}

const elSpeedBar = document.getElementById("speedBar");
const elRpmBar = document.getElementById("rpmBar");

createRoundMeter(elSpeedBar, "SPEED", "percent", 0, 100, 10, 20);
createRoundMeter(elRpmBar, "RPM", "100 rpm", 0, 10, 1, 2);


// ----- direction control

// install event handler for navigation/direction control
const directionCtrlCircle = document.getElementById("directionCtrlCircle");
const directionCtrl = document.getElementById("directionCtrlSvg");
let cursorPoint = directionCtrl.createSVGPoint(); // helper point for getCoordinates
let mousePressed = false;

/** Returns an svg point with the event coordinates in svg space */
function getCoordinates(evt) {
  cursorPoint.x = evt.clientX;
  cursorPoint.y = evt.clientY;
  return cursorPoint.matrixTransform(directionCtrl.getScreenCTM().inverse());
}

function directionDownEvent(evt) {
  directionMoveEvent(evt);
  directionCtrl.addEventListener("pointermove", directionMoveEvent)
  directionCtrl.setPointerCapture(evt.pointerId);
}

function directionUpEvent(evt) {
  directionCtrlCircle.setAttribute('cx', 0);
  directionCtrlCircle.setAttribute('cy', 0);

  overrideSignals[signalNameMapping["ST_THROTTLE"]] = RCSIGNAL_INVALID;
  overrideSignals[signalNameMapping["ST_YAW"]] = RCSIGNAL_INVALID;
  triggerOverrideSignals();

  directionCtrl.removeEventListener("pointermove", directionMoveEvent)
  directionCtrl.releasePointerCapture(evt.pointerId);
}

function directionMoveEvent(evt) {
  const loc = getCoordinates(evt);
  directionCtrlCircle.setAttribute('cx', loc.x);
  directionCtrlCircle.setAttribute('cy', loc.y);

  // TODO: should we clamp?
  overrideSignals[signalNameMapping["ST_THROTTLE"]] = -loc.y * 20.0;
  overrideSignals[signalNameMapping["ST_YAW"]] = loc.x * 20.0;
  triggerOverrideSignals();
}

directionCtrl.addEventListener("pointerdown", directionDownEvent);
directionCtrl.addEventListener("pointerup", directionUpEvent);


/** Update the status of the button div for a specific signal. */
function updateButton(elButton, signalName, signalValue) {
  if (overrideSignals[signalNameMapping[signalName]] == RCSIGNAL_INVALID) {
      if (signalValue > 100) {
        elButton.setAttribute("status", "on");
      } else {
        elButton.setAttribute("status", "off");
      }
  }
}

/** Handles the button clicked event. */
function buttonClicked(evt, elButton, signalName) {

  const auto = elButton.getAttribute("auto");
  const stat = elButton.getAttribute("status");

  if (stat == "on") {
    elButton.setAttribute("status", "off");
    elButton.removeAttribute("auto");
    overrideSignals[signalNameMapping[signalName]] = 0;

  } else {
    elButton.setAttribute("status", "on");
    elButton.removeAttribute("auto");
    overrideSignals[signalNameMapping[signalName]] = 1000;
  }

  triggerOverrideSignals();
}

/** Updates the UI (the signals table) with the values
 *  from the data view.
 *
 *  @param dataView a DataView object containing the received data.
 */
function updateSignals(dataView) {
  const signals = new SimpleInputStream(dataView).readSignals(NUM_SIGNALS);

  // the buttons
  for (let i = 0; i < buttons.length; i++) {
    let buttonEntry = buttons[i];
    updateButton(
        buttonEntry["el"],
        buttonEntry["signalName"],
        signals[signalNameMapping[buttonEntry["signalName"]]]);
  }

  // gear
  setGear(signals[signalNameMapping["ST_GEAR"]]);

  let rpm = signals[signalNameMapping["ST_RPM"]];
  let rpmStr;
  if (rpm == RCSIGNAL_INVALID) {
    rpm = -1;
    rpmStr = "--";
  } else {
    rpmStr = rpm + " RPM";
    rpm = rpm / 10;
  }
  rpmBar.setSignal(rpm / 100, rpmStr);

  let speed = signals[signalNameMapping["ST_SPEED"]];
  let speedStr;
  if (speed == RCSIGNAL_INVALID) {
    speed = -1;
    speedStr = "--";
  } else {
    speed = speed / 10;
    speedStr = "speed: " + speed + "%";
  }
  speedBar.setSignal(speed, speedStr);
}


function getOverrideSignals() {
  const buffer = new ArrayBuffer(NUM_SIGNALS * 2);
  const dataView = new DataView(buffer);

  for (let i = 0; i < NUM_SIGNALS; i++ ) {
    dataView.setInt16(i * 2, overrideSignals[i]);
  }
  return dataView;
}


/** Variable to remember if a timer is already active. */
var overrideTimer = 0;

/** This function triggers writing of the override signals
 *  via BT.
 *
 *  It ensures a minimum delay between sending signals
 *  to enable finishing of the previous BT communication.
 */
function triggerOverrideSignals() {

  if (overrideTimer == 0) {
    // send out immediately and remember to delay the next transmission.
    bluetooth.uploadSignals(getOverrideSignals());
  }
  overrideTimer = setTimeout(() => {
      bluetooth.uploadSignals(getOverrideSignals());
      overrideTimer = 0;
  }, 200);
}

export {
  updateSignals,
}

