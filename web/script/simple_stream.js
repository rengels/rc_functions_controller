/** Stream class that helps serializing and deserializing
 *  for the functions controller
 */

import {defProcs} from './def_procs.js';
import {defSignals} from './def_signals.js';

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

// create an id/name to signal mapping for all signals
let signalsIdMapping = {};
let signalsNameMapping = {};

let signalIndex = 0;
for (let i = 0; i < defSignals.length; i++ ) {
    let signal = defSignals[i]

    if ("name" in signal) {
      signalsIdMapping[signal["id"]] = signal;
      signalsNameMapping[signal["name"]] = signal;
    }
}


const RCSIGNAL_INVALID = -32768;

class SimpleInputStream {

  indexRead = 0;  ///< cursor for reading. Index in bytes.
  dataview = null;  ///< A DataView instance as a source.

  /** Construct a new stream from a dataview. */
  constructor(dataview) {
      this.dataview = dataview;
  }

  /** Returns true if we can't read another byte (because the buffer is
    *  at it's end.  */
  get eof() {
    return this.indexRead >= this.dataview.byteLength;
  }

  /** Returns the current writing index.  */
  get tellg() {
    return this.indexRead;
  }

  /** Reads a byte from the stream.
  *
  *  @returns the read byte or 0U in case there is no other byte to read.  */
  readInt8() {
    if (this.eof) {
      return 0;
    } else {
      return this.dataview.getInt8(this.indexRead++);
    }
  }

  /** Reads a byte from the stream.
  *
  *  @returns the read byte or 0U in case there is no other byte to read.  */
  readUint8() {
    if (this.eof) {
      return 0;
    } else {
      return this.dataview.getUint8(this.indexRead++);
    }
  }

  /** Reads a Uint16 from the stream.
  *
  *  @returns The uint16 value from the stream or 0
  *     in case the stream was at eof.  */
  readUint16() {
    if (this.indexRead + 1 >= this.dataview.byteLength) {
      return 0;
    } else {
      const res = this.dataview.getUint16(this.indexRead);
      this.indexRead += 2;
      return res;
    }
  }

  /** Reads a Uint32 from the stream.
  *
  *  @returns The uint16 value from the stream or 0
  *     in case the stream was at eof.  */
  readUint32() {
    if (this.indexRead + 3 >= this.dataview.byteLength) {
      return 0;
    } else {
      const res = this.dataview.getUint32(this.indexRead);
      this.indexRead += 4;
      return res;
    }
  }

  /** Reads a float from the stream.
  *
  *  @returns The float value from the stream or 0
  *     in case the stream was at eof.  */
  readFloat() {
    if (this.indexRead + 3 >= this.dataview.byteLength) {
      return 0.0;
    } else {
      const res = this.dataview.getFloat32(this.indexRead);
      this.indexRead += 4;
      return res;
    }
  }

  /** Reads a RcSignal from the stream.
  *
  *  @returns the RcSignal from the stream or RCSIGNAL_INVALID
  *     in case the stream was at eof.  */
  readRcSignal() {
    if (this.indexRead + 1 >= this.dataview.byteLength) {
      return RCSIGNAL_INVALID;
    } else {
      const res = this.dataview.getInt16(this.indexRead);
      this.indexRead += 2;
      return res;
    }
  }

  /** Reads a SignalType from the stream.
  *
  *  @returns the SignalType name or "ST_NONE"
  *     in case the stream was at eof.  */
  readSignalType() {
    const signalId = String.fromCharCode(this.readUint8());
    if (signalId in signalsIdMapping) {
      return signalsIdMapping[signalId]["name"];
    }
    return "ST_NONE";
  }

  /** Reads an audio sample id / sample data
  *
  *  :returns: A string with the sample ID
  */
  readSampleData() {
    return String.fromCharCode(this.readUint8(), this.readUint8(), this.readUint8());
  }

  /** Reads a Signals struct from the stream.
  *
  *  @returns the Signals from the stream as an array of ints */
  readSignals(count) {
    let signals = [];
    for (let i = 0; i < count; i++) {
      signals[i] = this.readRcSignal();
    }
    return signals;
  }

  /** Reads an audio ID (three bytes) and returns it as a string. */
  readAudioId() {
    return String.fromCharCode(
      this.readUint8(), this.readUint8(), this.readUint8());
  }

