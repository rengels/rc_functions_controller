/** RC functions controller for Arduino ESP32
 *
 *  Implementation for flash handling for dynamic samples
 *
 *  @file
*/

#include "flash_sample.h"

#ifdef HAVE_NV
#include <esp_log.h>
// #include <nvs.h>
#include <nvs_flash.h>
#include <esp_flash.h>
#include <spi_flash_mmap.h>

static const char* TAG = "FlashSample";
#endif

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

using namespace FlashSample;

FlashSingleton::FlashSingleton() :
#ifdef HAVE_NV
    part(nullptr),
    mapHandle(0),
#endif
    mapPtr(nullptr),
    maxSectors(0),
    sectorBufferIndex(0) {

#ifdef HAVE_NV
    part = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_ANY,
            "samples");

    if (part) {
        maxSectors = part->size / SPI_FLASH_SEC_SIZE;
        sectorBufferIndex = maxSectors;

        // map the partition to data memory
        const void* maddr = nullptr;
        ESP_ERROR_CHECK(
            esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA,
                &maddr,
                &mapHandle));
        mapPtr = static_cast<const uint8_t*>(maddr);

    } else {
        ESP_LOGW(TAG, "Can't find the audio partition, please define it correctly in `partitions.csv`");
    }

#else
    // for unit tests we just allocate some memory
    maxSectors = 10;
    mapPtr = new uint8_t[maxSectors * SPI_FLASH_SEC_SIZE];
#endif

}

FlashSingleton::~FlashSingleton() {

    flush();

#ifdef HAVE_NV
    if (mapHandle != 0) {
        esp_partition_munmap(mapHandle);
        mapHandle = 0;
    }
#else
    delete[] mapPtr;
#endif
}


void FlashSingleton::flush() {

    if (sectorBufferIndex >= maxSectors) {
        // no buffer to flush
        return;
    }

#ifdef HAVE_NV
    if (part != nullptr) {
        ESP_ERROR_CHECK(
            esp_flash_erase_region(
                part->flash_chip,
                part->address + sectorBufferIndex * SPI_FLASH_SEC_SIZE,
                SPI_FLASH_SEC_SIZE));

        ESP_ERROR_CHECK(
            esp_flash_write(
                part->flash_chip,
                sectorBuffer.data(),
                part->address + sectorBufferIndex * SPI_FLASH_SEC_SIZE,
                SPI_FLASH_SEC_SIZE));
    }
#else
    std::copy(sectorBuffer.begin(),
        sectorBuffer.end(),
        const_cast<uint8_t*>(mapPtr) + sectorBufferIndex * SPI_FLASH_SEC_SIZE);
#endif
    sectorBufferIndex = maxSectors;
}


uint32_t FlashSingleton::setData(uint16_t index, uint32_t offset, const std::span<const uint8_t>& data) {

    // sanity handling for too large offset
    while (offset > SPI_FLASH_SEC_SIZE) {
        index++;
        offset -= SPI_FLASH_SEC_SIZE;
    }

    // check out of bounds
    if (index >= maxSectors) {
        return 0;
    }

#ifdef HAVE_NV
    // check flash partition available
    if (part == nullptr) {
        return 0;
    }
#endif

    // need to flush the old buffer
    if (index != sectorBufferIndex) {
        flush();

        // read the current flash content
        memcpy(sectorBuffer.data(), this->data(index), sizeof(sectorBuffer));
        sectorBufferIndex = index;
    }

    // this is how much we have to write
    uint32_t count = std::min(
        static_cast<uint32_t>(SPI_FLASH_SEC_SIZE - (offset % SPI_FLASH_SEC_SIZE)),
        static_cast<uint32_t>(data.size()));


    std::copy(data.begin(), data.begin() + count, sectorBuffer.data() + offset);

    return count;
}


const void* FlashSingleton::data(uint16_t index) const {
    if (mapPtr != nullptr && index < maxSectors) {
        return mapPtr + index * SPI_FLASH_SEC_SIZE;
    } else {
        return nullptr;
    }
}


uint16_t FlashSingleton::getIndex(const void* data) const {
    if (mapPtr != nullptr) {
        return (static_cast<const uint8_t*>(data) - mapPtr) / SPI_FLASH_SEC_SIZE;
    } else {
        return maxSectors;
    }
}


void FlashSingleton::reset(uint16_t index) {

    // to invalidate a sector we zero out the first bytes
    // this saves an erasing step
    uint64_t zeros = 0u;

#ifdef HAVE_NV
    if (part != nullptr) {
        ESP_ERROR_CHECK(
            esp_flash_write(
                part->flash_chip,
                &zeros,
                part->address + index * SPI_FLASH_SEC_SIZE,
                sizeof(zeros)));
    }
#else
    std::copy(&zeros,
        &zeros + sizeof(zeros),
        const_cast<uint8_t*>(mapPtr) + index * SPI_FLASH_SEC_SIZE);
#endif
}


// --------- SampleBlock

const SampleBlock* SampleBlock::first() {
    FlashSingleton& flash = FlashSingleton::getInstance();
    const SampleBlock* result = static_cast<const SampleBlock*>(flash.data(0));

    if (result->magic != MAGIC) {
        result = nullptr;
    };

    return result;
}


