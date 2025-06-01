/** Javascript module for handling Signals
 *
 *  For the rc functions controller.
 */

import {defSignals} from "./def_signals.js";
import {SimpleInputStream} from "./simple_stream.js";
import * as bluetooth from "./bluetooth.js";

// add a number to all signals and count them.
let signalIndex = 0;
for (let i = 0; i < defSignals.length; i++ ) {
    let signal = defSignals[i]

    if ("index" in signal) {
        signalIndex = signal["index"];
    }
    if ("name" in signal) {
      signal["index"] = signalIndex;
      signalIndex++;
    }
}

const NUM_SIGNALS = signalIndex;

const RCSIGNAL_INVALID = -32768;

const signalTableId = "signalsTable";

/** Returns a DOM element for signals slider
 *
 *  The returned element has "setSignal" and "setOverride"
 *  functions to set the values.
 *
 *  @returns the svg element
 */
function createSignalSlider() {
    const width = 200; // positive and negative width in svg units
    const height = 40; // overall height of the bar svg
    const barHeight = 5;  // height of the bar itself
    const barDist = 10;  // space between bar and checkmarks
    const sigBarHeight = 16;  // height of the signal bar itself
    const sigBarDist = 4;  // space between signal bar and checkmarks
    const markerRadius = 10;
    const checkmarkHeight = 5; // height for the checkmarks they start at y-pos 0;
    const maxSignal = 1200; // positive and negative slider range
    const checkmarks = [
      [-1000, 2],
      [-500, 2],
      [0, 2],
      [500, 2],
      [1000, 2],
    ];

    let elSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    elSvg.setAttribute("viewBox",
      "" + (-width / 2) + " " + (-height + checkmarkHeight) + " " +
      (width) + " " + (height)); 
    elSvg.setAttribute("width", "10em");
    let svgNS = elSvg.namespaceURI;

    // background bar
    let elRect = document.createElementNS(svgNS, 'rect');
    elRect.setAttribute("class", "progressBar");
    elRect.setAttribute("x", -width / 2);
    elRect.setAttribute("y", -barHeight - barDist);
    elRect.setAttribute("width", width);
    elRect.setAttribute("height", barHeight);
    elSvg.appendChild(elRect);

    // checkmarks
    for (let i = 0; i < checkmarks.length; i++) {
      const checkmark = checkmarks[i];

      let elLine = document.createElementNS(svgNS, 'line');
      elLine.setAttribute("class", "checkMark");
      elLine.setAttribute("x1", width / 2 * checkmark[0] / maxSignal);
      elLine.setAttribute("y1", 0);
      elLine.setAttribute("x2", width / 2 * checkmark[0] / maxSignal);
      elLine.setAttribute("y2", checkmarkHeight);
      elSvg.appendChild(elLine);
    }

    // signals bar
    let elSBar = document.createElementNS(svgNS, 'rect');
    elSBar.setAttribute("class", "signalBar");
    elSBar.setAttribute("x", 0);
    elSBar.setAttribute("y", -sigBarHeight - sigBarDist);
    elSBar.setAttribute("width", 0);
    elSBar.setAttribute("height", sigBarHeight);
    elSvg.appendChild(elSBar);

    // override bar
    let elOBar = document.createElementNS(svgNS, 'rect');
    elOBar.setAttribute("class", "overrideBar");
    elOBar.setAttribute("x", 0);
    elOBar.setAttribute("y", -sigBarHeight - sigBarDist);
    elOBar.setAttribute("width", 0);
    elOBar.setAttribute("height", sigBarHeight);
    elSvg.appendChild(elOBar);

    // signal circle
    let elSMark = document.createElementNS(svgNS, 'circle');
    elSMark.setAttribute("class", "signalMark");
    elSMark.setAttribute("cx", 0);
    elSMark.setAttribute("cy", -barHeight / 2 - barDist);
    elSMark.setAttribute("r", markerRadius);
    elSvg.appendChild(elSMark);

    // signal text
    let elSText = document.createElementNS(svgNS, 'text');
    elSText.setAttribute("class", "signalText");
    elSText.setAttribute("x", 0);
    elSText.setAttribute("y", -barHeight / 2 - barDist + 1);
    elSText.setAttribute("dominant-baseline", "middle");
    elSText.setAttribute("text-anchor", "middle");
    let elSTextNode = document.createTextNode("XX");
    elSText.appendChild(elSTextNode);
    elSvg.appendChild(elSText);

    // override circle
    let elOMark = document.createElementNS(svgNS, 'circle');
    elOMark.setAttribute("class", "overrideMark");
    elOMark.setAttribute("cx", width * 2);
    elOMark.setAttribute("cy", -barHeight / 2 - barDist);
    elOMark.setAttribute("r", markerRadius);
    elSvg.appendChild(elOMark);

    // override text
    let elOText = document.createElementNS(svgNS, 'text');
    elOText.setAttribute("class", "overrideText");
    elOText.setAttribute("x", width * 2);
    elOText.setAttribute("y", -barHeight / 2 - barDist + 1);
    elOText.setAttribute("dominant-baseline", "middle");
    elOText.setAttribute("text-anchor", "middle");
    let elOTextNode = document.createTextNode("XX");
    elOText.appendChild(elOTextNode);
    elSvg.appendChild(elOText);

    // -- setter and getter functions

    let signalValue = 0;
    elSvg.setSignal = (value) => {
      signalValue = value; // remember the value for getOverride
      if (value != RCSIGNAL_INVALID) {
        elSTextNode.nodeValue = value;
        value = Math.max(Math.min(value, maxSignal), -maxSignal); // clamp
        elSText.setAttribute("x", (width / 4) * value / maxSignal);
        elSMark.setAttribute("cx", (width / 2) * value / maxSignal);
        if (value < 0) {
          elSBar.setAttribute("x", (width / 2) * value / maxSignal);
          elSBar.setAttribute("width", (-width / 2) * value / maxSignal);
        } else {
          elSBar.setAttribute("x", 0);
          elSBar.setAttribute("width", width / 2 * value / maxSignal);
        }
      } else {
        elSText.setAttribute("x", width); // move it outside
        elSMark.setAttribute("cx", width); // move it outside
        elSBar.setAttribute("x", 0);
        elSBar.setAttribute("width", 0);
      }
    }
    elSvg.getSignal = () => {
      return signalValue;
    }

    let overrideValue = RCSIGNAL_INVALID;
    elSvg.setOverride = (value) => {
      overrideValue = value; // remember the value for getOverride
      if (value != RCSIGNAL_INVALID) {
        elOTextNode.nodeValue = value;
        value = Math.max(Math.min(value, maxSignal), -maxSignal); // clamp
        elOText.setAttribute("x", (width / 4) * value / maxSignal);
        elOMark.setAttribute("cx", (width / 2) * value / maxSignal);
        if (value < 0) {
          elOBar.setAttribute("x", (width / 2) * value / maxSignal);
          elOBar.setAttribute("width", (-width / 2) * value / maxSignal);
        } else {
          elOBar.setAttribute("x", 0);
          elOBar.setAttribute("width", (width / 2) * value / maxSignal);
        }
      } else {
        elOText.setAttribute("x", width); // move it outside
        elOMark.setAttribute("cx", width); // move it outside
        elOBar.setAttribute("x", 0);
        elOBar.setAttribute("width", 0);
      }
    }

    elSvg.getOverride = () => {
      return overrideValue;
    }

    // --- event handling
    let cursorPoint = elSvg.createSVGPoint(); // helper point for getCoordinates
    let mousePressed = false;

    // Returns an svg point with the event coordinates in svg space
    let getCoordinates = function(evt) {
      cursorPoint.x = evt.clientX;
      cursorPoint.y = evt.clientY;
      return cursorPoint.matrixTransform(elSvg.getScreenCTM().inverse());
    }

    elSvg.addEventListener("pointerdown",
        (evt) => {
          mousePressed = true;

          const loc = getCoordinates(evt);
          elSvg.setOverride(Math.round(loc.x * maxSignal * 2 / width));
      }
    );

    return elSvg;
}

