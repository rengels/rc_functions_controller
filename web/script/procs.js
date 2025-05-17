/** Javascript module for handling Procs
 *
 *  For the rc functions controller.
 */

import {defProcs} from './def_procs.js';
import {defSample} from './def_sample.js';
import {uploadMissingSamples, sampleSetStatusCallback, resetSamples} from './samples.js';
import {SimpleInputStream, SimpleOutputStream} from './simple_stream.js';

import * as bluetooth from "./bluetooth.js";

import {
  createSignalSelect,
  createSampleSelect,
  createCustomSelect,
  createProcSelect,
  createTypeRow,
  createValueRow,
  } from './widgets.js';


// create an id to proc mapping for all procs
let procsIdMapping = {};
let procsNameMapping = {};

let colorColor = 0;
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

    // add some color tags that is later on used in the element styling
    if (!("color" in proc)) {
      proc["color"] = "hsl(" + colorColor + " 80% 30%)";
      proc["colorBg"] = "hsl(" + colorColor + " 50% 10%)";
      colorColor += 7;
    }
  }
}

// create an id to audio mapping for all static audios
let audioIdMapping = {};
for (let i = 0; i < defSample.length; i++) {
  const audio = defSample[i];
  if ("id" in audio) {
    audioIdMapping[audio["id"]] = audio;
  }
}

/** A callback function for status update.
 *
 *  Will get as parameters a string with a status message
 *  and some option dict.
 */
let procStatusCallback = null;

const configTableId = "configTable";

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


/** Creates a group menu button including context menu.
 *
 *  @param elGroup The DOM element of the group (the list item).
 *  @returns An DOM element for the button.
 */
function createGroupMenuButton(elGroup) {

  // options menu button
  let elMenuButton = document.createElement('span');
  elMenuButton.className = "options_menu";

  let elImg = document.createElement('img');
  elImg.src="./res/burger.svg";
  elMenuButton.appendChild(elImg)

  elMenuButton.addEventListener("click", (event) => {
    // TODO
  });

  // options menu hover div
  let elDropdown = document.createElement("ul");
  elDropdown.className = "options_window";

  let elAD0 = document.createElement('li');
  elAD0.textContent = "Change group type"
  elAD0.title = "Change the type of the group (e.g. from Input to Vehicle)."
  elAD0.addEventListener("click", (event) => {
    elGroup.changeType();
  });
  elDropdown.appendChild(elAD0)

  let elAD1 = document.createElement('li');
  elAD1.textContent = "Save Group.."
  elAD1.title = "Save the group and it's content in a .json file."
  // TODO: check if group is empty and enable save button
  elAD1.addEventListener("click", (event) => {
    saveGroupConfig(elGroup);
  });
  elDropdown.appendChild(elAD1)

  let elAD2 = document.createElement('li');
  elAD2.textContent = "Replace Group from file.."
  elAD2.title = "Replace the group including it's content with elements loaded from a .json file."
  elAD2.addEventListener("click", (event) => {
    loadGroupConfig(elGroup);
  });
  elDropdown.appendChild(elAD2)

  let elAD11 = document.createElement('li');
  elAD11.textContent = "Move up"
  elAD11.title = "Move the group up."
  elAD11.addEventListener("click", (event) => {
    moveUp(elGroup);
  });
  elDropdown.appendChild(elAD11)

  let elAD22 = document.createElement('li');
  elAD22.textContent = "Move down"
  elAD22.title = "Move the group down."
  elAD22.addEventListener("click", (event) => {
    moveDown(elGroup);
  });
  elDropdown.appendChild(elAD22)

  let elAD3 = document.createElement('li');
  elAD3.textContent = "Remove Group"
  elAD3.title = "Remove group including the content of the group."
  elAD3.addEventListener("click", (event) => {
    elGroup.parentElement.removeChild(elGroup);  // remove myself
  });
  elDropdown.appendChild(elAD3)

  elMenuButton.appendChild(elDropdown)

  elMenuButton.addEventListener("click", (event) => {
    // toggle visibility of options popup window
    if (elDropdown.style.display == "block") {
      elDropdown.style.display = "none";
    } else {
      elDropdown.style.display = "block";
    }
  });

  return elMenuButton;
}

