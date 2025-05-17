/** Javascript module for handling samples
 *
 *  This is CRC calculation, uploading, and misc stuff.
 *
 *  For the rc functions controller.
 */

import {defProcs} from './def_procs.js';
import {SimpleInputStream, SimpleOutputStream} from './simple_stream.js';
import * as bluetooth from "./bluetooth.js";


// create an id to proc mapping for all procs
let procsNameMapping = {};

for (let key in defProcs) {
  if (defProcs.hasOwnProperty(key)) {
    const proc = defProcs[key]
    if ("name" in proc) {
      procsNameMapping[proc["name"]] = proc;
    }
  }
}

/** A callback function for status update.
 *
 *  Will get as parameters a string with a status message
 *  and an options dict.
 */
let sampleStatusCallback = null;

const AUDIO_CMD_RESET = 0;
const AUDIO_CMD_ADD = 1;
const AUDIO_CMD_ADD_DATA = 2;

/** This is the same crc implementation as used by esp32 from esp_rom_crc.c */
const crc16_le_table = [
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1,0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40,0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3,0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42,0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 0xce4c, 0xdfc5,0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44,0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7,0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46,0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,

    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9,0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948,0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 0x2942, 0x38cb,0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a,0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd,0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c,0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf,0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e,0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
];


/** This is the same crc implementation as used by esp32 from esp_rom_crc.c */
function esp_rom_crc16_le(crc, buf) {
    crc = (~crc) & 0xffff;
    for (let i = 0; i < buf.length; i++) {
        crc = crc16_le_table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }
    return ((~crc) & 0xffff);
}


// -- own implementation of base64 encoding.
// there does not really seem to be a nice portable way to convert
// from and to an array buffer

const BASE64STR = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

/** Returns a base 64 encoded string for an ArrayBuffer
 *
 *  @param buffer an ArrayBuffer with the input
 *  @returns A string with the encoding
 */
function base64Encode(buffer) {

  const view = new DataView(buffer);
  let result = "";

  for (let i = 0; i < buffer.byteLength; i+=3) {

    let code1 = view.getUint8(i);
    let code2 = 0;
    let code3 = 0;
    if (i + 1 < buffer.byteLength) {
      code2 = view.getUint8(i + 1);
    }
    if (i + 2 < buffer.byteLength) {
      code3 = view.getUint8(i + 2);
    }

    let encoded1 = code1 >> 2;
    let encoded2 = ((code1 & 0x3) << 4) | (code2 >> 4);
    let encoded3 = ((code2 & 0xf) << 2) | (code3 >> 6);
    let encoded4 = code3 & 0x3f;

    if (i + 1 >= buffer.byteLength) {
      encoded3 = 64;
      encoded4 = 64;
    }
    if (i + 2 >= buffer.byteLength) {
      encoded4 = 64;
    }

    result = result +
      BASE64STR.charAt(encoded1) +
      BASE64STR.charAt(encoded2) +
      BASE64STR.charAt(encoded3) +
      BASE64STR.charAt(encoded4);
  }

  return result;
}

/** Returns an ArrayBuffer for a base 64 encoded string.
 *
 *  @results buffer an ArrayBuffer with the output.
 */
function base64Decode(input) {

  let buffer = new ArrayBuffer(input.length / 4 * 3);
  const view = new DataView(buffer);
  let iOut = 0;

  for (let i = 0; i < input.length; i+=4) {

    let encoded1 = BASE64STR.indexOf(input.charAt(i));
    let encoded2 = "=";
    let encoded3 = "=";
    let encoded4 = "=";

    if (i + 1 < input.length) {
      encoded2 = BASE64STR.indexOf(input.charAt(i + 1));
      if (encoded2 != 64) {
        view.setUint8(iOut, (encoded1 << 2 | encoded2 >> 4));
        iOut++;
      }
    }
    if (i + 2 < input.length) {
      encoded3 = BASE64STR.indexOf(input.charAt(i + 2));
      if (encoded3 != 64) {
        view.setUint8(iOut, ((encoded2 & 0xf) << 4 | encoded3 >> 2));
        iOut++;
      }
    }
    if (i + 3 < input.length) {
      encoded4 = BASE64STR.indexOf(input.charAt(i + 3));
      if (encoded4 != 64) {
        view.setUint8(iOut, ((encoded3 & 0x3) << 6 | encoded4));
        iOut++;
      }
    }
  }

  return buffer;
}

/** A list of audio samples as returned e.g. by bluetooth.
 *
 *  This class provides several helper functions for handling
 *  such list and support sample upload.
 */
