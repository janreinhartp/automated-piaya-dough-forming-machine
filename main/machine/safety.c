#include "safety.h"

static bool s_recovery_unlocked = false;

void safety_init(void)
{
    s_recovery_unlocked = false;
}

bool safety_is_recovery_unlocked(void)
{
    return s_recovery_unlocked;
}

void safety_set_recovery_unlocked(bool unlocked)
{
    s_recovery_unlocked = unlocked;
}
