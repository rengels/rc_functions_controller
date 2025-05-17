/** This file contains a simple implementation of a byte stream.
 *
 *  @file
 */

#include "simple_byte_stream.h"
#include "sample_storage_singleton.h"
#include "signals.h"

#include "engine_gear.h"  // for GearCollection

#include <cstdlib>
#include <algorithm>  // clamp

using namespace rcSignals;

SimpleInStream::SimpleInStream(const std::span<const uint8_t>& bufVal) :
    indexRead(0u),
    buf(bufVal),
    failFlag(false)
{}

uint8_t SimpleInStream::readUint8() {
    if (eof()) {
        failFlag = true;
        return 0u;
    } else {
        return buf[indexRead++];
    }
}

uint16_t SimpleInStream::readUint16le() {

    uint16_t val = 0u;
    val |= (readUint8() << 0);
    val |= (readUint8() << 8);

    if (fail()) {
        val = 0;
    }

    return val;
}

uint32_t SimpleInStream::readUint32le() {

    uint32_t val = 0u;
    val |= (readUint8() << 0);
    val |= (readUint8() << 8);
    val |= (readUint8() << 16);
    val |= (readUint8() << 24);

    if (fail()) {
        val = 0;
    }

    return val;
}



SimpleOutStream::SimpleOutStream() :
    indexWrite(0),
    buf(static_cast<uint8_t*>(nullptr), 0),
    failFlag(false)
{
    // allocate an initial memory segment
    uint8_t* data = static_cast<uint8_t*>(malloc(32));
    if (data) {
        buf = {data, 32};
    }
}

void SimpleOutStream::writeUint8(const uint8_t val) {
    if (eof()) {
        // try to realloc
        size_t newSize = buf.size() + buf.size() / 2;
        uint8_t* newData = static_cast<uint8_t*>(realloc(
            buf.data(),
            newSize));

        if (!newData) { // realloc failed
            failFlag = true;
            return;
        }

        buf = {newData, newSize};
    }

    buf[indexWrite++] = val;
}

void SimpleOutStream::writeUint16le(const uint16_t val) {

    writeUint8(static_cast<uint8_t>((val >> 0) & 0xFF));
    writeUint8(static_cast<uint8_t>((val >> 8) & 0xFF));
}

void SimpleOutStream::writeUint32le(const uint32_t val) {

    writeUint8(static_cast<uint8_t>((val >> 0) & 0xFF));
    writeUint8(static_cast<uint8_t>((val >> 8) & 0xFF));
    writeUint8(static_cast<uint8_t>((val >> 16) & 0xFF));
    writeUint8(static_cast<uint8_t>((val >> 24) & 0xFF));
}

SimpleOutStream& operator<<(SimpleOutStream& out, const bool& val) {
    out.writeUint8(static_cast<uint8_t>(val));
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, bool& val) {
    val = static_cast<bool>(in.readUint8());
    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const int8_t& val) {
    out.writeUint8(static_cast<uint8_t>(val));
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, int8_t& val) {
    val = static_cast<uint8_t>(in.readUint8());
    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const char& val) {
    out.writeUint8(static_cast<uint8_t>(val));
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, char& val) {
    val = static_cast<uint8_t>(in.readUint8());
    return in;
}


SimpleOutStream& operator<<(SimpleOutStream& out, const uint8_t& val) {
    out.writeUint8(val);
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, uint8_t& val) {
    val = in.readUint8();
    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const uint16_t& val) {

    out.writeUint8(static_cast<uint8_t>((val >> 8) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 0) & 0xFF));

    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, uint16_t& val) {

    val = 0u;
    val |= (in.readUint8() << 8);
    val |= (in.readUint8() << 0);

    if (in.fail()) {
        val = 0;
    }

    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const uint32_t& val) {

    out.writeUint8(static_cast<uint8_t>((val >> 24) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 16) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 8) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 0) & 0xFF));

    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, uint32_t& val) {

    val = 0u;
    val |= (in.readUint8() << 24);
    val |= (in.readUint8() << 16);
    val |= (in.readUint8() << 8);
    val |= (in.readUint8() << 0);

    if (in.fail()) {
        val = 0;
    }

    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const uint64_t& val) {

    out.writeUint8(static_cast<uint8_t>((val >> 56) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 48) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 40) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 32) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 24) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 16) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 8) & 0xFF));
    out.writeUint8(static_cast<uint8_t>((val >> 0) & 0xFF));

    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, uint64_t& val) {

    val = 0u;
    val |= (static_cast<uint64_t>(in.readUint8()) << 56);
    val |= (static_cast<uint64_t>(in.readUint8()) << 48);
    val |= (static_cast<uint64_t>(in.readUint8()) << 40);
    val |= (static_cast<uint64_t>(in.readUint8()) << 32);
    val |= (static_cast<uint64_t>(in.readUint8()) << 24);
    val |= (static_cast<uint64_t>(in.readUint8()) << 16);
    val |= (static_cast<uint64_t>(in.readUint8()) << 8);
    val |= (static_cast<uint64_t>(in.readUint8()) << 0);

    if (in.fail()) {
        val = 0;
    }

    return in;
}