class AudioList {
  constructor() {
    this.list = [];
  }

  /** Pushes a new entry to the list.
   *
   *  @param id String with three character id. If "null"
   *    we will only add it if "length" and "crc" are unique.
   */
  push(id, length, crc) {
      if (id != null || !this.findSample(length, crc)) {
        this.list.push([id, length, crc]);
      }
  }

  /** Pushes a new entry to the list.
   *
   *  @param sample A tuple of id, length, crc
   */
  pushSample(sample) {
      this.list.push(sample);
  }

  /** Tries to find an audio sample in the list matching the given length and crc
  *
  *  @returns The list entry.
  */
  findSample(length, crc) {
    return this.list.find((audio) => (length == audio[1] && crc == audio[2]));
  }

  /** Returns the content of this audio list minus the other list.
   */
  substract(otherList) {
    var result = new AudioList();
    for (let i = 0; i < this.list.length; i++) {
      let sample = this.list[i];
      if (!otherList.findSample(sample[1], sample[2])) {
        result.pushSample(sample);
      }
    }
    return result;
  }

  /** Creates a unique sample ID not contained in the given audio list.
   *
   *  The ID does not contain special characters.
   *
   *  @returns a three character string not contained already in the audio list.
   */
  createSampleId(existingIds) {
    let code = 64 * 64 * 61;
    let id;

    do {
      code++;
      id = BASE64STR[Math.floor(code / 64 / 64) % 64] + 
        BASE64STR[Math.floor(code / 64) % 64] + 
        BASE64STR[(code) % 64];
    } while (existingIds.includes(id));

    return id;
  }

  /** Fills missing IDs.
   *
   *  Modifies the AudioList so that all entries have a unique ID.
   *  Takes IDs where missing from "otherList" or creating unique new ones.
   *
   *  @param[in] otherList An instance of AudioList
   */
  ensureIds(otherList) {
    // first try to fill the IDs from otherList
    for (let i = 0; i < this.list.length; i++) {
      let sample = this.list[i];


      // fill in missing IDs from the "otherList"
      if (!sample[0]) {
        let otherSample = otherList.findSample(sample[1], sample[2]);
        if (otherSample && otherSample[0]) {
          sample[0] = otherSample[0];
          continue;
        }
      }

      // fill in missing crc and length from the "otherList"
      if (sample[0] && !(sample[1] && sample[2])) {
        let otherSample = otherList.list.find((sample2) => (sample[0] == sample2[0]));
        if (otherSample) {
          sample[1] = otherSample[1];
          sample[2] = otherSample[2];
        }
      }
    }

    // first collect all ids
    let allIds = [];
    for (let i = 0; i < this.list.length; i++) {
      let sample = this.list[i];
      if (sample?.[0]) {
        allIds.push(sample[0]);
      }
    }
    for (let i = 0; i < otherList.list.length; i++) {
      let sample = otherList.list[i];
      if (sample?.[0]) {
        allIds.push(sample[0]);
      }
    }

    // now create unique IDs for the entries that still don't have an id
    for (let i = 0; i < this.list.length; i++) {
      let sample = this.list[i];
      if (!sample?.[0]) {
        sample[0] = this.createSampleId(allIds);
        allIds.push(sample[0]);
      }
    }
  }

  /** Calculates the number of sectors consumed by this list */
  sectorSize() {
    let neededSectors = 0;
    for (let i = 0; i < this.list.length; i++) {
      const length = this.list[i][1];
      neededSectors += Math.ceil((length + 16 /* size of block header*/) / 4096.0);
    }
    return neededSectors;
  }
}

/** Determines and returns a list of audio samples for a proc.
 *
 *  Goes through the vars of the proc looking for SampleData types.
 *  This function is very similar to SimpleOutputStream.writeProc().
 *
 *  @param value a Proc value config structure.
 *
 *  @returns a list of audio samples with [id, data, length, crc]
 */
