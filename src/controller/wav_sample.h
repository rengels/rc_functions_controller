/** File contains the static definitions for the audio sounds.
 *
 * @file
 */

#ifndef _WAV_SAMPLE_H_
#define _WAV_SAMPLE_H_

#include <cstdint>
#include <span>

class SimpleInStream;

static constexpr uint32_t SAMPLE_RATE = 22050; // fixed sample rate for audio

/** Tries to decode a wav file and return the data chunk content.
 *
 *  This function is used by the Storage class to decode
 *  audio data.
 *
 *  It just understands 8 bit single channel wav unsigned files.
 *  Also LIST and INFO chunks are not supported.
 *  It has to be a flat wav file.
 *
 *  In all other cases it will fall back to considering the data
 *  as raw data.
 *
 *  @see https://en.wikipedia.org/wiki/WAV
 *  @returns The data chunk content or the whole content in case
 *    it couldn't be decoded.
 */
std::span<const uint8_t> getWavSamples(const std::span<const uint8_t>& wavData);

#endif // _WAV_SAMPLE_H_

