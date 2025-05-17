/**
 *  Implementation for the output class
 *
 *  @file
*/

#include "output.h"

#include <cstdint>
#include <array>

namespace rcOutput {

std::array<bool, 3> Output::freeTimerGroupIds = {true, true, true};

uint8_t Output::reserveTimerGroupId() {
    for (uint8_t i = 0; i < freeTimerGroupIds.size(); i++) {
        if (freeTimerGroupIds[i]) {
            freeTimerGroupIds[i] = false;
            return i;
        }
    }
    return 255u;
}

void Output::freeTimerGroupId(uint8_t id) {
    freeTimerGroupIds[id] = true;
}

} // namespace