/** Provides a nice group specific color for a group element.
 *
 *  @param elLi The group li element.
 */
function tintGroupElement(elLi) {

  // we get nice border colors from procs
  const groupTypeMapping = {
    0: procsNameMapping["INPUT_DEMO"],  // input
    1: procsNameMapping["PROC_ENGINE_REVERSE"],  // vehicle
    2: procsNameMapping["OUTPUT_AUDIO"],  // output
    3: procsNameMapping["AUDIO_LOOP"],  // audio
    4: procsNameMapping["PROC_COMBINE"],  // logic
    5: procsNameMapping["PROC_CRANKING"],  // lights
    6: procsNameMapping["PROC_ENGINE_SIMPLE"]  // motor
  }

  if (groupTypeMapping?.[elLi.configType]?.["color"]) {
    elLi.style.borderColor = groupTypeMapping?.[elLi.configType]["color"];
    elLi.style.backgroundColor = groupTypeMapping?.[elLi.configType]["colorBg"];
  }
}

/** Returns a DOM element for the group proc.
*
*  The DOM element has a getConfig() function that returns
*  the current configuration.
*  It also has getConfigChildren() returning all sub-configs.
*
*  @param id An unique number/string identifying the proc config element.
*  @param proc The proc config structure
*  @returns the li element
*/
function createGroupElement(id, config) {
  const proc = procsNameMapping[config["name"]];

  let elLi = document.createElement('li');
  elLi.className = "procGroup";
  elLi.configType = config?.["values"]?.["type"]?.[0] ?? 0;

  elLi.appendChild(createGroupMenuButton(elLi));

  // details
  let elDetails = document.createElement('details');

  // summary
  let elSummary = document.createElement('summary');
  elDetails.appendChild(elSummary);

  // title
  let elH = document.createTextNode(
      proc["type names"]?.[elLi.configType] + " (group)");
  elSummary.appendChild(elH);
  elLi.appendChild(elDetails);


  // a container for the children
  const elUl = document.createElement('ul');
  elDetails.appendChild(elUl);


  // a function to change the group type
  elLi.changeType = function() {
      elLi.configType = (elLi.configType + 1) % proc["type names"].length;
      elH.nodeValue =
        proc["type names"]?.[elLi.configType] + " (group)";
      tintGroupElement(elLi);
  }

  // -- add the functions to re-create the config
  elLi.getConfig = function() {
    return {
      "name": "PROC_GROUP",
      "types": {},
      "values": {
        "type": [elLi.configType],
        "numChilds": [elUl.children.length]
      }
    };
  };

  elLi.configUl = elUl;  // so that we get the ul later on
  elLi.open = function() {  // so that we know if it's unfolded/open
    return elDetails.open;
  };

  tintGroupElement(elLi);
  return elLi;
}

/** Creates a proc menu button including context menu.
 *
 *  @param elProc The DOM element of the proc (the list item).
 *  @returns An DOM element for the button.
 */
