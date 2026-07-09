#ifndef __PULSE_MEASURE_H__
#define __PULSE_MEASURE_H__

#include "main.h"
#include <stdbool.h>

typedef struct
{
  float sample_rate_hz;
  float adc_full_scale_vpp;
  uint16_t adc_max_code;
  uint16_t level_avg_count;
  uint16_t bad_zero_limit;
  uint16_t min_amplitude_code;
} pulse_measure_config_t;

typedef struct
{
  uint16_t zero_count;
  uint16_t low_code;
  uint16_t high_code;
  uint16_t amplitude_code;
  float amplitude_vpp;
  float t10_ns;
  float t90_ns;
  float rise_time_ns;
  bool frame_valid;
  bool rise_time_valid;
} pulse_measure_result_t;

void pulse_measure_get_default_config(pulse_measure_config_t *config);
bool pulse_measure_analyze(uint16_t *samples,
                           uint16_t sample_count,
                           const pulse_measure_config_t *config,
                           pulse_measure_result_t *result);

#endif /* __PULSE_MEASURE_H__ */
