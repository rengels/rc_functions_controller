/** BLE handling functions.
 *
 *  This file contains several functions for handling
 *  bluetooth low energy.
 *
 *  Mostly uploading and downloading of:
 *  - signals
 *  - config (proc)
 *  - audio
 *
 *  Users must register callback functions.
 */

// Global Variables to Handle Bluetooth

const uuidSignalsService = "31522e91-04d6-dfae-2042-3a40b42d393f";
const uuidSignalsCharacteristic = "177d4281-2c71-50b2-264f-3fb3f9b556a8";
const uuidConfigService = "32522e91-04d6-dfae-2042-3a40b42d393f";
const uuidConfigCharacteristic = "187d4281-2c71-50b2-264f-3fb3f9b556a8";
const uuidAudioCharacteristic  = "197d4281-2c71-50b2-264f-3fb3f9b556a8";
const uuidAudioListCharacteristic  = "1a7d4281-2c71-50b2-264f-3fb3f9b556a8";

/** Either null or the object returned by requestDevice().gatt.connect() */
let bleServer = null
/** Either null or the object returned by getPrimaryService() */
let serviceSignals = null;
/** Either null or the object returned by getPrimaryService() */
let serviceConfig = null;
/** The signals characteristics returned by getCharacteristics() */
let characteristicSignals;
/** The config characteristics returned by getCharacteristics() */
let characteristicConfig;
/** The audio characteristics returned by getCharacteristics() */
let characteristicAudio;
/** The audio list characteristics returned by getCharacteristics() */
let characteristicAudioList;

/** A callback function for status update
 *  of bluetooth.
 *
 *  Will get as parameters a string with a status message
 *  and an options object.
 */
let btStatusCallback = null;

/** A callback function for when new signals are received via
 *  bluetooth.
 *
 *  Will get as parameters a DataView object.
 */
let btSignalsUpdateCallback = null;

/** True if startNotifications was called.
 *
 *  This should be always done if btSignalsUpdateCallback != null
 */
let btSignalsNotificationsStarted = false;

var isChromium = window.chrome;

/** Check if BLE is available in your Browser */
function isWebBluetoothEnabled() {
    if (!navigator.bluetooth) {
      console.log("Web Bluetooth API is not available in this browser!");
      if (btStatusCallback) {
        if (isChrome) {
          btStatusCallback("Bluetooth is not available. You need to switch it on chrome://flags/#enable-experimental-web-platform-features",
              {"color": "red", "connected": false});
        } else {
          btStatusCallback("Web Bluetooth is not available. Try a different browser, e.g. Chrome",
             {"color": "red", "connected": false});
        }
      }
      return false;

    } else {
      console.log("Web Bluetooth API supported in this browser.");
      return true;
    }
}

function connect() {
  if (!isWebBluetoothEnabled()) {
    return;
  }
  if (bleServer) {
    return;  // already connected
  }
  connectToDevice();
}

// Connect to BLE Device and Enable Notifications
async function connectToDevice(){
  try {
    console.log("Initializing Bluetooth...");
    const device = await navigator.bluetooth.requestDevice({
      "filters": [{ "namePrefix": "RcFuncCtrl-"}],
      "optionalServices":  [uuidSignalsService, uuidConfigService]
    });

    console.log("Device Selected:", device.name);
    if (btStatusCallback) {
      btStatusCallback("Connecting to device " + device.name);
    }
    device.addEventListener("gattserverdisconnected", onDisconnected);

    // -- connecting
    try {
      bleServer = await device.gatt.connect();
    } catch(error) {
      console.log("First connect failed: ", error);
      // this happens sometime. Might be a noisy environment
    }

    // trying again
    if (!bleServer) {
      await Promise.resolve()
        .then(() => new Promise(resolve => setTimeout(resolve, 200)))
      bleServer = await device.gatt.connect();
    }

    console.log("Connected to GATT Server");

    // -- discovering all our services
    serviceSignals = await bleServer.getPrimaryService(uuidSignalsService);
    console.log("Service discovered:", serviceSignals.uuid);

    characteristicSignals = await serviceSignals.getCharacteristic(uuidSignalsCharacteristic);
    console.log("Characteristic discovered:", characteristicSignals.uuid);
    if (btSignalsUpdateCallback) {
      startSignalsNotification(btSignalsUpdateCallback);
    }

    serviceConfig = await bleServer.getPrimaryService(uuidConfigService);
    console.log("Service discovered:", serviceConfig.uuid);

    characteristicConfig = await serviceConfig.getCharacteristic(uuidConfigCharacteristic);
    characteristicAudio = await serviceConfig.getCharacteristic(uuidAudioCharacteristic);
    characteristicAudioList = await serviceConfig.getCharacteristic(uuidAudioListCharacteristic);

    if (btStatusCallback) {
      btStatusCallback("Connected to device " + device.name,
        {"color": "green", "connected": true});
    }

  } catch(error) {
    console.log("Error: ", error);
    if (btStatusCallback) {
      btStatusCallback(error,
          {"color": "red", "connected": false} );
    }
    if (bleServer) {
      bleServer.disconnect();
    }
  }
}