function createProcMenuButton(elProc) {

  // options menu button
  let elMenuButton = document.createElement('span');
  elMenuButton.className = "options_menu";

  let elImg = document.createElement('img');
  elImg.src="./res/burger.svg";
  elMenuButton.appendChild(elImg)

  // options menu hover div
  let elDropdown = document.createElement("ul");
  elDropdown.className = "options_window";

  let elAD1 = document.createElement('li');
  elAD1.textContent = "Move up"
  elAD1.title = "Move the proc up."
  elAD1.addEventListener("click", (event) => {
    moveUp(elProc);
  });
  elDropdown.appendChild(elAD1)

  let elAD2 = document.createElement('li');
  elAD2.textContent = "Move down"
  elAD2.title = "Move the proc down."
  elAD2.addEventListener("click", (event) => {
    moveDown(elProc);
  });
  elDropdown.appendChild(elAD2)

  let elAD3 = document.createElement('li');
  elAD3.textContent = "Remove"
  elAD3.title = "Remove the proc element."
  elAD3.addEventListener("click", (event) => {
    elProc.parentElement.removeChild(elProc);  // remove myself
  });
  elDropdown.appendChild(elAD3)

  elMenuButton.appendChild(elDropdown)

  elMenuButton.addEventListener("click", (event) => {
    // toggle visibility of options popup window
    if (elDropdown.style.display == "block") {
      elDropdown.style.display = "none";
    } else {
      elDropdown.style.display = "block";
    }
  });

  return elMenuButton;
}

/** Adds special event handling for the Audio Loop element.
 *
 *  This function adds a special event handler for changing the audio type.
 *  Else it wouldn't be tricky to set the start end end loop values
 *  for predefined sounds.
 *
 *  @param elValueRows A list of all the value row elements.
 */
function modifyAudioLoopElement(elValueRows) {
  const proc = procsNameMapping["AUDIO_LOOP"];
  const values = proc["values"];

  const indexSample = values.findIndex((x) => x["type"] == "SampleData");
  const indexBegin  = values.findIndex((x) => x["name"] == "loopBegin");
  const indexEnd    = values.findIndex((x) => x["name"] == "loopEnd");

  if (indexSample >= 0 && indexBegin >= 0 && indexEnd >= 0) {

    // new event listener for the audio sample widget (createAudioSampleWidget())
    elValueRows[indexSample].addEventListener("change", (event) => {
      let newSample = elValueRows[indexSample].getConfig();
      if (newSample in audioIdMapping) {
        const sample = audioIdMapping[newSample];
        elValueRows[indexBegin].setConfig([sample["loopStart"]]);
        elValueRows[indexEnd].setConfig([sample["loopEnd"]]);
      }
    });
  }
}

/** Returns a DOM element for the proc.
 *
 *  The created DOM element has a getConfig() function attached.
 *
 *  @param id An unique number/string identifying the proc config element.
 *  @param proc The proc config structure
 *
 *  @returns the li element
 */
function createConfigElement(id, config) {
  const proc = procsNameMapping[config["name"]];

  let elLi = document.createElement('li');
  elLi.className = "proc";
  elLi.style.borderColor = proc["color"];
  elLi.style.backgroundColor = proc["colorBg"];

  // title
  let elH1 = document.createElement('h1');
  elH1.textContent = niceName(proc["name"]);
  elLi.appendChild(elH1);
  elLi.appendChild(createProcMenuButton(elLi));

  let elDescr = document.createElement('small');
  elDescr.textContent = proc["description"];
  elLi.appendChild(elDescr);

  // table (for the types and values)
  let elTable = document.createElement('table');

  // types
  let elTypeRows = [];
  if ("types" in proc) {
    for (let i = 0; i < proc["types"].length; i++) {
      const type = proc["types"][i];
      let values = [];
      try {
        values = config["types"][type["name"]];
      } catch (e) {
        console.log("Error", e.message); 
      }

      let typeRow = createTypeRow(id, type, values);
      elTypeRows.push(typeRow);
      elTable.appendChild(typeRow);
    }
  }

  // values
  let elValueRows = [];
  if ("values" in proc) {
    for (let i = 0; i < proc["values"].length; i++) {
      const value = proc["values"][i];
      // the *values* of the *value* (the config of the definition)
      let values = config?.["values"]?.[value["name"]] ?? [];

      let valueRow = createValueRow(id, value, values);
      elValueRows.push(valueRow);
      elTable.appendChild(valueRow);
    }
  }

  elLi.appendChild(elTable);

  // -- add special handling for special config types
  if (config["name"] == "AUDIO_LOOP") {
    modifyAudioLoopElement(elValueRows);
  }

  // -- add the functions to re-create the config
  elLi.getConfig = function() {
    let newConfig = {
      "name": config["name"],
      "types": {},
      "values": {}
    };
    for (let i = 0; i < elTypeRows.length; i++) {
      newConfig["types"][proc["types"][i]["name"]] = elTypeRows[i].getConfig();
    }
    for (let i = 0; i < elValueRows.length; i++) {
      newConfig["values"][proc["values"][i]["name"]] = elValueRows[i].getConfig();
    }
    return newConfig;
  };
  elLi.configUl = null;

  return elLi;
}

