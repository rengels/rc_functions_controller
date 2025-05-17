
let context = null;

/** Takes an array buffer and put's it into an AudioContext to decode it. */
async function arrayBufferToAudio(array) {
  if (context == null) {
    context = new AudioContext();
  }
  const audio = await context.decodeAudioData(array);
  return audio;
}


/** Converts an audio buffer to a wav file
 *
 *  Important note: wav files are little endian,
 *  so we don't use the SimpleOutputStream
 *
 *  @param normalize If true the maximum values will be scaled to 100%
 *  @param sampleRate The new sample rate (for us 22050) of the audio.
 *    The original sample rate for the audio buffer is always
 *    implementation dependent and not the one from the original audio
 *    file.
 *
 *  @returns An float 32 array with the resampled and normalized
 *    audio data.
 */
function audioToBuffer(audioBuffer, sampleRate = 22050) {

  // float 32 array (-1..1)
	let buffer = audioBuffer.getChannelData(0);

  // -- convert to mono if not already
  // (add the other channels to channel 0)
	if (audioBuffer.numberOfChannels > 1)
	{
		for(let c = 1; c < audioBuffer.numberOfChannels; c++)
		{
			const cb = audioBuffer.getChannelData(c);
			for(let i = 0; i < cp.length; i++)
				buffer[i] += cb[i];
		}
	}
	
  // -- resample
  // seems like audioBuffer.sampleRate is the playback sample
  // rate which is OS specific.
  let array;
	{
    const origSampleRate = audioBuffer.length / audioBuffer.duration;
		const scale = audioBuffer.sampleRate / sampleRate;
		const length = Math.floor((buffer.length - 1) / scale); 
    array = new Float32Array(length);
		for(let i = 0; i < length; i++)
		{
			const p = Math.floor(i * scale);
			array[i] = buffer[p];
		}
	}

  return array
}

/** Converts an audio buffer to a wav file
 *
 *  Important note: wav files are little endian,
 *  so we don't use the SimpleOutputStream
 *
 *  @param array An float array with the audio data (1 channel, 22050 sample rate)
 *
 *  @returns An ArrayBuffer with the new audio file
 */
function bufferToWav(array, sampleRate = 22050) {

  const bytesNo = 1;  // number of bytes per sample
  const channels = 1;
  const samplesNo = array.length;
  const fileLength = samplesNo * bytesNo + 44;
  
  const buf = new ArrayBuffer(fileLength);
  const view = new DataView(buf, 0)

  // -- RIFF chunk
  view.setUint8(0, "R".charCodeAt(0));
  view.setUint8(1, "I".charCodeAt(0));
  view.setUint8(2, "F".charCodeAt(0));
  view.setUint8(3, "F".charCodeAt(0));
  view.setUint32(4, fileLength - 8, true)

  // -- format chunk
  view.setUint8(8, "W".charCodeAt(0));
  view.setUint8(9, "A".charCodeAt(0));
  view.setUint8(10, "V".charCodeAt(0));
  view.setUint8(11, "E".charCodeAt(0));

  view.setUint8(12, "f".charCodeAt(0));
  view.setUint8(13, "m".charCodeAt(0));
  view.setUint8(14, "t".charCodeAt(0));
  view.setUint8(15, " ".charCodeAt(0));

  view.setUint32(16, 16, true); // format section length
  view.setUint16(20, 1, true);  // format type (PCM)
  view.setUint16(22, channels, true); // format type (PCM)
  view.setUint32(24, sampleRate, true);

  const bitRate = bytesNo * channels * sampleRate;
  view.setUint32(28, bitRate, true);
  view.setUint16(32, bytesNo * channels, true);
  view.setUint16(34, bytesNo * 8, true);

  // -- data chunk
  view.setUint8(36, "d".charCodeAt(0));
  view.setUint8(37, "a".charCodeAt(0));
  view.setUint8(38, "t".charCodeAt(0));
  view.setUint8(39, "a".charCodeAt(0));

  // clamp function
  const clamp = (num, min, max) => Math.min(Math.max(num, min), max)

  const dataSectionLength = bytesNo * samplesNo;
  view.setUint32(40, dataSectionLength, true); // data section length
  let pos = 44;
	for (let i = 0; i < array.length; i++)
	{
		let out = Math.round(array[i] * 127.0);
   
    if (bytesNo == 1) {
      view.setUint8(pos,
          clamp(
              Math.round(array[i] * (1 << 7) + (1 << 7)),
              -127, 127));
      pos++;
    } else if (bytesNo == 2) {
      view.setUint16(pos,
          clamp(
              Math.round(array[i] * (1 << 15)),
              -(1 << 15) + 1, (1 << 15) - 1), true);
      pos+=2;
    } else if (bytesNo == 4) {
      view.setUint32(pos,
          clamp(
              Math.round(array[i] * (1 << 31)),
              -(1 << 31) + 1, (1 << 31) - 1), true);
      pos+=4;
    }
	}	

	return buf;
}

export {
  arrayBufferToAudio,
  audioToBuffer,
  bufferToWav
}