/** Returns a DOM element for signals row
 *
 *  The row will get:
 *  - an id: signalRow_ST_THROTTLE
 *  - a function setValue(signalValue) that will set the sliders
 *  - a function getOverride() returning the current value
 *
 *  @param signal: An entry from the defSignal array.
 *  @returns the tr element
 */
function createSignalRow(signal) {
    let elTr = document.createElement("tr");
    if ("name" in signal) {
      elTr.id = "signalRow_" + signal["name"];
    }

    if ("name" in signal) {
      let niceName = signal["name"];
      niceName = niceName.replace(/^ST_/, "");
      niceName = niceName.replaceAll("_", " ");
      niceName = niceName.toLowerCase();

      let elTd1 = document.createElement("td");
      elTd1.textContent = niceName + ":";
      elTd1.title = signal["description"];
      elTr.appendChild(elTd1);

      let elTd2 = document.createElement("td");
      let elSlider = createSignalSlider();
      elTd2.appendChild(elSlider);
      elTr.appendChild(elTd2);

      let elTd3 = document.createElement("td");
      let elOverride = document.createElement("span");
      elOverride.className = "button"
      elOverride.textContent = "!"
      elOverride.id = "signalOverride" + signal["name"];
      elOverride["slider"] = elSlider;
      elSlider["override"] = elOverride;
      elTd3.appendChild(elOverride);
      elTr.appendChild(elTd3);

      elTr.setValue = (value) => {
        elSlider.valueAsNumber = value;
        elSlider.setSignal(value);
      };

      elTr.getOverride = () => {
        return elSlider.getOverride();
      };

      elOverride.addEventListener("click", (event) => {
        let active = elOverride.getAttribute("active");
        active = !active; // toggle
        if (active) {
          elOverride.setAttribute("active", active);
          elSlider.setOverride(elSlider.getSignal());
        } else {
          elOverride.removeAttribute("active");
          elSlider.setOverride(RCSIGNAL_INVALID);
        }
        bluetooth.uploadSignals(getOverrideSignals());
      });

      elSlider.addEventListener("pointerdown", () => {
        elOverride.setAttribute("active", true);
        bluetooth.uploadSignals(getOverrideSignals());
      });

    } else {
      let elTd1 = document.createElement("th");
      elTd1.textContent = signal["description"];
      elTd1.colSpan = 3;
      elTr.appendChild(elTd1);

      elTr.setValue = (value) => { /* do nothing. */ };
      elTr.getOverride = () => { return 0; /* do nothing. */ };
    }

    return elTr;
}