/** Returns a DOM element for a new proc (where you can select the type)
*
*  This element will "morph" into an actual proc element
*  once the user selects the type.
*
*  @returns the li element
*/
function createNewElement() {

  let elLi = document.createElement('li');
  elLi.className = "proc";

  let elSelect = createProcSelect();
  elLi.appendChild(elSelect);

  elLi.appendChild(createProcMenuButton(elLi));

  // when a proc type is selected we change ourself into the "real" one.
  elSelect.addEventListener("change", (event) => {
    const procType = elSelect.value;
    const proc = procsNameMapping[procType];

    let newConfig = {
      "name": procType,
      "types": proc?.["defaultTypes"] ?? {},
      "values": proc?.["defaultValues"] ?? {}
    };

    let elNew;
    if (newConfig["name"] == "PROC_GROUP") {
      elNew = createGroupElement(0, newConfig);

    } else {
      elNew = createConfigElement(0, newConfig);

    }

    elLi.replaceWith(elNew);
  });

  // -- add dummy functions to re-create the config like with a "normal" proc
  elLi.getConfig = function() {
    return null;
  };
  elLi.configUl = null;

  return elLi;
}


/** Fills the DOM element with elements created out of the provided config.
 *
 *  To create a tree of groups we have a stack where we push
 *  the groups on and off.
 *
 *  @param elUl The DOM element that should be filled.
 */
function fillConfigElement(elUl, configProcs) {

  let elUlStack = [elUl];
  let ulStackCount = [9999];  // the number of elements until the group is full

  for (let i = 0; i < configProcs.length; i++ ) {
    let config = configProcs[i];
    if ("name" in config && config["name"] in procsNameMapping) {

      // create a new item
      let elLi;
      if (config["name"] == "PROC_GROUP") {
        elLi = createGroupElement(i, config);

      } else {
        elLi = createConfigElement(i, config);

      }
      elLi.id = "proc" + i.toString(); // give them some IDs

      // add new item
      elUlStack[0].appendChild(elLi);
      ulStackCount[0] -= 1;

      // for a group, open a new stack frame
      if (config["name"] == "PROC_GROUP") {
        const elUl2 = elLi.configUl
        elUlStack.unshift(elUl2);
        ulStackCount.unshift(config?.["values"]?.["numChilds"]?.[0] ?? 1);
      }

      // remove old stack elements
      while (ulStackCount.length > 0 && ulStackCount[0] < 1) {
        elUlStack.shift();
        ulStackCount.shift();
      }
    }
  }
}

/** Fills the config table with rows for the different config.
 *
 *  It's actually a list and not a table, but that doesn't really
 *  matter.
 *
 *  To create a tree of groups we have a stack where we push
 *  the groups on and off.
 */
function fillConfigTable(configProcs) {
  const elUl = document.getElementById(configTableId);
  fillConfigElement(elUl, configProcs);
}


/** Returns the list or config elements from a starting elements */
function recursiveGetConfigElements(elUl) {
  let elList = [];
  for (let i = 0; i < elUl.children.length; i++) {
    elList.push(elUl.children[i]);
    const elSub = elUl.children[i].configUl;
    if (elSub) {
      elList.push(...recursiveGetConfigElements(elSub));
    }
  }
  return elList;
}


