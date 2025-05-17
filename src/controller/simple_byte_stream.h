/** This file contains a simple implementation of a byte stream.
 *
 *  @file
 */

#ifndef _SIMPLE_BYTE_STREAM_H_
#define _SIMPLE_BYTE_STREAM_H_

#include <cstdint>
#include <array>
#include <vector>
#include <span>
#include "signals.h"
#include "sample.h"
#include "audio.h"

#ifdef ARDUINO
#include <hal/gpio_types.h>  // for the declaration of gpio_num_t
#endif

namespace rcEngine {
    class Idle;  // forward declaration from engine_idle.h
    class GearCollection;  // forward declaration from engine_gear.h
}

namespace rcOutput {
    enum class FreqType : uint8_t;
}

/** This class represents a simple byte stream with a provided buffer.
 *
 *  We use this byte stream for saving and restoring configuration.
 *  It's smaller and easier to work with than protobuf.
 *  And it's less band-width consuming than Json.
 *
 *  The data is big endian.
 *
 */
class SimpleInStream {
    private:
        uint32_t indexRead; ///< cursor for reading
        const std::span<const uint8_t> buf;

        /** Set to true if a read operation tried to read bejond the end
         *  of the buffer.
         */
        bool failFlag;

    public:
        /** Construct a new byte stream. */
        SimpleInStream(const std::span<const uint8_t>& bufVal);

        /** Returns the original buffer */
        const std::span<const uint8_t>& buffer() const {
            return buf;
        }

        /** Returns true if we can't read another byte (because the buffer is
         *  at it's end.  */
        bool eof() const {
            return indexRead >= buf.size();
        }

        /** Returns if a read operation tried to read bejond the end
         *  of the buffer.
         */
        bool fail() const {
            return failFlag;
        }

        /** Returns the current read index in bytes from the start. */
        uint32_t tellg() const {
            return indexRead;
        }

        /** Set the current read index in bytes from the start. */
        void seekg(uint32_t pos) {
            if (pos > buf.size()) {
                failFlag = true;
            }
            indexRead = pos;
        }

        /** Reads a byte from the stream.
         *
         *  @returns the read byte or 0U in case there is no other byte to read.  */
        uint8_t readUint8();

        /** Reads an uint16 value from the stream little endian.
         *
         *  This function is for the wav file reading which is little endian.
         */
        uint16_t readUint16le();

        /** Reads an uint32 value from the stream little endian.
         *
         *  This function is for the wav file reading which is little endian.
         */
        uint32_t readUint32le();

        template <typename T> T read() {
            T val;
            *this >> val;
            return val;
        }
};


/** This class represents a simple byte stream with a provided buffer.
 *
 *  We use this byte stream for saving and restoring configuration.
 *  It's smaller and easier to work with than protobuf.
 *  And it's less band-width consuming than Json.
 *
 *  The data is little endian.
 *
 *  Regarding buffer management:
 *  The buffer is assumed to be dynamically allocated.
 *  It will be re-allocated if it's too small, so be aware of that.
 *
 *  Vital: the buffer has to be freed or memory will leak.
 *  Note: C-style memory management is used because we want to use the
 *    buffer in the C-style queue and bluetooth parts of the code.
 */
class SimpleOutStream {
    private:
        uint32_t indexWrite; ///< cursor for writing
        std::span<uint8_t> buf;

        /** Set to true if a write operation tried to write bejond the end
         *  of the buffer or for some other failure.
         */
        bool failFlag;

    public:
        /** Construct a new byte stream.
         *
         *  @param bufVal The underlaying memory for the output.
         */
        SimpleOutStream();

        /** Returns the buffer */
        const std::span<uint8_t>& buffer() const {
            return buf;
        }

        /** Returns true if we can't write another byte (because the buffer is
         *  at it's end.  */
        bool eof() const {
            return indexWrite >= buf.size();
        }

        /** Returns if a write operation tried to write bejond the end
         *  of the buffer.
         */
        bool fail() const {
            return failFlag;
        }

        /** Returns the current write index in bytes from the start. */
        uint32_t tellg() const {
            return indexWrite;
        }