SimpleOutStream& operator<<(SimpleOutStream& out, const float& val) {
    // dirty. aliasing is usually forbidden.
    const float* pVal = &val;
    const uint32_t* pU32 = static_cast<const uint32_t*>(static_cast<const void*>(pVal));

    return out << (*pU32);
}

SimpleInStream& operator>>(SimpleInStream& in, float& val) {
    // dirty. aliasing is usually forbidden.
    uint32_t u32;
    in >> u32;

    *(static_cast<uint32_t*>(static_cast<void *>(&val))) = u32;
    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const rcSignals::RcSignal& val) {
    return out << static_cast<uint16_t>(val);
}

SimpleInStream& operator>>(SimpleInStream& in, rcSignals::RcSignal& val) {
    uint16_t u16;
    in >> u16;
    if (in.fail()) {
        val = RCSIGNAL_INVALID;
    } else {
        val = static_cast<rcSignals::RcSignal>(u16);
    }
    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const rcSignals::SignalType& val) {
    return out << static_cast<uint8_t>(val);
}

SimpleInStream& operator>>(SimpleInStream& in, rcSignals::SignalType& val) {
    uint8_t u8;
    in >> u8;
    val = static_cast<rcSignals::SignalType>(u8);
    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const rcAudio::SampleData& data) {

    const SampleStorageSingleton& ss = SampleStorageSingleton::getInstance();
    auto id = ss.getAudioId(data);

    out << id;
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, rcAudio::SampleData& data) {

    const SampleStorageSingleton& ss = SampleStorageSingleton::getInstance();
    auto id = in.read<rcSamples::AudioId>();

    data = ss.getSampleData(id);
    return in;
}

namespace rcEngine {

/** The gear collection output operator.
 *
 *  Gears can be a little bit of a space hog. We have up to 20 floats.
 *  Therefore we are writing a dynamic list with ratios encoded as a
 *  fixed point number (int8_t / 10)
 */
SimpleOutStream& operator<<(SimpleOutStream& out, const GearCollection& gears) {

    out.write<int8_t>(gears.size());
    for (int8_t i = 0; i < gears.size(); i++) {
        out.write<int8_t>(
            std::clamp(gears.get(i) * 10.0f, -127.0f, 127.0f));
    }
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, GearCollection& gears) {

    int8_t num = in.read<int8_t>();

    std::array<float, rcEngine::GearCollection::NUM_GEARS> ratios = {0.0f};
    for (uint8_t i = 0; i < num && i < ratios.size(); i++) {
        ratios[i] = in.read<int8_t>() / 10.0f;
    }
    gears.set(ratios);
    return in;
}


SimpleOutStream& operator<<(SimpleOutStream& out, const Idle& idle) {
    out << idle.rpmIdleStart << idle.rpmIdleRunning << idle.loadStart << idle.timeStart << idle.throttleStep;
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, Idle& idle) {
    in >> idle.rpmIdleStart >> idle.rpmIdleRunning >> idle.loadStart >> idle.timeStart >> idle.throttleStep;
    return in;
}
}  // end namespace


SimpleOutStream& operator<<(SimpleOutStream& out, const rcAudio::Volume& val) {
    return out << static_cast<uint8_t>(val.value * 100.0f);
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, rcAudio::Volume& val) {
    uint8_t u8;
    in >> u8;
    val.value = u8 / 100.0f;
    return in;
}

SimpleOutStream& operator<<(SimpleOutStream& out, const rcOutput::FreqType& val) {
    return out << static_cast<uint8_t>(val);
    return out;
}

SimpleInStream& operator>>(SimpleInStream& in, rcOutput::FreqType& val) {
    uint8_t u8;
    in >> u8;
    val = static_cast<rcOutput::FreqType>(u8);
    return in;
}

