#include "actuators.h"
#include "relays.h"

esp_err_t actuators_all_safe(void)
{
    return relays_force_safe();
}

esp_err_t actuators_mixer(bool on)
{
    return relays_set(RELAY_R1_MIXER, on);
}

esp_err_t actuators_door_extend(bool extend)
{
    return relays_set(RELAY_R2_DOOR, extend);
}

esp_err_t actuators_press_forward(bool on)
{
    if (on) {
        relays_set(RELAY_R4_PRESS_REV, false);
        return relays_set(RELAY_R3_PRESS_FWD, true);
    }
    return relays_set(RELAY_R3_PRESS_FWD, false);
}

esp_err_t actuators_press_reverse(bool on)
{
    if (on) {
        relays_set(RELAY_R3_PRESS_FWD, false);
        return relays_set(RELAY_R4_PRESS_REV, true);
    }
    return relays_set(RELAY_R4_PRESS_REV, false);
}

esp_err_t actuators_press_coast(void)
{
    esp_err_t e1 = relays_set(RELAY_R3_PRESS_FWD, false);
    esp_err_t e2 = relays_set(RELAY_R4_PRESS_REV, false);
    return (e1 != ESP_OK) ? e1 : e2;
}

esp_err_t actuators_cutter_extend(bool extend)
{
    return relays_set(RELAY_R5_CUTTER, extend);
}

esp_err_t actuators_conveyor(bool on)
{
    return relays_set(RELAY_R6_CONVEYOR, on);
}

esp_err_t actuators_flattener_extend(bool extend)
{
    return relays_set(RELAY_R7_FLATTENER, extend);
}
