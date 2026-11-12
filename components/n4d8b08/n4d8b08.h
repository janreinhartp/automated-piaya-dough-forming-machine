#ifndef N4D8B08_H
#define N4D8B08_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/uart.h"
#include "esp_err.h"

#define N4D8B08_UART_NUM      UART_NUM_1
/* Waveshare ESP32-S3-Touch-LCD-7: SP3485 auto DE/RE.
 * ESP TX=GPIO16 (into transceiver DI), ESP RX=GPIO15 (from RO).
 * Waveshare silk "RS485_RXD/TXD" is named from the transceiver side. */
#define N4D8B08_TX_GPIO       16
#define N4D8B08_RX_GPIO       15
#define N4D8B08_DEFAULT_SLAVE 1
#define N4D8B08_DEFAULT_BAUD  9600

/* FC06 data word = Command (hi) | Delay (lo). Invalid command → no reply. */
#define N4D8B08_CMD_OPEN       0x0100u
#define N4D8B08_CMD_CLOSE      0x0200u
#define N4D8B08_CMD_CLOSE_ALL  0x0800u

esp_err_t n4d8b08_init(uint8_t slave_id, uint32_t baud);
esp_err_t n4d8b08_set_channel(uint8_t channel_1_to_8, bool open);
esp_err_t n4d8b08_close_all(void);
bool n4d8b08_is_ready(void);

#endif