/** Recover the configuration from the table.
 *
 *  This will call the getConfig() and getConfigChildren() functions
 *  to re-create a config structure.
 */
function getConfigFromTable() {
  const elUl = document.getElementById(configTableId);

  // -- first get a list of DOM elements
  let elList = recursiveGetConfigElements(elUl);

  // -- get the config from the DOM elements
  let configList = [];
  for (let i = 0; i < elList.length; i++) {
    const config = elList[i].getConfig();
    if (config != null) {
      configList.push(config);
    }
  }

  return configList;
}


/** Updates the UI (the configs table) with the values
 *  from a config structure
 *
 *  @param config The new configuration
 */
function updateConfigFromConfig(config) {
  // create the table from scratch
  const elTable = document.getElementById(configTableId);
  elTable.innerHTML = "";

  fillConfigTable(config);
}


// --- serializing and deserializing of config (Proc)

/** Updates the UI (the configs table) with the values
 *  from the data view.
 *
 *  @param dataView a DataView object containing the received data.
 */
function updateConfigFromData(dataView) {

  let stream = new SimpleInputStream(dataView);
  updateConfigFromConfig(stream.readProcs());
}

/** Returns the encoded configuration ready to send
 *
 *  @param configStruct A list of proc configurations.
 *  @returns dataView a DataView object containing the received data.
 */
function encodeConfig(configStruct) {

  let buffer = new ArrayBuffer(400);
  const dataView = new DataView(buffer);

  let stream = new SimpleOutputStream(dataView);
  stream.writeProcs(configStruct);

  if (stream.eof) {
    throw new Error("Array buffer ran out of space while sending config");
  }

  // we return a resized buffer (but actually BT doesn't like resizable
  // buffers so we use _transfer_
  return new DataView(buffer.transfer(stream.tellg));
}

/** Opens a file dialog box for saving the data.
 *
 *  This simulates a click on a temporary created <a> tag
 *  to trigger a download operation.
 */
function saveJson(jsonObj, exportName){
  const dataStr = "data:text/json;charset=utf-8," +
      encodeURIComponent(JSON.stringify(jsonObj, null, 2));

  let elA = document.createElement('a');
  elA.setAttribute("href",     dataStr);
  elA.setAttribute("download", exportName + ".json");
  document.body.appendChild(elA); // required for firefox
  elA.click();
  elA.remove();
}

function loadJson(notifyFunc){
  let elInput = document.createElement("input");
  elInput.type = "file";
  elInput.accept = "application/json";
  elInput.addEventListener("change", (event) => {
    if (event.target.files.length > 0) {
      const fr = new FileReader();
      fr.addEventListener("load", (loaded) => {
        const jsonObj = JSON.parse(loaded.target.result);
        notifyFunc(jsonObj);
      });
      fr.readAsText(event.target.files[0]); // begin reading
    }
  });

  document.body.appendChild(elInput);
  elInput.click();

  setTimeout(function() {
    document.body.removeChild(elInput);
  }, 0);
}

async function downloadConfig() {
  if (procStatusCallback) {
    procStatusCallback("Downloading proc configuration.");
  }

  let dataView = await bluetooth.downloadConfig()
  console.log("Received " + dataView.byteLength + " bytes.");
  updateConfigFromData(dataView);

  if (procStatusCallback) {
    procStatusCallback("Downloading done.", {"color": "green"});
  }
}


/** Uploads the configuration, ensuring audios are in place.
 *
 *  For a valid config we first need to upload the audio data,
 *  and then the new config.
 */
