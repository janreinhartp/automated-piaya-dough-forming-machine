#ifndef PCF8575_H
#define PCF8575_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PCF8575_DEFAULT_ADDR 0x20
/** Pass as addr to pcf8575_init() to auto-pick first ACK in 0x20..0x27 (skips 0x24). */
#define PCF8575_ADDR_AUTO 0x00

/**
 * @brief Probe PCF8575 candidates 0x20..0x27 (skip board IO_EXTENSION 0x24).
 * @return Found address, or 0 if none.
 */
uint8_t pcf8575_detect_addr(void);

/**
 * @brief Initialize PCF8575 on the shared Waveshare I2C bus.
 *
 * Requires DEV_I2C_Init() / touch bring-up to have created the master bus.
 *
 * @param addr 7-bit I2C address, or PCF8575_ADDR_AUTO to detect.
 */
esp_err_t pcf8575_init(uint8_t addr);

/** Write both ports (P0 then P1). */
esp_err_t pcf8575_write_ports(uint8_t port0, uint8_t port1);

/** Write Port0 only, preserving last Port1 image. */
esp_err_t pcf8575_write_port0(uint8_t port0);

/** Read both ports into out_port0 / out_port1. */
esp_err_t pcf8575_read_ports(uint8_t *out_port0, uint8_t *out_port1);

bool pcf8575_is_ready(void);
uint8_t pcf8575_get_addr(void);

#ifdef __cplusplus
}
#endif

#endif /* PCF8575_H */
