/** RC functions controller for Arduino ESP32
 *
 *  Definitions for the proc storage class.
 *
 *  @file
 *
*/

#ifndef _RC_PROC_STORAGE_H_
#define _RC_PROC_STORAGE_H_

#include "signals.h"
#include "sample.h"
#include <vector>

namespace rcProc {
class StepInfo;
class Proc;
}

class SimpleInStream;
class SimpleOutStream;

/** This class manages the functions controller configuration.
 *
 *  The class provides methods to:
 *
 *  - execute the step functions of the Proc modules.
 *  - serialize to binary stream
 *  - deserialize from binary stream
 */
class ProcStorage {
    private:

        /** List of all procs.
         *
         *  Storage manages the lists of procs, meaning processing
         *  nodes for the signals.
         *
         *  Note: not using unique_ptr here. Content has to be deleted.
         */
        std::vector<rcProc::Proc*> procs;

        /** Removes all procs from the procs vector and frees their memory.
         */
        void clear();

        /** Serializes a proc to the output stream.
         *
         *  This function is created by a python script out of the proc
         *  configuration file.
         *
         *  Look for it in serialize.cpp.
         */
        void serializeProc(SimpleOutStream& out, const rcProc::Proc& proc) const;

        /** Deserializes a proc to the output stream.
         *
         *  This function is created by a python script out of the proc
         *  configuration file.
         *
         *  Look for it in serialize.cpp.
         */
        rcProc::Proc* deserializeProc(SimpleInStream& in);

        /** Returns the sample file for the audio id.
         *
         *  Searches in static and dynamic samples list.
         *  @returns the requested file or the first one from the static samples
         *    list as a fallback.
         */
        const rcSamples::SampleFile& getSampleFile(const rcSamples::AudioId& id) const;

        /** Creates a default configuration, adding some example procs */
        void createDefaultConfig();

        /** Adds procs for a steam train engine and sound.
         *
         *  The sound and statistics are from a 4-6-0 Hall class locomotive.
         */
        void vehicleSteamTrain();

        /** Adds procs for an old diesel driven ship plus sound.
         *
         *  This simulates an old one stroke motor with a clutch.
         *  Also included are wave sounds and a ship horn.
         */
        void vehicleShip();

        void vehicleTruck();

        /** Example configuration for a 1974 VW beetle
         *
         *  Corner values:
         *
         *  32kW, idle: 875RPM, max: 4200RPM
         *  weight: 800 kg
         *
         *  transmission:
         *  - 1. Gang= 3,80 (38:10)
         *  - 2. Gang= 2,06 (35:17)
         *  - 3. Gang= 1,26:1 (29:33)
         *  - R-Gang 3,88:1.
         *
         *
         */
        void vehicleCar();

    public:

        /** Constructor.
         *
         *  The resulting storage contains a default set of procs.
         */
        ProcStorage();
        ~ProcStorage();

        /** Calls start() for all the procs. */
        void start();

        /** Calls stop() for all the procs. */
        void stop();

        /** Applies the signal transformation for all the *procs*.
         *
         *  This function needs to be called sufficiently often
         *  for short effects to work correctly.
         *  e.g. 15-30 ms.
         *
         *  @param info The input/output signals.
         */
        void step(const rcProc::StepInfo& info);

        /** Tries to load the configuration from non volatile memory (flash) */
        void loadFromNvm();

        /** Tries to save the configuration from non volatile memory (flash) */
        void saveToNvm() const;


        /** Serialize the Proc to a SimpleByteStream.
         *
         *  This stores all configuration data in the stream.
         *
         *  @param last The last index of the procs that should be serialized (exclusive)
         */
        void serialize(SimpleOutStream& out) const;

        /** Deserialize the Proc to a SimpleByteStream.
         *
         *  This replaces all elements from *first* to *last* with
         *  configuration data from the stream.
         *
         *  @returns false if deserialization didn't work
         */
        bool deserialize(SimpleInStream& in);
};


#endif // _RC_PROC_STORAGE_H_