  /** Reads a rcProc::GearCollection and returns it as an array. */
  readGearCollection() {
    let numGears = this.readInt8();
    let ratios = [];
    for (let i = 0; i < numGears; i++) {
      ratios[i] = this.readInt8() / 10.0;  // fixed point times 10
    }
    return ratios;
  }

  /** Reads a rcProc::Idle and returns it as an dict. */
  readIdle() {
    return {
      "rpmIdleStart": this.readUint16(),
      "rpmIdleRunning": this.readUint16(),
      "loadStart": this.readRcSignal(),
      "timeStart": this.readUint32(),
      "throttleStep": this.readUint16()
    };
  }

  /** Reads the value with the given type.
   *
   *  @param type A string with the value type, e.g. "bool"
   *  @returns The read type.
   */
  readValue(type) {
    switch (type) {
      case "bool":
        return this.readUint8() > 0;
      case "uint8_t":
      case "gpio_num_t":
      case "EngineSimple::EngineType":
      case "InputDemo::DemoType":
      case "AudioNoise::NoiseType":
      case "ProcCombine::Function":
      case "OutputEsc::FreqType":
        return this.readUint8();
      case "uint16_t":
        return this.readUint16();
      case "uint32_t":
        return this.readUint32();
      case "float":
        return this.readFloat();
      case "RcSignal":
        return this.readRcSignal();
      case "TimeMs":
        return this.readUint32();
      case "SignalType":
        return this.readSignalType();
      case "SampleData":
        return this.readSampleData();
      case "Volume":
        return this.readUint8();
      case "rcEngine::GearCollection":
        return this.readGearCollection();
      case "rcEngine::Idle":
        return this.readIdle();
      default:
        console.log("Unhandled type " + type +
        " in SimpleInputStream::readValue");
        return null;
    }
  }

  /** Reads a Proc struct from the stream.
  *
  *  @returns a proc config structure from the stream. */
  readProc() {
    const procId = String.fromCharCode(this.readUint8(), this.readUint8());
    const len = this.readUint8();
    const endPos = this.indexRead + len;

    if (!(procId in procsIdMapping)) {
      this.indexRead = endPos;
      console.log("Unknown proc ID " + procId +
        " in SimpleInputStream::readProc");
      return null;
    }

    const proc = procsIdMapping[procId];
    const config = {"name": proc["name"]};

    // -- types
    if ("types" in proc) {
      config["types"] = {}; 
      const procTypes = proc["types"];
      for (let i = 0; i < procTypes.length; i++) {
        const type = procTypes[i];
        const num = type?.["num"] ?? 1;

        let result = [];
        for (let j = 0; j < num; j++) {
          result.push(this.readSignalType());
        }
        config["types"][type["name"]] = result;
      }
    }

    // -- values
    if ("values" in proc) {
      config["values"] = {}; 
      const procValues = proc["values"];
      for (let i = 0; i < procValues.length; i++) {
        const value = procValues[i];
        const type = value["type"];
        const num = value?.["num"] ?? 1;

        // we use an array even for single values
        // makes it easier to write an event function later on.
        let result = [];
        for (let j = 0; j < num; j++) {
          result.push(this.readValue(type));
        }
        config["values"][value["name"]] = result;
      }
    }

    // in case we did end up reading a different amount of
    // bytes than expected
    if (this.indexRead != endPos) {
      console.log("Inconsistent proc length vs. proc configuration in " + procId +
        " " + len.toString() + " vs. " +
        (this.indexRead - endPos + len).toString() + " in SimpleInputStream::readProc");
      // we return the config although it might be corrupt.
      // and we trust the value in the stream more than our own.
      this.indexRead = endPos;
    }

    return config;
  }

  /** Reads list of procs struct from the stream.
  *
  *  @returns the Procs from the stream */
  readProcs() {
    // read header
    const magic = String.fromCharCode(this.readUint8(), this.readUint8());
    const version = this.readUint8();
    if (magic != "RC") {
      console.log("Incorrect magic " + magic +
        " in SimpleInputStream::readProcs");
    }
    if (version != 1) {
      console.log("Incorrect version " + version +
        " in SimpleInputStream::readProcs");
    }

    const count = this.readUint8();

    let result = [];
    for (let i = 0; i < count; i++) {
      const proc = this.readProc();
      if (proc != null) {
        result.push(proc);
      }
    }
    return result;
  }

} // end SimpleInputStream

