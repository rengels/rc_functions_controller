/** Javascript module for widgets, mostly for procs
 *
 *  For the rc functions controller.
 */

import {defProcs} from "./def_procs.js";
import {defSignals} from "./def_signals.js";
import {defSample} from "./def_sample.js";
import {esp_rom_crc16_le, base64Encode, base64Decode} from "./samples.js";
import {SimpleInputStream, SimpleOutputStream} from "./simple_stream.js";
import {
  arrayBufferToAudio,
  audioToBuffer,
  bufferToWav
} from "./wav.js";

// create an id to proc mapping for all procs
let procsIdMapping = {};
let procsNameMapping = {};

let procId = 0;
for (let key in defProcs) {
  if (defProcs.hasOwnProperty(key)) {
    const proc = defProcs[key]
    if ("id" in proc) {
      procsIdMapping[proc["id"]] = proc;
    }
    if ("name" in proc) {
      procsNameMapping[proc["name"]] = proc;
    }
  }
}

// create a sample ID lookup table
let samplesIdMapping = {};
for (let i = 0; i < defSample.length; i++) {
  const sample = defSample[i];
  if ("id" in sample) {
    samplesIdMapping[sample.id] = sample;
  }
}

// create a GPIO enum
let gpioEnum = [];
for (let i = 0; i < 40; i++) {
  gpioEnum.push({
    "name": "GPIO_" + i,
    "id": i,
    "description": "GPIO " + i});
}

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
  name = name.replace(/([a-z])([A-Z])/g, "$1 $2"); // camel case
  name = name.toLowerCase();

  return name;
}


/** Returns a DOM element for signal type selection
 *
 *  @returns the select element
 */
function createSignalSelect() {
  let elSelect = document.createElement("select");
  elSelect.required = true;

  let elDefaultOption = document.createElement("option");
  elDefaultOption.value = null;
  elDefaultOption.disabled = true;
  elDefaultOption.selected = true;
  elDefaultOption.textContent = "Select a signal";
  elSelect.appendChild(elDefaultOption);

  // create options
  let elGroup = null;
  for (let i = 0; i < defSignals.length; i++ ) {
    let signal = defSignals[i]

    if ("name" in signal) {
      let elOption = document.createElement("option");
      elOption.value = signal["name"];

      elOption.textContent = niceName(signal["name"]);
      elOption.title = signal["description"];

      if (elGroup != null) {
        elGroup.appendChild(elOption);
      } else {
        elSelect.appendChild(elOption);
      }
    } else {
      // it's a comment. Let's make a group out of it
      elGroup = document.createElement("optgroup");
      elGroup.label = signal["description"];
      elSelect.appendChild(elGroup);
    }
  }

  return elSelect;
}

/** Returns a DOM element for audio type selection
 *
 *  @returns the select element
 */
function createSampleSelect() {
  let elSelect = document.createElement("select");
  elSelect.required = true;

  {
      let elOption = document.createElement("option");
      elOption.value = "custom";
      elOption.selected = true;
      elOption.textContent = "Custom sound";
      elOption.title = "Upload any audio sample";
      elSelect.appendChild(elOption);
  }

  // create options
  let elGroup = null;
  for (let i = 0; i < defSample.length; i++ ) {
    let audio = defSample[i]

    if ("name" in audio) {
      let elOption = document.createElement("option");
      elOption.value = audio["id"];

      elOption.textContent = niceName(audio["name"]);
      elOption.title = audio["description"];

      if (elGroup != null) {
        elGroup.appendChild(elOption);
      } else {
        elSelect.appendChild(elOption);
      }

    } else {
      // it's a comment. Let's make a group out of it
      elGroup = document.createElement("optgroup");
      elGroup.label = audio["description"];
      elSelect.appendChild(elGroup);
    }
  }

  return elSelect;
}


/** Returns a DOM element for an select with an enumData struct as an input.
 *
 *  @param enumData A structure like the proc structure:
 *
 *  <pre>
 *  [
 *    {"name": "hi", "id": "some id", "description": "some descr"},
 *    {"description": "some group"}
 *  ]
 *  </pre>
 *  @returns the select element
 */
