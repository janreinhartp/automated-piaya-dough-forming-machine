#ifndef N4D8B08_H
#define N4D8B08_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define N4D8B08_UART_NUM      UART_NUM_1
#define N4D8B08_TX_GPIO       16
#define N4D8B08_RX_GPIO       15
#define N4D8B08_DEFAULT_SLAVE 1
#define N4D8B08_DEFAULT_BAUD  9600

esp_err_t n4d8b08_init(uint8_t slave_id, uint32_t baud);
esp_err_t n4d8b08_set_channel(uint8_t channel_1_to_8, bool open);
esp_err_t n4d8b08_close_all(void);
bool n4d8b08_is_ready(void);

#endif