        /** Set the current write index in bytes from the start. */
        void seekg(uint32_t pos) {
            if (pos > buf.size()) {
                failFlag = true;
            }
            indexWrite = pos;
        }

        /** Write a byte to the stream.  */
        void writeUint8(const uint8_t val);

        /** Write a word (little endian) to the stream.  */
        void writeUint16le(const uint16_t val);

        /** Write a long word (little endian) to the stream.  */
        void writeUint32le(const uint32_t val);

        template <typename T> void write(const T &val) {
            *this << val;
        }
};

SimpleOutStream& operator<<(SimpleOutStream& out, const bool&);
SimpleInStream& operator>>(SimpleInStream& in, bool&);

SimpleOutStream& operator<<(SimpleOutStream& out, const char&);
SimpleInStream& operator>>(SimpleInStream& in, char&);

SimpleOutStream& operator<<(SimpleOutStream& out, const int8_t&);
SimpleInStream& operator>>(SimpleInStream& in, int8_t&);

SimpleOutStream& operator<<(SimpleOutStream& out, const uint8_t&);
SimpleInStream& operator>>(SimpleInStream& in, uint8_t&);

SimpleOutStream& operator<<(SimpleOutStream& out, const uint16_t&);
SimpleInStream& operator>>(SimpleInStream& in, uint16_t&);

SimpleOutStream& operator<<(SimpleOutStream& out, const uint32_t&);
SimpleInStream& operator>>(SimpleInStream& in, uint32_t&);

SimpleOutStream& operator<<(SimpleOutStream& out, const uint64_t&);
SimpleInStream& operator>>(SimpleInStream& in, uint64_t&);

SimpleOutStream& operator<<(SimpleOutStream& out, const float&);
SimpleInStream& operator>>(SimpleInStream& in, float&);

SimpleOutStream& operator<<(SimpleOutStream& out, const rcSignals::RcSignal&);
SimpleInStream& operator>>(SimpleInStream& in, rcSignals::RcSignal&);

SimpleOutStream& operator<<(SimpleOutStream& out, const rcSignals::SignalType&);
SimpleInStream& operator>>(SimpleInStream& in, rcSignals::SignalType&);

SimpleOutStream& operator<<(SimpleOutStream& out, const rcAudio::SampleData&);
SimpleInStream& operator>>(SimpleInStream& in, rcAudio::SampleData&);

SimpleOutStream& operator<<(SimpleOutStream& out, const rcAudio::Volume&);
SimpleInStream& operator>>(SimpleInStream& in, rcAudio::Volume&);

SimpleOutStream& operator<<(SimpleOutStream& out, const rcOutput::FreqType&);
SimpleInStream& operator>>(SimpleInStream& in, rcOutput::FreqType&);

namespace rcEngine {
    SimpleOutStream& operator<<(::SimpleOutStream& out, const GearCollection&);
    SimpleInStream& operator>>(::SimpleInStream& in, GearCollection&);

    SimpleOutStream& operator<<(::SimpleOutStream& out, const Idle&);
    SimpleInStream& operator>>(::SimpleInStream& in, Idle&);
}

// -- arrays
template <class T, std::size_t N>
SimpleOutStream& operator<<(SimpleOutStream& out, const std::array<T,N>& arr) {
    for (const auto& elem : arr) {
        out << elem;
    }
    return out;
}

template <class T, std::size_t N>
SimpleInStream& operator>>(SimpleInStream& in, std::array<T,N>& arr) {
    for (auto& elem : arr) {
        in >> elem;
    }
    return in;
}

#ifdef ARDUINO
template <std::size_t N>
SimpleOutStream& operator<<(SimpleOutStream& out, const std::array<gpio_num_t,N>& arr) {
    for (const auto& elem : arr) {
        out.writeUint8(static_cast<int8_t>(elem));
    }
    return out;
}

template <std::size_t N>
SimpleInStream& operator>>(SimpleInStream& in, std::array<gpio_num_t,N>& arr) {
    for (auto& elem : arr) {
        elem = static_cast<gpio_num_t>(in.readUint8());
    }
    return in;
}
#endif

#endif // _SIMPLE_BYTE_STREAM_H_
