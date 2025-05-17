/** RC functions controller for Arduino ESP32
 *
 *  Definitions for the sample storage class.
 *
 *  @file
 *
*/

#ifndef _RC_SAMPLE_STORAGE_H_
#define _RC_SAMPLE_STORAGE_H_

#include "sample.h"
#include "audio.h"
#include "flash_sample.h"

#include <span>
#include <vector>

/** This class manages the functions controller audio samples.
 *
 *  The class provides methods to:
 *
 *  - find samples by id
 *  - manages wav to raw conversion
 *  - handles bluetooth commands
 *  - stores and restores dynamic samples form NVM
 *
 */
class SampleStorageSingleton {
    private:
        static constexpr uint8_t CMD_RESET = 0u; ///< this bt command will remove all existing samples
        static constexpr uint8_t CMD_ADD = 1u; ///< Create a new sample that data can be added onto later. You can't create an existing ID
        static constexpr uint8_t CMD_ADD_DATA = 2u;

        FlashSample::SampleStorage flashSampleStorage;

        /** Indicates if we have modified the dynamic files but not
         *  flushed or re-read them.
         */
        mutable bool dynamicDirty;

        /** List of uploades sample files
         */
        mutable std::vector<rcSamples::SampleFile> dynamicFiles;

        /** List of data (matching to the dynamicFiles)
         */
        mutable std::vector<rcAudio::SampleData> dynamicData;

        /** List of data (matching to the staticFiles)
         */
        mutable std::vector<rcAudio::SampleData> staticData;

        /** Constructor. */
        SampleStorageSingleton();

        /** Destructor */
        ~SampleStorageSingleton();

        /** Re-creates dynamic samples lists, filling dynamicFiles and dynamicData */
        void updateDynamic() const;

        /** Returns the index of a static sample
         *
         *  Searches in static samples list and return an index to the audio id.
         *  @returns -1 if the id is not found.
         */
        int32_t getStaticIndex(const rcSamples::AudioId& id) const;

        /** Returns the index of a dynamic sample
         *
         *  Searches in dynamic samples list and return an index to the audio id.
         *  @returns -1 if the id is not found.
         */
        int32_t getDynamicIndex(const rcSamples::AudioId& id) const;

    public:
        static SampleStorageSingleton& getInstance()
        {
            static SampleStorageSingleton instance; // Guaranteed to be destroyed.
                                                    // Instantiated on first use.
            return instance;
        }

        SampleStorageSingleton(SampleStorageSingleton const&) = delete;
        void operator=(SampleStorageSingleton const&) = delete;

        /** Executes the command from data.
         *
         *  This is called when data is received via bluetooth,
         *  specifically the Audio Characteristics.
         */
        void executeCommand(SimpleInStream& in);

        /** Returns the sample file for the audio id.
         *
         *  Searches in static and dynamic samples list.
         *  @returns the requested file or the first one from the static samples
         *    list as a fallback.
         */
        const rcSamples::SampleFile& getSampleFile(const rcSamples::AudioId& id) const;

        /** Returns the sample data for the audio id.
         *
         *  Searches in static and dynamic samples list.
         *  @returns the requested file or the first one from the static samples
         *    list as a fallback.
         */
        const rcAudio::SampleData& getSampleData(const rcSamples::AudioId& id) const;

        /** Returns the audio sample ID to a SampleData
         *
         *  Searches in static and dynamic samples list.
         *  @returns the requested ID or the first static ID as fallback.
         */
        const rcSamples::AudioId& getAudioId(const rcAudio::SampleData& sample) const;

        /** Fills the output buffer with the list of dynamic files.
         */
        void serializeList(SimpleOutStream& out) const;
};


#endif // _RC_SAMPLE_STORAGE_H_
