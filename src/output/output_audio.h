/**
 *  Implementation of the output via DAC
 *
 *  @file
*/

#ifndef _OUTPUT_AUDIO_H_
#define _OUTPUT_AUDIO_H_

#include "output.h"

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/dac_continuous.h>

namespace rcOutput {

/** Concrete implementation of the Output interface for audio.
 *
 *  It uses:
 *
 *  - GPIO_25
 *  - GPIO_26
 *  - DAC1
 *  - DAC2
 *  - TIMER_GROUP_1
 *  - TIMER_0
 *
 *  This is a digital output capable of pwm duty cycle control.
 *  It uses different output pins and one timer module.
 *
 *  @see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/dac.html
 */
class OutputAudio : public Output {
    public:
        static constexpr uint32_t SAMPLE_RATE = 22050;
    private:
        dac_continuous_handle_t handleDac;
        QueueHandle_t handleQueue;

    public:
        OutputAudio();
        ~OutputAudio();

        /** Checks for finished DMA jobs and starts new ones.
         *
         * The AudioRingbuffer is filled by the audio procs in
         * by their own step() functions.
         *
         * The sample output is done a continous DMA job.
         */
        virtual void step(const rcProc::StepInfo& info) override;
        virtual void start();
        virtual void stop();

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const OutputAudio&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, OutputAudio&);
};


} // namespace

#endif  // _OUTPUT_AUDIO_H_
