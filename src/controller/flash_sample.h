/** RC functions controller for Arduino ESP32
 *
 *  Definitions for handling a special flash partition
 *  to store dynamic samples
 *
 *  @file
*/

#ifndef _RC_FLASH_SAMPLE_H_
#define _RC_FLASH_SAMPLE_H_

#include "sample.h"
#include "audio.h"

#ifdef HAVE_NV
// #include <nvs.h>
#include <nvs_flash.h>
#include <spi_flash_mmap.h>
#else
#define SPI_FLASH_SEC_SIZE 4096
#endif

#include <array>
#include <cstdint>
#include <span>
#include <vector>

/** Namespace contains the block managing code for flash storage of
 *  dynamic samples */
namespace FlashSample {

/** Low level flash access wrapper.
 *
 *  All writes are immediate.
 */
class FlashSingleton {
    private:
#ifdef HAVE_NV
        /** The partition used for storing the audio samples. */
        const esp_partition_t* part;

        /** Handle for memory mapping flash to data memory */
        esp_partition_mmap_handle_t mapHandle;
#endif
        const uint8_t* mapPtr;

        /** Maximum number of available sectors for flash content */
        uint16_t maxSectors;

        /** Index of the sector in the sectorBuffer.
         *
         *  When we are writing partial sectors, we store the partial
         *  data in the sectorBuffer.
         *  This variable indicates the sector index for the data
         *  contained in the sectorBuffer.
         *
         *  maxSectors if unused.
         */
        uint16_t sectorBufferIndex;
        std::array<uint8_t, SPI_FLASH_SEC_SIZE> sectorBuffer;

        /** Constructor */
        FlashSingleton();

        /** Destructor */
        ~FlashSingleton();

    public:
        static FlashSingleton& getInstance()
        {
            static FlashSingleton instance; // Guaranteed to be destroyed.
                                            // Instantiated on first use.
            return instance;
        }

        FlashSingleton(FlashSingleton const&) = delete;
        void operator=(FlashSingleton const&) = delete;

        /** Ensures that the data buffered in "sectorBuffer" is
         *  written to flash.
         */
        void flush();

        /** Write the sector with the given index.
         *
         *  Add data to an id in the list of dynamic files
         *
         *  The data migh only actually be written when calling "flush()"
         *
         *  @param index The sector index
         *  @param offset The offset of the data inside the sector
         *  @param data A span of data. If size is too large, then only the
         *    part of the data that fit's into the sector is written.
         *
         *  @param returns the number of bytes copied
         */
        uint32_t setData(uint16_t index, uint32_t offset, const std::span<const uint8_t>& data);

        /** Get a pointer to the memory mapped location of the specified sector */
        const void* data(uint16_t index) const;

        /** Get the sector index for a given data pointer.
         *
         *  @returns maxSectors if the data is not inside the covered memory location.
         */
        uint16_t getIndex(const void* data) const;

        uint16_t getMaxSectors() const {
            return maxSectors;
        }

        /** Resets the flash memory behind the block. */
        void reset(uint16_t index);
};

/** A block of one or more sectors in the flash.
 *
 *  Instances of SampleBlock are memory mapped directly from flash.
 *  The data of this block directly succeeds this blocks instance members.
 */
struct SampleBlock {
    static constexpr uint32_t MAGIC = 0xABCDu;

    uint32_t magic; ///< an indicator that the block header is actually valid. 0xABCD
    uint16_t numSectors;  ///< number of sector in this block

    /** The audio ID contained within the block (and the following ones). */
    rcSamples::AudioId id;

    uint32_t size; ///< sample size

    /** Returns the first block in flash
     *
     *  @returns nullptr if there is no such block.
     */
    static const SampleBlock* first();

    /** Returns a block after the last valid block.
     *
     *  Flash::setData() for this block has already been called to
     *  set magic, numSectors and audio ID.
     *
     *  @returns nullptr if there is no space available or something else went wrong.
     */
    static const SampleBlock* addBlock(const rcSamples::AudioId& id, uint32_t size);

    /** Returns the Block following this one.
     *
     *  @returns nullptr if there is no such block.
     */
    const SampleBlock* next() const;

    /** Set data in the sample Block. */
    void setData(uint32_t offset, std::span<const uint8_t> data) const;

    /** Reset all blocks.
     *
     *  This will overwrite all block headers, invalidating the magic
     *  numbers.
     */
    static void reset();

    /** Get a pointer to the memory mapped location of this block (excluding the header.*/
    const uint8_t* data() const;

    /** Total available size for this block (excluding the header. */
    uint32_t maxSize() const;

    /** Returns an audio SampleFile pointing to the data inside this block. */
    rcSamples::SampleFile getFile() const;
};


/** Internal implementation of a linked list mapped to flash memory.
 *
 *  This class handles the structure of the dynamic samples,
 *  which is a linked list of blocks.
 *
 *  It also marks the sectors and places them in the correct
 *  position in memory.
 */
class SampleStorage {
    private:
        /** List of SampleBlocks (the dynamic samples
        */
        std::vector<const SampleBlock*> sampleBlocks;

        /** Reads all blocks from flash memory into the sampleBlocks vector. */
        void readFromFlash();

    public:
        SampleStorage() {
            readFromFlash();
        }

        /** Reset all blocks */
        void reset();

        /** Returns a list of all the files in flash.
         */
        std::vector<rcSamples::SampleFile> getFiles() const;

        /** Add an id to the list of dynamic files.
         *
         *  @param[in] The audio ID to create.
         *
         *  @returns Returns false if the ID cannot be generated, e.g.
         *    because it already exists.
         */
        bool addId(const rcSamples::AudioId& id, uint32_t size);

        /** Add data to an id in the list of dynamic files
         *
         *  @param[in] The audio ID to add data to.
         */
        void setData(const rcSamples::AudioId& id, uint32_t offset, const std::span<const uint8_t>& data);

        /** Number of unused sectors. */
        uint16_t sectorsFree() const;

        /** Number of used sectors. */
        uint16_t sectorsUsed() const;
};

} // namespace

#endif // _RC_FLASH_SAMPLE_H_
