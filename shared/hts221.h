#ifndef HTS221_H
#define HTS221_H
#include <stdint.h>

float hts221_get_temperature(void);
int16_t hts221_read_raw16(uint8_t reg);

#endif