function onDisconnected(event){
  console.log("Device Disconnected:", event.target.name);

  bleServer = null;
  serviceSignals = null;
  characteristicSignals = null;
  characteristicConfig = null;
  characteristicAudio = null;
  characteristicAudioList = null;
  btSignalsNotificationsStarted = false;
  if (btStatusCallback) {
    btStatusCallback("Disconnected",
        {"color": "red", "connected": false});
  }
}

/** Disconnects from the server.
 */
async function disconnect() {
  console.log("Disconnect Device.");
  
  if (bleServer && bleServer.connected) {
    stopSignalsNotification()
    .then(() => {
      console.log("Notifications Stopped");
      return bleServer.disconnect();
    })
    .catch(error => {
      console.log("Error while disconnecting: ", error);
    });
  } else {
    // Throw an error if Bluetooth is not connected
    console.error("Error Bluetooth is not connected.");
  }

  if (btStatusCallback) {
    btStatusCallback("Disconnected",
        {"color": "red", "connected": false});
  }
}

/** Called when the characteristics update notification is received. */
async function handleSignalsUpdate(event) {
  if (btSignalsUpdateCallback) {
    btSignalsUpdateCallback(event.target.value);
  }
}

async function uploadSignals(dataView) {
  if (bleServer && bleServer.connected && characteristicSignals) {
    try {
      return characteristicSignals.writeValue(dataView.buffer);
    } catch(error) {
        console.error("Error writing signals")
    }
  }
}

async function downloadConfig() {
  if (bleServer && bleServer.connected && characteristicConfig) {
      let dataView = await characteristicConfig.readValue();
      return dataView;
  }
  return null;
}

async function uploadConfig(dataView) {
  if (bleServer && bleServer.connected && characteristicConfig) {
    try {
      await characteristicConfig.writeValue(dataView.buffer);
    } catch(error) {
        console.error("Error writing config characteristics")
    }
  }
}

/** Sends a message to the audio characteristics.
 *
 *  This is not a complete file, but instead an audio command.
 *  Function returns once the sending operation finished.
 */
async function uploadAudio(dataView) {
  if (bleServer && bleServer.connected && characteristicAudio) {
    // three retrys
    for (let i = 0; i < 3; i++) {
      try {
        await characteristicAudio.writeValue(dataView.buffer);
        return;
      } catch(error) {
        console.error("Error writing Audio characteristics: " + error)
      }
      await Promise.resolve()
        .then(() => new Promise(resolve => setTimeout(resolve, 200)))
    }
    throw new Error("Error uploading: unable to write.");
  }
  throw new Error("Error uploading: bluetooth not connected.");
}

async function downloadAudioList() {
  if (bleServer && bleServer.connected && characteristicAudioList) {
    let dataView = await characteristicAudioList.readValue();
    return dataView;
  }
  return null;
}

function setStatusCallback(func) {
  btStatusCallback = func;
}

/** Starts notifications for signals service.
 *
 *  @param func A function that takes a DataView object with the signals message.
 *  @returns a promise with the notification update.
 */
function startSignalsNotification(func) {

  // we already have a notification running. so just  set the callback
  btSignalsUpdateCallback = func;

  if (characteristicSignals && !btSignalsNotificationsStarted) {
    characteristicSignals.startNotifications().then(_ => {
      console.log("> Notifications started");
      characteristicSignals.addEventListener("characteristicvaluechanged",
      handleSignalsUpdate);
      btSignalsNotificationsStarted = true;
    });
  } else {
    return Promise.resolve();
  }
}

/** Starts notifications for signals service.
 *
 *  @param func A function that takes a DataView object with the signals message.
 *  @returns a promise with stopNotification.
 */
function stopSignalsNotification(func) {
  btSignalsUpdateCallback = null;

  if (characteristicSignals && btSignalsNotificationsStarted) {
    btSignalsNotificationsStarted = false;
    return characteristicSignals.stopNotifications();
  } else {
    console.log("No characteristic found to disconnect.");
    return Promise.resolve();
  }
}

export {
  connect,
  disconnect,

  startSignalsNotification,
  stopSignalsNotification,
  uploadSignals,

  downloadConfig,
  uploadConfig,

  uploadAudio,
  downloadAudioList,

  setStatusCallback,
}