function createCustomSelect(enumData) {
  let elSelect = document.createElement("select");
  elSelect.required = true;

  // create options
  let elGroup = null;
  for (let i = 0; i < enumData.length; i++ ) {
    let enumValue = enumData[i]

    if ("name" in enumValue) {
      let elOption = document.createElement("option");
      elOption.value = enumValue["id"];

      elOption.textContent = enumValue["name"];
      elOption.title = enumValue["description"];

      if (elGroup != null) {
        elGroup.appendChild(elOption);
      } else {
        elSelect.appendChild(elOption);
      }

    } else {
      // it's a comment. Let's make a group out of it
      elGroup = document.createElement("optgroup");
      elGroup.label = enumValue["description"];
      elSelect.appendChild(elGroup);
    }
  }

  return elSelect;
}


/** Returns a DOM element for proc type selection
 *
 *  @returns the select element
 */
function createProcSelect() {
  let elSelect = document.createElement("select");
  elSelect.required = true;

  let elDefaultOption = document.createElement("option");
  elDefaultOption.value = null;
  elDefaultOption.disabled = true;
  elDefaultOption.selected = true;
  elDefaultOption.textContent = "Select a Proc type";
  elSelect.appendChild(elDefaultOption);

  // create options
  let elGroup = null;
  for (let i = 0; i < defProcs.length; i++ ) {
    let proc = defProcs[i]

    if ("name" in proc) {
      let elOption = document.createElement("option");
      elOption.value = proc["name"];

      elOption.textContent = niceName(proc["name"]);
      elOption.title = proc["description"];

      if (elGroup != null) {
        elGroup.appendChild(elOption);
      } else {
        elSelect.appendChild(elOption);
      }
    } else {
      // it's a comment. Let's make a group out of it
      elGroup = document.createElement("optgroup");
      elGroup.label = proc["description"];
      elSelect.appendChild(elGroup);
    }
  }

  return elSelect;
}

/** Returns a correct configured input element for the gear ratio. */
function createGearInput() {
  let elInput = document.createElement('input');
  elInput.type = 'number';
  elInput.size = 3;
  elInput.step = 0.1;
  elInput.min = -12.7;
  elInput.max = 12.7;
  elInput.valueAsNumber = 0;

  return elInput;
}

/** Returns a span element for editing gears.
*
*  A unsorted list with 20 gears looks ugly.
*  This widget will dynamically add new edit fields and
*  sort the gear ratios.
*
*  @param id An unique number/string identifying the proc config element.
*  @param type A dict with the configuration for the value from procs_config.json
*  @param values An array of values from the configuration.
*
*  @returns the tr element
*/
function createGearCollectionWidget() {
  let elSpan = document.createElement("span");
  elSpan.title = "Gear ratios";

  let elInput = createGearInput();
  elSpan.appendChild(elInput);

  let elDelButton = document.createElement("span");
  elDelButton.className = "button";
  elDelButton.textContent = "-";
  elDelButton.title = "Remove the last gear ratio.";
  elSpan.appendChild(elDelButton);

  let elAddButton = document.createElement("span");
  elAddButton.className = "button";
  elAddButton.textContent = "+";
  elAddButton.title = "Add another gear ratio.";
  elSpan.appendChild(elAddButton);

  elDelButton.addEventListener("click", (event) => {
    if (elSpan.childElementCount > 2) {
      elSpan.removeChild(elDelButton.previousSibling);
    }
    if (elSpan.childElementCount <= 2) {
      elDelButton.toggleAttribute("disabled", true);
    }
  });

  elAddButton.addEventListener("click", (event) => {
    let elInput = createGearInput();
    elSpan.insertBefore(elInput, elDelButton);
    elDelButton.toggleAttribute("disabled", false);
  });

  elSpan.getConfig = function() {
    let values = [];
    for (let i = 0; i < elSpan.childElementCount - 2; i++) {
      if (elSpan.children[i].valueAsNumber) {
        values.push(elSpan.children[i].valueAsNumber);
      }
    }
    return values;
  }
  elSpan.setConfig = function(values) {
    if (!Array.isArray(values)) {
      console.log("Values sould be an array in gearCollectionWidget.");
      values = [values];
    }

    // add enough inputs for all the values.
    while (elSpan.childElementCount < values.length + 2) {
      let elInput = createGearInput();
      elSpan.insertBefore(elInput, elDelButton);
    }

    // set the values
    for (let i = 0; i < elSpan.childElementCount - 2; i++) {
      elSpan.children[i].valueAsNumber = values?.[i] ?? 0.0;
    }
  }

  return elSpan;
}