async function uploadConfig() {
  let configStruct = getConfigFromTable();

  if (procStatusCallback) {
    procStatusCallback("Uploading proc configuration.");
  }

  try {
    // upload samples
    await uploadMissingSamples(configStruct);

    // upload the rest of the config
    let dataView = encodeConfig(configStruct);
    console.log("Uploading " + dataView.byteLength + " bytes.");
    await bluetooth.uploadConfig(dataView);

  } catch(error) {
    console.error("Error writing Audio characteristics: " + error)
    if (procStatusCallback) {
      procStatusCallback("Uploading failed. Try again.", {"color": "red"});
      return;
    }
  }

  if (procStatusCallback) {
    procStatusCallback("Uploading done.", {"color": "green"});
  }
}


/** Resets the configuration by sending an empty one.
 *
 */
async function resetConfig() {
  let configStruct = [];

  await resetSamples();

  // upload an empty config (triggers a reset)
  let dataView = encodeConfig(configStruct);
  await bluetooth.uploadConfig(dataView);
}


/** Saves the whole config to the user's disk
 */
function saveConfig() {
  saveJson(getConfigFromTable(), "rc_config");
}

/** Loads the whole config from the user's disk
 */
function loadConfig() {
  loadJson(updateConfigFromConfig);
}

/** Saves part of the config from one proc_group to the other
 *
 *  @param elGroup A group DOM element (actually a *li* DOM element created
 *    by createConfigGroup()
 */
function saveGroupConfig(elGroup) {
  // -- first get a list of DOM elements
  let elList = recursiveGetConfigElements(elGroup.configUl);

  // -- get the config from the DOM elements
  let configList = [];
  for (let i = 0; i < elList.length; i++) {
    configList.push(elList[i].getConfig());
  }

  if (configList.length == 0) {
    console.log("No procs in group when saving.");
  } else {
    saveJson(configList, "rc_config_group");
  }
}

/** Loads and replaces part of the config from one proc_group to the other
 *
 *  @param elGroup A group DOM element (actually a *li* DOM element created
 *    by createConfigGroup()
 */
function loadGroupConfig(elGroup) {
  let elUl = elGroup.configUl;
  elUl.innerHTML = "";

  loadJson((config) => fillConfigElement(elUl, config));
}

/** Move a group or proc element to the front.
 *
 *  @param element A DOM element to move.
 */
function moveUp(element) {

    if (element.previousSibling != null) {
      if (element.previousSibling?.configUl &&
          element.previousSibling?.open()) {
        // move into the group
        element.previousSibling.configUl.appendChild(
            element,
        );
      } else {
        element.parentElement.insertBefore(element, element.previousSibling);
      }

    } else {
      // we have to move out of the group to move up.

      // search a parent whose parent is an UL (that's my group)
      let group = element.parentElement;
      while (group != null && group.parentElement != null &&
        !group?.configUl) {
        group = group.parentElement;
      }

      group.parentElement.insertBefore(element, group);
    }
}

/** Move a group or proc element to the back.
 *
 *  @param element A DOM element to move.
 */
function moveDown(element) {

    if (element.nextSibling != null) {
      if (element.nextSibling?.configUl &&
          element.nextSibling?.open()) {
        // move into the group
        element.nextSibling.configUl.insertBefore(
            element,
            element.nextSibling.configUl.firstChild
        );
      } else {
        element.parentElement.insertBefore(element, element.nextSibling.nextSibling);
      }

    } else {
      // we have to move out of the group to move down.

      // search a parent whose parent is an UL (that's my group)
      let group = element.parentElement;
      while (group != null && group.parentElement != null &&
        !group?.configUl) {
        group = group.parentElement;
      }

      group.parentElement.insertBefore(element,
          group.nextSibling);
    }
}

function newProc() {
  const elUl = document.getElementById(configTableId);

  const elLi = createNewElement();
  elUl.insertBefore(elLi, elUl.firstChild);
}

function procSetStatusCallback(func) {
  sampleSetStatusCallback(func);
  procStatusCallback = func;
}

export {fillConfigTable,
    updateConfigFromConfig,
    updateConfigFromData,
    downloadConfig,
    uploadConfig,
    resetConfig,
    saveConfig,
    loadConfig,
    newProc,
    procSetStatusCallback,
}

