/**
 *  This file contains structures and types that are used
 *  with the rc_functions_controller project.
 *
 *  @file
*/

#ifndef _RC_SIGNALS_H_
#define _RC_SIGNALS_H_

#include <cstdint>
#include <limits>
#include <cassert>
#include <array>

#include "signal_types.h"

/** Namespace for rc controller components and types.
 *
 */
namespace rcSignals {

/** Strong Type for the system ms counter.
 *
 *  Will wrap around after 5 days.
 */
typedef uint32_t TimeMs;

/** Type for most rc signals inside the project.
 *
 * This type is used for both the raw input signals and
 * also for the processed ones.
 *
 * We are mapping all input signals into the range -1000/0/1000.
 * For a "classic" pwm signal that means a resolution of 0.5us
 *
 * A 0 indicates a neutral signal.
*/
typedef int16_t RcSignal;

static const RcSignal RCSIGNAL_NEUTRAL = static_cast<RcSignal>(0);

/** Invalid signal.
 *
 *  This value is used whenever the input was not received (or not received
 *  for some time).
 *  It also represents any signal with an indetermined value.
 */
static const RcSignal RCSIGNAL_INVALID = std::numeric_limits<int16_t>::min();

/** Maximum nominal signal.
 *
 *  This represents the maximums signal, but all transmitters are
 *  exceeding this in various amounts. 1100 is not uncommon.
 */
static const RcSignal RCSIGNAL_MAX = static_cast<RcSignal>(1000);

/** Minimum nominal signal.
 */
static const RcSignal RCSIGNAL_MIN = static_cast<RcSignal>(-1000);

/** Limit between a _true_ and a _false_ signal.
 *
 *  This is used many logic conditions, e.g. ST_IGNITION.
 */
static const RcSignal RCSIGNAL_TRUE = static_cast<RcSignal>(300);

/** Limit between a nominal neutral signal.
 *
 *  This is used when determining if a signal is considered "neutral".
 *  e.g. in the engine logic to determine if throttle is "off".
 */
static const RcSignal RCSIGNAL_EPSILON = static_cast<RcSignal>(20);

/** This struct represents the preprocessed *RawSignals*.
 *
 *  This struct is more suitable for output since it has
 *  a standard format.
 *
 *  The list contains three synthetic signals that can be used
 *  for mappings.
 *
 *  For boolean signals (e.g. indicator) the signal is set to MAX_RCSIGNAL
 */
struct Signals {
    static const uint8_t NUM_SIGNALS = static_cast<uint8_t>(SignalType::ST_NUM); ///< the maximum count in the signals struct. Should match *SignalType*

public:
    /** The array containing the actual signals.
     *
     *  For convinience the [] operator is overloaded to be able to
     *  access the signals easier.
     */
    std::array<RcSignal, NUM_SIGNALS> signals;


    /** Resets (invalidates) all signals */
    void reset() {
        for (auto& signal : signals) {
            signal = RCSIGNAL_INVALID;
        }
    }

    /** Subscript operator overloading with direct SignalType.
     */
    RcSignal& operator[](SignalType type) {
        uint8_t index = static_cast<uint8_t>(type);
        assert(index >=0 && index < signals.size());
        return signals[index];
    }

    /** Subscript operator overloading with direct SignalType.
     */
    const RcSignal& operator[](SignalType type) const {
        uint8_t index = static_cast<uint8_t>(type);
        assert(index >=0 && index < signals.size());
        return signals[index];
    }

    /** Returns the signal or the default if the signal is invalid.
     */
    RcSignal get(const SignalType type, const RcSignal def = RCSIGNAL_INVALID) const {
        RcSignal val = (*this)[type];
        if (val == RCSIGNAL_INVALID) {
            return def;
        } else {
            return val;
        }
    }

    /** Sets the signal but only if it's invalic.
     *
     *  This function in procs since they should be careful
     *  overwrite each others output.
     */
    void safeSet(const SignalType type, const RcSignal value) {
        RcSignal &val = (*this)[type];
        if (val == RCSIGNAL_INVALID) {
            val = value;
        }
    }
};

} // namespace

#endif // _RC_SIGNALS_H_