/** Creates an Idle widget to edit rcEngine::Idle values */
function createIdleWidget() {
  const types = [
      {
          "name": "rpmIdleStart",
          "type": "uint16_t",
          "description": "The RPM after cranking has finished."
      },
      {
          "name": "rpmIdleRunning",
          "type": "uint16_t",
          "description": "The RPM after timeStart has passed."
      },
      {
          "name": "loadStart",
          "type": "RcSignal",
          "unit": "kW",
          "description": "The additional engine load after cranking in kW."
      },
      {
          "name": "timeStart",
          "type": "uint32_t",
          "unit": "ms",
          "description": "The time between rpmIdleStart and rpmIdleRunning in ms."
      },
      {
          "name": "throttleStep",
          "type": "uint16_t",
          "unit": "sig",
          "description": "The amount that throttle is changed to regulate the RPM in ms."
      }
  ];

  let elSpan = document.createElement("table");

  let elements = {};
  for (var i = 0; i < types.length; i++) {
    const element = createValueRow("", types[i], []);
    elements[types[i]["name"]] = element;
    elSpan.appendChild(element);
  }

  elSpan.getConfig = function() {
    let result = {};
    for (var i = 0; i < types.length; i++) {
      const name = types[i]["name"];
      result[name] = elements[name].getConfig()[0];
    }
    return result;
  }
  elSpan.setConfig = function(val) {
    for (var i = 0; i < types.length; i++) {
      const name = types[i]["name"];
      elements[name].setConfig([val?.[name] ?? 0]);
    }
  }

  return elSpan;
}


/** Returns a span element displaying the expo curve. */
function createExpoWidget() {
  let elSpan = document.createElement("span");

  let elCanvas = document.createElement("canvas");
  elCanvas.className = "expo";
  elCanvas.width = 50;
  elCanvas.height = 50;
  elSpan.appendChild(elCanvas);

  let elSlider = document.createElement("input");
  elSlider.type = "range";
  elSlider.min = 0;
  elSlider.max = 150;
  elSlider.step = 1;
  elSlider.value = 100;
  elSpan.appendChild(elSlider);

  const updateCanvas = function update() {
    const ctx = elCanvas.getContext("2d");

    const width = elCanvas.width;
    const height = elCanvas.height;

    const b = elSlider.valueAsNumber / 100.0;

    ctx.fillStyle = "white";
    ctx.fillRect(0, 0, width, height);

    // some lables
    ctx.save();
    ctx.fillStyle = "grey";
    ctx.textAlign = "center";

    ctx.fillText(b.toString(), width / 4, height - 10);
    ctx.fillText("input", width / 2, 10);
    ctx.rotate(Math.PI/2);
    ctx.fillText("output", height / 2, - width + 10);
    ctx.restore();

    ctx.lineWidth = 1;
    ctx.strokeStyle = "grey";

    // Draw axis
    ctx.beginPath();
    ctx.moveTo(0, height / 2);
    ctx.lineTo(width, height / 2);
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(width / 2, 0);
    ctx.lineTo(width / 2, height);
    ctx.stroke();

    // draw expo
    ctx.strokeStyle = "black";
    ctx.beginPath();
    ctx.moveTo(0, height);
    const steps = 5;
    for (let i = -steps; i <= steps; i++) {
      const x = i / steps;
      const y = b * x + (1 - b) * x * x * x;
      ctx.lineTo(
          (0.5 + 0.5 * x) * width, 
          (0.5 - 0.5 * y) * height);
    }
    ctx.stroke();
  }

  // the input event listener updates the canvas
  elSlider.addEventListener("input", (event) => {
    updateCanvas();
  });

  elSpan.setConfig = (value) => {
    elSlider.value = value * 100.0;
    updateCanvas();
  }
  elSpan.getConfig = function() {
    return elSlider.valueAsNumber / 100.0;
  }

  return elSpan;
}


/** Returns a DOM element for a volume slider
 *
 *  @returns the element.
 */
function createVolumeSlider() {
  let elSpan = document.createElement("span");

  let elSlider = document.createElement("input");
  elSlider.type = "range";
  elSlider.min = 0;
  elSlider.max = 200;
  elSlider.step = 1;
  elSlider.value = 100;
  elSlider.setAttribute("list", "listVolume");
  elSpan.appendChild(elSlider);

  let elBubble = document.createElement("span");
  elBubble.className = "sliderBubble"
  elBubble.textContent = "100%";
  elSlider["bubble"] = elBubble;
  elSpan.appendChild(elBubble);

  elSlider.addEventListener("input", (event) => {
    elBubble.textContent = elSlider.value + "%";
  });
  elSpan.setConfig = (value) => {
    elSlider.value = value;
    elBubble.textContent = elSlider.value + "%";
  }
  elSpan.getConfig = function() {
    return elSlider.valueAsNumber;
  }

  return elSpan;
}

