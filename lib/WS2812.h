#ifndef WS2812
#define WS2812

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

static inline uint32_t urgb_u32(char *color_name);
int get_color_index(const char *color);
void set_pattern(PIO pio, uint sm, uint8_t pattern, char *color_name);
void clear_matrix(PIO pio, uint sm);

#endif
