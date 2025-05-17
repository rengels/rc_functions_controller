/** RC functions controller for Arduino ESP32
 *
 *  Implementation for sample list handling.
 *
 *  @file
*/

#include "sample_storage_singleton.h"
#include "simple_byte_stream.h"
#include "flash_sample.h"
#include "wav_sample.h"

#ifdef HAVE_NV
#include <esp_log.h>
#include <esp_crc.h>

static const char* TAG = "SampleStorage";
#endif

#include <algorithm>
#include <vector>
#include <span>

#define STORAGE_NAMESPACE "sampleStorage"

using namespace rcSamples;



SampleStorageSingleton::SampleStorageSingleton() :
    dynamicDirty(true) {

    // fill static data
    auto staticFiles = rcSamples::getStaticSamples();
    for (const auto& file: staticFiles) {
        staticData.push_back(rcAudio::SampleData(
            getWavSamples(file.content)));
    }
}


SampleStorageSingleton::~SampleStorageSingleton() {
}

void SampleStorageSingleton::updateDynamic() const {

    dynamicFiles.resize(0);
    dynamicData.resize(0);

    // fill dynamic data
    dynamicFiles = flashSampleStorage.getFiles();
    for (const auto& file: dynamicFiles) {
        rcAudio::SampleData sample(getWavSamples(file.content));
#ifdef HAVE_NV
        ESP_LOGI(TAG, "Dynamic sample: %c%c%c size %d.",
            file.id[0], file.id[1], file.id[2], file.content.size());
#endif
        dynamicData.push_back(sample);
    }

    dynamicDirty = false;
}


int32_t SampleStorageSingleton::getStaticIndex(const rcSamples::AudioId& id) const {
    auto staticFiles = rcSamples::getStaticSamples();
    for (size_t i = 0; i < staticFiles.size(); i++) {
        if (staticFiles[i].id == id) {
            return i;
        }
    }
    return -1;
}


int32_t SampleStorageSingleton::getDynamicIndex(const rcSamples::AudioId& id) const {

    if (dynamicDirty) {
        updateDynamic();
    }

    for (size_t i = 0; i < dynamicFiles.size(); i++) {
        if (dynamicFiles[i].id == id) {
            return i;
        }
    }
    return -1;
}


const rcSamples::SampleFile& SampleStorageSingleton::getSampleFile(const rcSamples::AudioId& id) const {

    if (dynamicDirty) {
        updateDynamic();
    }
    for (const rcSamples::SampleFile& file : dynamicFiles) {
        if (file.id == id) {
            return file;
        }
    }

    auto staticFiles = rcSamples::getStaticSamples();
    for (const rcSamples::SampleFile& file : staticFiles) {
        if (file.id == id) {
            return file;
        }
    }

    return staticFiles.front();
}


const rcAudio::SampleData& SampleStorageSingleton::getSampleData(const rcSamples::AudioId& id) const {

    auto index = getDynamicIndex(id);
    if (index >= 0) {
        return dynamicData[index];
    }

    auto staticFiles = rcSamples::getStaticSamples();
    index = getStaticIndex(id);
    if (index >= 0) {
        return staticData[index];
    }

#ifdef HAVE_NV
    ESP_LOGW(TAG, "Sample not found: %c%c%c.\n", id[0], id[1], id[2]);
#endif

    return staticData.front();
}


const rcSamples::AudioId& SampleStorageSingleton::getAudioId(const rcAudio::SampleData& data) const {

    if (dynamicDirty) {
        updateDynamic();
    }
    for (size_t i = 0; i < dynamicData.size(); i++) {
        if (data.data() == dynamicData[i].data()) {
            return dynamicFiles[i].id;
        }
    }

    auto staticFiles = rcSamples::getStaticSamples();
    for (size_t i = 0; i < staticData.size(); i++) {
        if (data.data() == staticData[i].data()) {
            return staticFiles[i].id;
        }
    }

    return staticFiles.front().id;
}


void SampleStorageSingleton::executeCommand(SimpleInStream& in) {

    // check header
    auto b1 = in.read<uint8_t>();
    auto b2 = in.read<uint8_t>();
    auto b3 = in.read<uint8_t>();
    if (b1 != 'R' || b2 != 'A' || b3 != 1) {
#ifdef HAVE_NV
        ESP_LOGW(TAG, "Audio Sample command incorrect header.\n");
#endif
        return;
    }

    auto command = in.readUint8();

    switch (command) {
    case CMD_RESET:
        {
            flashSampleStorage.reset();
            dynamicDirty = true;
        }
        break;
    case CMD_ADD:
        {
            auto id = in.read<rcSamples::AudioId>();
            auto size = in.read<uint32_t>();
#ifdef HAVE_NV
            ESP_LOGI(TAG, "New dynamic sample: %c%c%c size %lu.",
                id[0], id[1], id[2], size);
#endif
            flashSampleStorage.addId(id, size);
            dynamicDirty = true;
        }
        break;
    case CMD_ADD_DATA:
        {
            auto id = in.read<rcSamples::AudioId>();
            auto offset = in.read<uint32_t>();
            auto size = in.read<uint32_t>();
            auto newData = in.buffer().subspan(in.tellg(),
                std::min(
                    static_cast<size_t>(size),
                    static_cast<size_t>(in.buffer().size() - in.tellg())));

#ifdef HAVE_NV
            ESP_LOGI(TAG, "New audio data: offset: %lu size: %lu, %d %d %d.",
                offset, size,
                static_cast<int>(newData.data()[0]),
                static_cast<int>(newData.data()[1]),
                static_cast<int>(newData.data()[2])
                );
#endif

            flashSampleStorage.setData(id, offset, newData);
            dynamicDirty = true;
        }
        break;
    default:
        ; // nothing to do
    }
}

void SampleStorageSingleton::serializeList(SimpleOutStream& out) const {

    if (dynamicDirty) {
        updateDynamic();
    }

    // write header
    out.writeUint8('R');
    out.writeUint8('L');
    out.writeUint8(1U);  // binary format version
    out.write<uint16_t>(flashSampleStorage.sectorsUsed());
    out.write<uint16_t>(flashSampleStorage.sectorsFree());
    out.write<uint8_t>(dynamicFiles.size());
    for (const auto& file : dynamicFiles) {
        uint16_t crc = 0u;

#ifdef HAVE_NV
        crc = esp_crc16_le(UINT16_MAX, &(*file.content.begin()), file.content.size());
#endif

        out << file.id << static_cast<uint32_t>(file.content.size()) << crc;
    }
}