class SimpleOutputStream {

  indexWrite = 0;  ///< cursor for writeing. Index in bytes.
  dataview = null;  ///< A DataView instance as a source.

  /** Construct a new stream from a dataview. */
  constructor(dataview) {
      this.dataview = dataview;
  }

  /** Returns true if we can't write another byte (because the buffer is
    *  at it's end.  */
  get eof() {
    return this.indexWrite >= this.dataview.byteLength;
  }

  /** Returns the current writing index.  */
  get tellg() {
    return this.indexWrite;
  }

  /** Set's write index to pos */
  seekg(pos) {
    this.indexWrite = pos;
  }

  /** Writes a boolean value to the stream.  */
  writeBool(value) {
    return this.dataview.setUint8(this.indexWrite++, value);
    if (value) {
      return this.writeUint8(1);
    } else {
      return this.writeUint8(0);
    }
  }

  /** Writes a byte to the stream.  */
  writeInt8(value) {
    if (!this.eof) {
      return this.dataview.setInt8(this.indexWrite++, value);
    }
  }

  /** Writes a byte to the stream.  */
  writeUint8(value) {
    if (!this.eof) {
      return this.dataview.setUint8(this.indexWrite++, value);
    }
  }

  /** Writes a Uint16 to the stream. */
  writeUint16(value) {
    this.dataview.setUint16(this.indexWrite, value);
    this.indexWrite += 2;
  }

  /** Writes a Uint32 to the stream. */
  writeUint32(value) {
    this.dataview.setUint32(this.indexWrite, value);
    this.indexWrite += 4;
  }

  /** Writes a float to the stream. */
  writeFloat(value) {
    this.dataview.setFloat32(this.indexWrite, value);
    this.indexWrite += 4;
  }

  /** Writes a RcSignal to the stream. */
  writeRcSignal(value) {
    this.dataview.setInt16(this.indexWrite, value);
    this.indexWrite += 2;
  }

  /** Writes a sample ID to the stream. */
  writeSampleData(value) {
    if (typeof value == "string") {
      if (value.length >= 3) {
        this.writeUint8(value.charCodeAt(0));
        this.writeUint8(value.charCodeAt(1));
        this.writeUint8(value.charCodeAt(2));
      } else {
        console.log("Sample ID too short.");
        this.writeSampleData("SHO");
      }
    } else {
      // check if it's a custom/dynamic sample
      if (value?.id) {
        this.writeSampleData(value.id);
      } else {
        console.log("Sample ID not a string.");
        this.writeSampleData("NAS");
      }
    }
  }

  /** Writes a SignalType from the stream.
  *
  *  @param value Eiter a number with the numeric ID of the signal type or
  *    a string with the signal name.
  */
  writeSignalType(value) {
    if (typeof value == "string") {
      if (value in signalsNameMapping) {
          value = (signalsNameMapping[value]["id"]).charCodeAt(0);
      } else {
          value = 0;
      }
    }
    this.writeUint8(value);
  }

  /** Writes an array of signal types from stream.
  *
  *  @param num The expected number of values to write.
  *  @param value An array of either type IDs or type names.
  */
  writeSignalTypes(num, value) {
    for (let i = 0; i < value.length; i++) {
      this.writeSignalType(value?.[i]);
    }
  }

  writeAudioId(value) {
    this.writeUint8(value.charCodeAt(0));
    this.writeUint8(value.charCodeAt(1));
    this.writeUint8(value.charCodeAt(2));
  }

  writeGearCollection(value) {
    if (!Array.isArray(value)) {
      console.log("Values should be an array in writeGearCollection().");
      value = [value];
    }

    // count non-zero gear ratios
    let numGears = 0;
    for (let i = 0; i < value.length; i++) {
      if (value[i]) {
        numGears++;
      }
    }

    // write the numbers
    this.writeInt8(numGears);
    for (let i = 0; i < value.length; i++) {
      let num = value[i];
      if (typeof num != 'number') {
        num = 1.0;
      }
      num = num * 10.0;  // fixed point times 10
      if (num > 127) {
        num = 127;
      }
      if (num < -127) {
        num = -127;
      }
      this.writeInt8(num);
    }
  }

