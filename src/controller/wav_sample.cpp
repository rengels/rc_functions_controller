/** File contains the static definitions for the audio sounds.
 *
 * @file
 */

#include "wav_sample.h"
#include "simple_byte_stream.h"

#include <cstdint>
#include <cstring>
#include <cstdio>

static constexpr const char* const FORMAT_CHUNK_NAME = "fmt ";
static constexpr const char* const DATA_CHUNK_NAME = "data";

/** Reads information from a wave data section
 *
 *  The input stream should be just after the section lable
 *
 *  @returns False in case of an error with the steam
 */
static std::span<const uint8_t> readDataChunk(SimpleInStream& is) {
    auto sectionLength = is.readUint32le();
    auto sectionEnd = is.tellg() + sectionLength;

    if (is.tellg() + sectionLength > is.buffer().size()) {
        sectionLength = is.buffer().size() - is.tellg();
    }
    auto result = is.buffer().subspan(is.tellg(), sectionLength);

    is.seekg(sectionEnd);
    return result;
}

/** Reads information from a format data section
 *
 *  The input stream should be just after the section lable
 *
 *  @returns False in case of an error with the steam
 */
static bool readFormatChunk(SimpleInStream& is) {
    auto sectionLength = is.readUint32le();
    if (sectionLength < 16) {
        printf("WAV: Invalid format section length.\n");
        return false;
    }
    auto sectionEnd = is.tellg() + sectionLength;

    auto formatType = is.readUint16le();
    auto channels = is.readUint16le();
    /*auto sampleRate =*/ is.readUint32le();

    /*auto bytePerSec =*/ is.readUint32le();
    /*auto bytePerBlock =*/ is.readUint16le();
    auto bitsPerSample = is.readUint16le();

    if (formatType != 1u ||
        channels != 1 ||
        bitsPerSample != 8) {
        printf("WAV: Invalid format, channel or bitcount for wav file.\n");
        return false;
    }

    is.seekg(sectionEnd);
    return true;
}

/** Jumps over a section
 *
 *  The input stream should be just after the section lable
 *
 *  @returns False in case of an error with the steam
 */
static bool readAnyChunk(SimpleInStream& is) {
    auto sectionLength = is.readUint32le();
    auto sectionEnd = is.tellg() + sectionLength;

    // read over the section
    is.seekg(sectionEnd);
    return true;
}

std::span<const uint8_t> getWavSamples(const std::span<const uint8_t>& wavData) {

    SimpleInStream is(wavData);

    if (is.read<char>() != 'R' ||
        is.read<char>() != 'I' ||
        is.read<char>() != 'F' ||
        is.read<char>() != 'F') {
        printf("WAV: Invalid header.\n");
        return wavData;
    }

    /*uint32_t fileLength = */ is.readUint32le();

    if (is.read<char>() != 'W' ||
        is.read<char>() != 'A' ||
        is.read<char>() != 'V' ||
        is.read<char>() != 'E') {
        printf("WAV: Invalid header 2.\n");
        return wavData;
    }

    // -- now read sections
    while (!is.eof() && !is.fail()) {

        // read the section name
        char name[5];
        for (uint8_t i = 0; i < 4; i++) {
            name[i] = is.read<char>();
        }
        name[4] = 0;

        // read the section
        bool ok;
        if (strcmp(name, FORMAT_CHUNK_NAME) == 0) {
            ok = readFormatChunk(is);
        } else if (strcmp(name, DATA_CHUNK_NAME) == 0) {
            auto sampleData = readDataChunk(is);
            return sampleData;
        } else {
            ok = readAnyChunk(is);
        }

        // in case of invalid section, fall back to raw
        if (!ok) {
            return wavData;
        }
    }
    return wavData;
}
