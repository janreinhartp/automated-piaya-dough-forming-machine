#ifndef MACHINE_SAFETY_H
#define MACHINE_SAFETY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void safety_init(void);
bool safety_is_recovery_unlocked(void);
void safety_set_recovery_unlocked(bool unlocked);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_SAFETY_H */