  /** Writes a rcEngine::Idle struct from a dict. */
  writeIdle(value) {
    this.writeUint16(value?.["rpmIdleStart"] ?? 0);
    this.writeUint16(value?.["rpmIdleRunning"] ?? 0);
    this.writeRcSignal(value?.["loadStart"] ?? 0);
    this.writeUint32(value?.["timeStart"] ?? 0);
    this.writeUint16(value?.["throttleStep"] ?? 0);
  }

  /** Writes a Signals struct from the stream.
  *
  *  @param value An array of signal values. */
  writeSignals(value) {
    for (let i = 0; i < value.length; i++) {
      this.writeRcSignal(value[i]);
    }
  }

  /** Writes a value with the specific type.
  *
  *  @param type A string with the type, e.g. "bool"
  *  @param value The proc value. Not an array of values.
  */
  writeValue(type, valueValue) {
    switch (type) {
    case "bool":
      this.writeBool(valueValue);
      break;
    case "uint8_t":
    case "gpio_num_t":
    case "EngineSimple::EngineType":
    case "InputDemo::DemoType":
    case "AudioNoise::NoiseType":
    case "ProcCombine::Function":
    case "OutputEsc::FreqType":
      this.writeUint8(valueValue);
      break;
    case "uint16_t":
      this.writeUint16(valueValue);
      break;
    case "uint32_t":
      this.writeUint32(valueValue);
      break;
    case "float":
      this.writeFloat(valueValue);
      break;
    case "RcSignal":
      this.writeRcSignal(valueValue);
      break;
    case "TimeMs":
      this.writeUint32(valueValue);
      break;
    case "SignalType":
      this.writeSignalType(valueValue);
      break;
    case "SampleData":
      this.writeSampleData(valueValue);
      break;
    case "Volume":
      this.writeUint8(valueValue); // html side threats it as an int
      break;
    case "rcEngine::GearCollection":
      this.writeGearCollection(valueValue);
      break;
    case "rcEngine::Idle":
      this.writeIdle(valueValue);
      break;
    default:
      console.log("Unhandled value " + type +
        " in SimpleInputStream::writeProc");
    }
  }

  /** Writes a Proc struct from the stream.
  *
  *  @param value A proc config structure. */
  writeProc(value) {

    if (!("name" in value)) {
      console.log("No 'name' in SimpleOutputStream::writeProc");
      return;
    }

    if (!(value["name"] in procsNameMapping)) {
      console.log("Unknown name " + value["name"] +
        " in SimpleOutputStream::writeProc");
      return;  // should be an error
    }

    const proc = procsNameMapping[value["name"]];

    this.writeUint8(proc["id"].charCodeAt(0));
    this.writeUint8(proc["id"].charCodeAt(1));
    const lenPos = this.tellg;
    this.writeUint8(0); // Placeholder for the length

    // -- types
    if ("types" in proc) {
      const procTypes = proc["types"];
      for (let i = 0; i < procTypes.length; i++) {
        const type = procTypes[i];
        const num = type?.["num"] ?? 1;

        let typesArray = [];
        if (!("types" in value)) {
          console.log("No 'types' in config for " + value["name"] + ".");
        } else if (!(type["name"] in value["types"])) {
          console.log("No " + type["name"] + "in 'types' in config for " +
            value["name"] + ".");
        } else {
          typesArray = value["types"][type["name"]];
        }

        this.writeSignalTypes(num, typesArray);
      }
    }

    // -- values
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
          this.writeValue(type, valueValue?.[j]);
        }
      }
    }

    // update the length
    const len = this.tellg - lenPos - 1;
    this.seekg(lenPos);
    this.writeUint8(len); // Placeholder for the length
    this.seekg(lenPos + len + 1);
  }

  /** Writes list of procs struct from the stream.
  *
  *  @param value An array of proc structs.
  */
  writeProcs(value) {
    this.writeUint8('RC'.charCodeAt(0));
    this.writeUint8('RC'.charCodeAt(1));
    this.writeUint8(1);  // binary format version

    this.writeUint8(value.length);
    for (let i = 0; i < value.length; i++) {
      this.writeProc(value[i]);
    }
  }

} // end SimpleOutputStream

export {SimpleInputStream, SimpleOutputStream};