function samplesForProc(value) {

    let result = [];

    if (!("name" in value)) {
      console.log("No 'name' in SimpleOutputStream::writeProc");
      return result;
    }

    if (!(value["name"] in procsNameMapping)) {
      console.log("Unknown name " + value["name"] +
        " in SimpleOutputStream::writeProc");
      return result;
    }

    const proc = procsNameMapping[value["name"]];

    if ("values" in proc) {
      const procValues = proc["values"];
      for (let i = 0; i < procValues.length; i++) {
        const procValue = procValues[i];
        const type = procValue["type"];
        const num = procValue?.["num"] ?? 1;

        let valueValue = 0;
        if (!("values" in value)) {
          console.log("No 'values' in config for " + procValue["name"] + ".");
        } else if (!(procValue["name"] in value["values"])) {
          console.log("No " + procValue["name"] + "in 'values' in config for " +
            value["name"] + ".");
        } else {
          valueValue = value["values"][procValue["name"]];
        }

        for (let j = 0; j < num; j++) {
          if (type == "SampleData" && valueValue?.[j]) {
            result.push(valueValue?.[j]);
          }
        }
      }
    }

    return result;
}


/** Downloads an audio list and returns the audio structure.
 *
 *  The audio list consists of elements with:
 *
 *  - three character ID
 *  - 16 bit length
 *  - 16 bit checksum
 *
 *  @returns An object with:
 *    - usedSectors Number of 4096 byte sectors already used up.
 *    - freeSectors Number of 4096 byte sectors free.
 *    - audioList: Instance of AudioList
 */
async function downloadAudioList() {

  let audioList = new AudioList();

  const dataView = await bluetooth.downloadAudioList();
  if (dataView != null) {
    let stream = new SimpleInputStream(dataView);

    // read header
    const magic = String.fromCharCode(stream.readUint8(), stream.readUint8());
    const version = stream.readUint8();
    if (magic != "RL") {
      console.log("Incorrect magic " + magic +
        " in downloadAudioList");
    }
    if (version != 1) {
      console.log("Incorrect version " + version +
        " in downloadAudioList");
    }

    const usedSectors = stream.readUint16();
    const freeSectors = stream.readUint16();

    const count = stream.readUint8();
    for (let i = 0; i < count; i++) {

      const id = stream.readAudioId();
      const length = stream.readUint32();
      const crc = stream.readUint16();

      console.log("Dynamic sample: " + id +
        " size: " + length + " crc: " + crc);

      audioList.push(id, length, crc);
    }
    return {"usedSectors": usedSectors, "freeSectors": freeSectors, "audioList": audioList};
  }
  return {"usedSectors": 0, "freeSectors": 0, "audioList": audioList};
}



/** Sends the reset cmd (0) via Bluetooth.
 */
async function resetSamples() {

  console.log("Resetting all dynamic samples.");

  const buffer = new ArrayBuffer(10);
  const dataView = new DataView(buffer);
  let stream = new SimpleOutputStream(dataView);

  stream.writeUint8('RA'.charCodeAt(0));
  stream.writeUint8('RA'.charCodeAt(1));
  stream.writeUint8(1);  // binary format version
  stream.writeUint8(AUDIO_CMD_RESET);

  await bluetooth.uploadAudio(new DataView(buffer.transfer(stream.tellg)));
}


/** Uploads one sample via BT.
 *
 *  Note: we create the buffers overly large and then
 *  resize them later to size.
 *
 *  @param audioArray an ArrayBuffer object with the audio file data.
 */
async function uploadSample(id, audioArray) {
  // seems like there is a limit around 500 bytes,
  // but messages are transmitted in blocks of 250 bytes
  const CHUNK_SIZE = 220; // the number of bytes transmitted in one go
  const INITIAL_BUFFER_SIZE = CHUNK_SIZE + 50;

  const uint8 = new Uint8Array(audioArray);

  // send "new sample" command
  {
    const buffer = new ArrayBuffer(INITIAL_BUFFER_SIZE);
    const dataView = new DataView(buffer);
    let stream = new SimpleOutputStream(dataView);

    stream.writeUint8('RA'.charCodeAt(0));
    stream.writeUint8('RA'.charCodeAt(1));
    stream.writeUint8(1);  // binary format version
    stream.writeUint8(AUDIO_CMD_ADD);
    stream.writeAudioId(id);
    stream.writeUint32(uint8.length);

    await bluetooth.uploadAudio(new DataView(buffer.transfer(stream.tellg)));
  }


  // send "add data" command
  for (let offset = 0; offset < uint8.length; offset += CHUNK_SIZE) {
    const buffer = new ArrayBuffer(INITIAL_BUFFER_SIZE);
    const dataView = new DataView(buffer);
    let stream = new SimpleOutputStream(dataView);

    const bytesToWrite = Math.min(uint8.length - offset, CHUNK_SIZE);

    stream.writeUint8('RA'.charCodeAt(0));
    stream.writeUint8('RA'.charCodeAt(1));
    stream.writeUint8(1);  // binary format version
    stream.writeUint8(AUDIO_CMD_ADD_DATA);
    stream.writeAudioId(id);
    stream.writeUint32(offset);
    stream.writeUint32(bytesToWrite);
    for (let i = 0; i < bytesToWrite; i++) {
      stream.writeUint8(uint8[i + offset]);
    }

    if (sampleStatusCallback) {
      sampleStatusCallback(null, {"progress": (100.0 * offset / uint8.length)});
    }

    await bluetooth.uploadAudio(new DataView(buffer.transfer(stream.tellg)));
  }
}