/** Returns a span for selecting static or dynamic samples.
 *
 *  The project has a list of audio samples that are programmed in,
 *  but the user can also upload it's own samples (dynamically).
 *
 */
function createAudioSampleWidget() {
  let elSpan = document.createElement("span");

  let elSelect = createSampleSelect();
  elSpan.appendChild(elSelect);

  let elLoadInput = document.createElement("input");
  elLoadInput.textContent = "Load sample..";
  elLoadInput.type = "file";
  elLoadInput.accept = ".wav, .mp3";
  elLoadInput.style.display = "none";  // we would rather have our own button
  elSpan.appendChild(elLoadInput);
  elSpan.appendChild(document.createTextNode("\u00A0"));

  let elLoadButton = document.createElement("span");
  elLoadButton.className = "button";
  elLoadButton.textContent = "Load\u00A0sample..";
  elLoadButton.toggleAttribute("disabled", true);
  elLoadButton.style.display = "none";
  elSpan.appendChild(elLoadButton);
  elSpan.appendChild(document.createTextNode("\u00A0"));

  let elFileName = document.createElement("span");
  elFileName.textContent = ".";
  elSpan.appendChild(elFileName);
  elSpan.appendChild(document.createTextNode("\u00A0"));

  let elPlayButton = document.createElement("span");
  elPlayButton.className = "button";
  elPlayButton.textContent = "play"; // "\u23F5";
  elPlayButton.toggleAttribute("disabled", true);
  elPlayButton.style.display = "none";
  elSpan.appendChild(elPlayButton);

  let elAudio = document.createElement("audio");
  elAudio.controls = true;
  elAudio.style.display = "none";  // we would rather have our own button

  let elSource = document.createElement("source");
  elSource.type = "audio/wav";
  // elSource.src = "../src/samples/" + audioIdMapping[value]["filename"];

  elAudio.appendChild(elSource);
  elSpan.appendChild(elAudio);

  // -- events
  elLoadButton.addEventListener("click", (event) => {
    elLoadInput.click();  // click on the hidden file input
  });
  elPlayButton.addEventListener("click", (event) => {
    elAudio.play();
  });

  elLoadInput.addEventListener("change", (event) => {
    if (elLoadInput.files.length > 0) {
      // read and convert the file to wav
      let reader = new FileReader();
      reader.addEventListener("load", async function (event) {
        elFileName.textContent = elLoadInput.files[0].name;

        const audio = await arrayBufferToAudio(reader.result);
        const buffer = audioToBuffer(audio, 22050);
        const wav = bufferToWav(buffer, 22050); // ArrayBuffer

        const blob = new Blob([wav], {type:'audio/wav'});
        const URLObject = window.webkitURL || window.URL;
        const url = URLObject.createObjectURL(blob);
        elSource.src = url;
        elAudio.src = url;
        elAudio.load();

        elPlayButton.style.display = "inline-block";
        elPlayButton.toggleAttribute("disabled", false);

        // add the custom sample to the span, so that we
        // can get it later on.
        elSpan["customSample"] = {
          "id": null,
          "filename": elLoadInput.files[0].name,
          "data": wav,
          "data2": base64Encode(wav),
          "length": wav.byteLength,
          "crc": esp_rom_crc16_le(65535, new Uint8Array(wav))
        };
      });
      reader.readAsArrayBuffer(elLoadInput.files[0]);	
    }
  });

  elSelect.addEventListener("change", (event) => {
    if (elSelect.value == "custom") {
      elLoadButton.toggleAttribute("disabled", false);
      elLoadButton.style.display = "inline-block";
      elPlayButton.style.display = "inline-block";
    } else {
      elLoadButton.toggleAttribute("disabled", true);
      elLoadButton.style.display = "none";
      elPlayButton.style.display = "none";
      elFileName.textContent = ".";
    }

    // emit the changed event for the span
    elSpan.value = elSelect.value;
    let newEvent = new Event("change");
    elSpan.dispatchEvent(newEvent);
  });

  elSpan.getConfig = () => {
    if (elSelect.value != "custom") {
      return elSelect.value;
    } else {
      return elSpan["customSample"];
    }
  }
  elSpan.setConfig = (value) => {
    // static sample
    if (value in samplesIdMapping) {
      elSelect.value = value;
      elLoadButton.style.display = "none";
      elLoadButton.toggleAttribute("disabled", true);
      elPlayButton.style.display = "none";
      elPlayButton.toggleAttribute("disabled", true);

    // dynamic sample
    } else {
      if (typeof value == "string") {
        // check if we have the same ID as before.
        // in that case we keep the old data
        if (elSpan?.["customSample"]?.["id"] == value) {
          value = elSpan["customSample"]["id"];
        } else {
          value = {"id": value};
        }
      }

      elSelect.value = "custom";  // dynamic sample
      elFileName.textContent = value["filename"];

      // convert base64 back
      if ("data2" in value) {
        const wav = base64Decode(value["data2"]);

        const blob = new Blob([wav], {type:'audio/wav'});
        const URLObject = window.webkitURL || window.URL;
        const url = URLObject.createObjectURL(blob);
        elSource.src = url;
        elAudio.src = url;
        elAudio.load();

        value["data"] = wav;

        elPlayButton.style.display = "inline-block";
        elPlayButton.toggleAttribute("disabled", false);
      }

      elSpan["customSample"] = value

      elLoadButton.style.display = "inline-block";
      elLoadButton.toggleAttribute("disabled", false);
    }
  }

  return elSpan;
}