const SampleBlock* SampleBlock::addBlock(const rcSamples::AudioId& id, uint32_t size) {
    FlashSingleton& flash = FlashSingleton::getInstance();

    // -- find the first empty sector
    uint16_t sectorIndex = 0;
    const SampleBlock* block = first();

    // iterate (and increase sectorIndex) until we can't find a
    // valid block any more
    while ((block != nullptr) &&
           (block->magic == MAGIC)) {
        sectorIndex += block->numSectors;
        block = block->next();
    }


    uint16_t numSectors =
        (sizeof(SampleBlock) + size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;
    // check if block fits
    if (sectorIndex + numSectors >= flash.getMaxSectors()) {
        return nullptr;
    }

    // -- write the new block to flash
    SampleBlock newBlock = {
        .magic = MAGIC,
        .numSectors = numSectors,
        .id = id,
        .size = size
    };
    std::span<const uint8_t> span(
            static_cast<const uint8_t*>(static_cast<const void*>(&newBlock)), sizeof(SampleBlock));
    auto written = flash.setData(sectorIndex, 0, span);

    // -- return reference to new block
    if (written > 0) { // if no byte was written, something went wrong
        flash.flush(); // so we can immediately use the block
        return static_cast<const SampleBlock*>(flash.data(sectorIndex));
    } else {
        return nullptr;
    }
}

const SampleBlock* SampleBlock::next() const {
    FlashSingleton& flash = FlashSingleton::getInstance();

    // check if we are a valid block ourselves
    if (this->magic != MAGIC) {
        return nullptr;
    }

    const uint16_t sectorIndex = flash.getIndex(this) + numSectors;
    if (sectorIndex >= flash.getMaxSectors()) {
        return nullptr;
    }

    const SampleBlock* block = static_cast<const SampleBlock*>(
        flash.data(sectorIndex));

    if ((block == nullptr) ||
        (block->magic != MAGIC)) {
        return nullptr;
    }

    return block;
}

void SampleBlock::setData(uint32_t offset, std::span<const uint8_t> data) const {
    FlashSingleton& flash = FlashSingleton::getInstance();

    // sanity check, does the data fit into the blocks
    if (offset + data.size() > maxSize()) {
        return;
    }

    offset += sizeof(SampleBlock);
    const uint8_t* basePtr = static_cast<const uint8_t*>(static_cast<const void*>(this));

    while (data.size() > 0) {
        uint16_t sectorIndex = flash.getIndex(basePtr + offset);
        const auto written = flash.setData(sectorIndex, offset % SPI_FLASH_SEC_SIZE, data);

        // if we can't write, we might as well finish
        if (written == 0) {
            break;
        }
        offset += written;
        data = data.subspan(written, data.size() - written);
    }
}

/** Go through all the blocks (starting with first) and call FlashSingleton::reset() for the
 *  first sector.
 */
void SampleBlock::reset() {
    FlashSingleton& flash = FlashSingleton::getInstance();

    const SampleBlock* block = first();

    while ((block != nullptr) &&
           (block->magic == MAGIC)) {

        uint16_t sectIndex = flash.getIndex(block);
        block = block->next();
        flash.reset(sectIndex);
    }
}

const uint8_t* SampleBlock::data() const {
    return static_cast<const uint8_t*>(static_cast<const void*>(this)) + sizeof(SampleBlock);
}

uint32_t SampleBlock::maxSize() const {
    return numSectors * SPI_FLASH_SEC_SIZE - sizeof(SampleBlock);
}

rcSamples::SampleFile SampleBlock::getFile() const {
    return rcSamples::SampleFile(
        id,
        std::span<const uint8_t>(
            static_cast<const uint8_t*>(data()), size));
}

// ---------- SampleStorage

void SampleStorage::readFromFlash() {

    sampleBlocks.resize(0);
    for (const SampleBlock* block = SampleBlock::first();
            block != nullptr;
            block = block->next()) {
        sampleBlocks.push_back(block);
    }
}

void SampleStorage::reset() {
    SampleBlock::reset();
    sampleBlocks.resize(0);
}

std::vector<rcSamples::SampleFile> SampleStorage::getFiles() const {

    FlashSingleton& flash = FlashSingleton::getInstance();
    flash.flush();

    std::vector<rcSamples::SampleFile> result;
    for (const auto& block: sampleBlocks) {
        result.push_back(block->getFile());
    }
    return result;
}

bool SampleStorage::addId(const rcSamples::AudioId& id, uint32_t size) {

    // check if we already have such an ID
    for (const auto& block: sampleBlocks) {
        if (id == block->id) {
            return false;
        }
    }

    const SampleBlock* newBlock = SampleBlock::addBlock(id, size);
    if (newBlock != nullptr) {
        sampleBlocks.push_back(newBlock);
        return true;

    } else {
        return false;
    }
}

void SampleStorage::setData(const rcSamples::AudioId& id, uint32_t offset, const std::span<const uint8_t>& data) {
    for (const auto& block: sampleBlocks) {
        if (id == block->id) {
            block->setData(offset, data);
            return;
        }
    }
}

uint16_t SampleStorage::sectorsFree() const {

    FlashSingleton& flash = FlashSingleton::getInstance();
    return flash.getMaxSectors() - sectorsUsed();
}

uint16_t SampleStorage::sectorsUsed() const {
    FlashSingleton& flash = FlashSingleton::getInstance();

    if (sampleBlocks.size() == 0) {
        return 0;
    } else {
        return flash.getIndex(sampleBlocks.back()) + sampleBlocks.back()->numSectors;
    }
}