/** Goes through the whole configuration for SampleData values.
 *
 *  If those are not found in the audio list, uploads them and
 *  assigns an ID in the configuration.
 *
 *  @param configStruct A list of proc configurations.
 *  @returns true if upload worked, else false (e.g. samples won't fit)
 */
async function uploadMissingSamples(configStruct) {

  // -- find dynamic samples in config
  // go through the config to find custom audio samples
  let samplesList = [];
  for (let i = 0; i < configStruct.length; i++) {
    samplesList.push(...samplesForProc(configStruct[i]));
  }
  // only consider dynamic samples
  samplesList = samplesList.filter((sample) => !(typeof sample == "string"));
  if (samplesList.length == 0) {
    return true; // no dynamic samples, nothing to do
  }

  // -- convert dynamic samples to AudioList
  let necessaryAudioList = new AudioList();
  for (let i = 0; i < samplesList.length; i++) {
    let sample = samplesList[i];
    const id = sample?.id;
    const data = sample?.data;
    const length = sample?.length;
    const crc = sample?.crc;

    // if we have length, crc and data we can upload it
    if ((length != null) && (crc != null) && (data != null)) {
      necessaryAudioList.push(null, length, crc);

    // if we have an ID we can possibly re-use an already uploaded sample
    } else if (id != null) {
      necessaryAudioList.push(id, length, crc);
    }
  }

  // -- download list of dynamic samples
  if (sampleStatusCallback) {
    sampleStatusCallback("Downloading dynamic samples list.");
  }
  const listResult = await downloadAudioList();
  const usedSectors = listResult.usedSectors;
  const freeSectors = listResult.freeSectors;
  let oldAudioList = listResult.audioList;

  // list of all unique samples we need
  necessaryAudioList.ensureIds(oldAudioList);
  if (necessaryAudioList.sectorSize() > freeSectors + usedSectors) {
    // the audios won't fit. even with a reset
    if (sampleStatusCallback) {
      sampleStatusCallback("Custom/dynamic samples too big..",
          {"color": "red"});
    }
    return false;
  }

  // list of unique samples we need to upload but are not available on the device
  let newAudioList = necessaryAudioList.substract(oldAudioList);
  if (newAudioList.sectorSize() > freeSectors) {

    // TODO: a reset could remove an existing sample that we can't upload again
    // because we don't have the data.

    // need to do a reset
    resetSamples();
    oldAudioList = new AudioList();
    newAudioList = necessaryAudioList;
  }

  // -- upload samples
  for (let i = 0; i < samplesList.length; i++) {
    let sample = samplesList[i];
    if (!("data" in sample)) {
      // no data. we might have gotten the ID from a download.
      // so don't modify the sample.ID and we also can't upload
      // anything, since there is no data.
      continue;
    }

    // check if it's already been uploaded
    let length = sample?.length ?? 0;
    let crc = sample?.crc ?? 0;
    let oldSample = oldAudioList.findSample(length, crc);
    if (oldSample) {
      // update ID in proc
      sample.id = oldSample[0];
      continue;
    }

    // upload it now
    let newSample = newAudioList.findSample(length, crc);
    if (newSample) {
      // update ID in proc
      sample.id = newSample[0];

      if (sampleStatusCallback) {
          sampleStatusCallback(
              "Uploading " + sample?.filename + " " + length + " bytes.");
      }
      console.log("Uploading sample: " + sample.id +
          " size: " + length + " crc: " + crc);

      await uploadSample(sample.id, sample.data);
      oldAudioList.pushSample(newSample);
    }
  }

  if (sampleStatusCallback) {
    sampleStatusCallback("Dynamic samples upload done.");
  }
}

function sampleSetStatusCallback(func) {
  sampleStatusCallback = func;
}

export {
  esp_rom_crc16_le,
  uploadMissingSamples,
  sampleSetStatusCallback,
  resetSamples,
  base64Encode,
  base64Decode,
}
