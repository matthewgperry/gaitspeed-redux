#pragma once
#include <stdint.h>
#include "gait_types.h"

// Play a tone pattern.  Blocks for the duration of the tone(s).
// Call only from vBuzzer_Task (priority 2) to avoid blocking higher tasks.
void buzzer_play(uint16_t freq_hz, BeepType_t type);