/** Fills the signal table with rows for the different signals */
function fillTable() {
  const elTable = document.getElementById(signalTableId);

  for (let i = 1; i < defSignals.length; i++ ) {
    const elTr = createSignalRow(defSignals[i]);
    elTable.appendChild(elTr);
  }
}

/** Updates the UI (the signals table) with the values
 *  from the data view.
 *
 *  @param dataView a DataView object containing the received data.
 */
function updateSignals(dataView) {
    const signals = new SimpleInputStream(dataView).readSignals(NUM_SIGNALS);
    for (let i = 1; i < defSignals.length; i++ ) {
        const signal = defSignals[i];
        if (!("name" in signal) ||
            !("index" in signal) ||
            (signal["index"] >= signals.length)) {
          continue;
        }

        const index = signal["index"];
        const elRow = document.getElementById("signalRow_" + signal["name"]);
        elRow.setValue(signals[index]);
    }
}

/** Returns a DataView object with the signals array.
 *
 *  @returns dataView a DataView object containing the sensor data.
 */
function getOverrideSignals() {
  const buffer = new ArrayBuffer(NUM_SIGNALS * 2);
  const dataView = new DataView(buffer);

  dataView.setInt16(0, RCSIGNAL_INVALID); // the "none" signal
  for (let i = 1; i < defSignals.length; i++ ) {
    const signal = defSignals[i]
    if ("name" in signal) {

      const elTr = document.getElementById("signalRow_" + signal["name"]);
      const value = elTr.getOverride();
      dataView.setInt16(signal["index"] * 2, value);
    }
  }
  return dataView;
}


/** Called when the tab is entered. */
function startSignals() {
  bluetooth.uploadSignals(getOverrideSignals());
}


export {
  fillTable,
  updateSignals,
  startSignals
}