/** Returns a TR element for the configuration of the type.
*
*  @param id An unique number/string identifying the proc config element.
*  @param type the configuration for the type (with _name_, _num_ and _description_)
*  @param values a list with the config values
*  @returns the li element
*/
function createTypeRow(id, type, values) {

  let elTr = document.createElement('tr');
  elTr.title = type["description"];

  let elTh = document.createElement('th');
  elTh.textContent = niceName(type["name"]);
  elTr.appendChild(elTh);

  let elTd = document.createElement('td');

  let elTypes = [];
  for (let j = 0; j < (type["num"] ?? 1); j++) {
    let elSelect = createSignalSelect();
    elSelect.id = "procType" + id.toString() + "_" + type["name"] + "_" + j;
    elSelect.value = values?.[j] ?? "ST_NONE";
    elTypes.push(elSelect);
    elTd.appendChild(elSelect);
  }
  elTr.appendChild(elTd);

  // -- add the functions to re-create the config
  elTr.getConfig = function() {
    return elTypes.map((el) => el.value);
  }
  return elTr;
}

/** Returns a TR element for the configuration of the proc values.
*
*  @param id An unique number/string identifying the proc config element.
*  @param type A dict with the configuration for the value from procs_config.json
*  @param values An array of values from the configuration.
*
*  @returns the tr element
*/
function createValueRow(id, type, values) {

  let elTr = document.createElement('tr');
  elTr.title = type["description"];

  let elTh = document.createElement('th');
  elTh.textContent = niceName(type["name"]);
  elTr.appendChild(elTh);

  let elTd = document.createElement('td');

  if (!Array.isArray(values)) {
    console.log("Values should be an array in " + type["name"]);
  }

  let elValues = [];
  for (let j = 0; j < (type?.["num"] ?? 1); j++) {
    let value = values?.[j];

    let elValue = null;
    if (type["type"] == "bool") {
      let elInput = document.createElement("input");
      elInput.id = "procValue" + id.toString() + "_" + type["name"];
      elInput.type = "checkbox";
      elInput.checked = value ?? false;
      elInput.getConfig = function() {
        return elInput.checked;
      }
      elInput.setConfig = function(val) {
        elInput.checked = val;
      }
      elValue = elInput;

    } else if (type["type"] == "uint8_t") {
      let elInput = document.createElement('input');
      elInput.id = "procValue" + id.toString() + "_" + type["name"];
      elInput.type = 'number';
      elInput.placeholder = type["description"];
      elInput.step = 1;
      elInput.min = 0;
      elInput.max = 255;
      elInput.style = 'width: 5em';
      elInput.value = value ?? 0;
      elInput.getConfig = function() {
        return elInput.valueAsNumber;
      }
      elInput.setConfig = function(val) {
        elInput.valueAsNumber = val;
      }
      elValue = elInput;

    } else if ((type["type"] == "uint16_t") ||
    (type["type"] == "uint32_t") ||
    (type["type"] == "TimeMs")) {
      let elInput = document.createElement('input');
      elInput.id = "procValue" + id.toString() + "_" + type["name"];
      elInput.type = 'number';
      elInput.placeholder = type["description"];
      elInput.step = 1;
      elInput.min = 0;
      elInput.style = 'width: 5em';
      elInput.value = value ?? 0;
      elInput.getConfig = function() {
        return elInput.valueAsNumber;
      }
      elInput.setConfig = function(val) {
        elInput.value = val;
      }
      elValue = elInput;

    } else if (type["type"] == "rcEngine::GearCollection") {
      let elInput = createGearCollectionWidget();
      elInput.setConfig(value ?? [-3.0, 3.0]);

      elValue = elInput;

    } else if (type["type"] == "rcEngine::Idle") {
      let elInput = createIdleWidget();
      elInput.setConfig(value);

      elValue = elInput;

    } else if (type["type"] == "Volume") {
      let elSlider = createVolumeSlider();
      elSlider.setConfig(value ?? 100);

      elValue = elSlider;

    } else if (type["type"] == "float" &&
        type["name"] == "b") {
      let elExpo = createExpoWidget();
      elExpo.setConfig(value ?? 0.5);

      elValue = elExpo;
    } else if (type["type"] == "float") {
      let elInput = document.createElement('input');
      elInput.id = "procValue" + id.toString() + "_" + type["name"];
      elInput.id = "procValue" + id.toString() + "_" + type["name"];
      elInput.placeholder = type["description"];
      elInput.type = 'number';
      elInput.style = 'width: 5em';
      elInput.value = value ?? 0.0;
      elInput.getConfig = function() {
        return elInput.value;
      }
      elInput.setConfig = function(val) {
        elInput.value = val;
      }
      elValue = elInput;

    } else if (type["type"] == "RcSignal") {
      let elInput = document.createElement('input');
      elInput.id = "procValue" + id.toString() + "_" + type["name"];
      elInput.placeholder = type["description"];
      elInput.type = 'number';
      elInput.step = 1;
      elInput.min = -1200;
      elInput.max = 1200;
      elInput.style = 'width: 5em';
      elInput.value = value ?? 0;
      elInput.getConfig = function() {
        return elInput.value;
      }
      elInput.setConfig = function(val) {
        elInput.value = val;
      }
      elValue = elInput;

    } else if (type["type"] == "SignalType") {
      let elSelect = createSignalSelect();
      elSelect.value = value ?? "ST_NONE";
      elSelect.getConfig = function() {
        return elSelect.value;
      }
      elSelect.setConfig = function(val) {
        elSelect.value = val;
      }
      elValue = elSelect;

    } else if (type["type"] == "SampleData") {
      let elSpan = createAudioSampleWidget();
      elSpan.id = "procValue" + id.toString() + "_" + type["name"];
      elSpan.setConfig(value ?? "Osi");
      elValue = elSpan;

    } else if (type["type"] == "gpio_num_t") {
      let elSelect = createCustomSelect(gpioEnum);
      elSelect.id = "procValue" + id.toString() + "_" + type["name"];
      elSelect.value = value ?? 39;
      elSelect.getConfig = function() {
        return elSelect.value;
      }
      elSelect.setConfig = function(val) {
        elSelect.value = val;
      }
      elValue = elSelect;

    } else if ((type["type"] == "EngineSimple::EngineType") ||
      (type["type"] == "InputDemo::DemoType") ||
      (type["type"] == "AudioNoise::NoiseType") ||
      (type["type"] == "ProcCombine::Function")  ||
      (type["type"] == "OutputEsc::FreqType")) {

      let elSelect = createCustomSelect(type["enum"]);
      elSelect.id = "procValue" + id.toString() + "_" + type["name"];
      elSelect.value = value ?? 0;
      elSelect.getConfig = function() {
        return elSelect.value;
      }
      elSelect.setConfig = function(val) {
        elSelect.value = val;
      }
      elValue = elSelect;
    }

    if (elValue) {
      elValues.push(elValue);
      elTd.appendChild(elValue);
    }

    if ("unit" in type) {
      elTd.appendChild(document.createTextNode(" " + type["unit"]));
    }
  }
  elTr.appendChild(elTd);

  // -- add the functions to re-create the config
  elTr.getConfig = function() {
    return elValues.map((el) => el.getConfig());
  }
  elTr.setConfig = function(val) {
    elValues.map((el) => el.setConfig(val));
  }
  return elTr;
}

export {
  createSignalSelect,
  createSampleSelect,
  createCustomSelect,
  createProcSelect,
  createAudioSampleWidget,
  createTypeRow,
  createValueRow,
}

