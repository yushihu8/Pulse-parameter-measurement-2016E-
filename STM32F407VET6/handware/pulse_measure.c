#include "pulse_measure.h"

#include <limits.h>
#include <string.h>

#define PULSE_MEASURE_DEFAULT_RATE_HZ      50000000.0f
#define PULSE_MEASURE_DEFAULT_FULL_VPP     4.0f
#define PULSE_MEASURE_DEFAULT_ADC_MAX_CODE 16383U
#define PULSE_MEASURE_DEFAULT_LEVEL_AVG    8U
#define PULSE_MEASURE_DEFAULT_BAD_ZERO     UINT16_MAX
#define PULSE_MEASURE_DEFAULT_MIN_AMP      20U
#define PULSE_MEASURE_DEFAULT_EDGE_GUARD   20U
#define PULSE_MEASURE_DEFAULT_MIN_RISE_SPAN 6U

static void pulse_measure_insert_smallest(uint16_t *values, uint16_t count, uint16_t sample)
{
  for(uint16_t i = 0; i < count; i++)
  {
    if(sample < values[i])
    {
      for(uint16_t j = count - 1U; j > i; j--)
      {
        values[j] = values[j - 1U];
      }
      values[i] = sample;
      break;
    }
  }
}

static void pulse_measure_insert_largest(uint16_t *values, uint16_t count, uint16_t sample)
{
  for(uint16_t i = 0; i < count; i++)
  {
    if(sample > values[i])
    {
      for(uint16_t j = count - 1U; j > i; j--)
      {
        values[j] = values[j - 1U];
      }
      values[i] = sample;
      break;
    }
  }
}

static bool pulse_measure_repair_samples(uint16_t *samples, uint16_t sample_count, uint16_t *zero_count)
{
  uint16_t zeros = 0U;

  for(uint16_t i = 0; i < sample_count; i++)
  {
    if(samples[i] == 0U)
    {
      zeros++;
    }
  }

  *zero_count = zeros;

  if(zeros == 0U)
  {
    return true;
  }

  for(uint16_t i = 1U; i < (sample_count - 1U); i++)
  {
    if((samples[i] == 0U) &&
       (samples[i - 1U] != 0U) &&
       (samples[i + 1U] != 0U))
    {
      samples[i] = (uint16_t)(((uint32_t)samples[i - 1U] + samples[i + 1U]) / 2U);
    }
  }

  return true;
}

static bool pulse_measure_calc_levels(const uint16_t *samples,
                                      uint16_t sample_count,
                                      const pulse_measure_config_t *config,
                                      pulse_measure_result_t *result)
{
  uint16_t low_values[PULSE_MEASURE_DEFAULT_LEVEL_AVG];
  uint16_t high_values[PULSE_MEASURE_DEFAULT_LEVEL_AVG];
  uint32_t low_sum = 0U;
  uint32_t high_sum = 0U;
  uint16_t valid_low = 0U;
  uint16_t valid_high = 0U;

  if(config->level_avg_count > PULSE_MEASURE_DEFAULT_LEVEL_AVG)
  {
    return false;
  }

  for(uint16_t i = 0; i < PULSE_MEASURE_DEFAULT_LEVEL_AVG; i++)
  {
    low_values[i] = UINT16_MAX;
    high_values[i] = 0U;
  }

  for(uint16_t i = 0; i < sample_count; i++)
  {
    if(samples[i] == 0U)
    {
      continue;
    }

    pulse_measure_insert_smallest(low_values, config->level_avg_count, samples[i]);
    pulse_measure_insert_largest(high_values, config->level_avg_count, samples[i]);
  }

  for(uint16_t i = 0; i < config->level_avg_count; i++)
  {
    if(low_values[i] != UINT16_MAX)
    {
      low_sum += low_values[i];
      valid_low++;
    }

    if(high_values[i] != 0U)
    {
      high_sum += high_values[i];
      valid_high++;
    }
  }

  if((valid_low == 0U) || (valid_high == 0U))
  {
    return false;
  }

  result->low_code = (uint16_t)(low_sum / valid_low);
  result->high_code = (uint16_t)(high_sum / valid_high);

  if(result->high_code <= result->low_code)  
  {
    return false;
  }

  result->amplitude_code = (uint16_t)(result->high_code - result->low_code)*0.96;
  result->amplitude_vpp = ((float)result->amplitude_code / (float)config->adc_max_code) *
                          config->adc_full_scale_vpp;

  return (result->amplitude_code >= config->min_amplitude_code);
}

static bool pulse_measure_find_crossing(const uint16_t *samples,
                                        uint16_t sample_count,
                                        float threshold,
                                        float ts_ns,
                                        uint16_t start_index,
                                        float *time_ns,
                                        uint16_t *cross_index)
{
  for(uint16_t i = start_index; i < (sample_count - 1U); i++)
  {
    float y0 = (float)samples[i];
    float y1 = (float)samples[i + 1U];

    if((samples[i] == 0U) || (samples[i + 1U] == 0U) || (y1 <= y0))
    {
      continue;
    }

    if((y0 <= threshold) && (y1 >= threshold))
    {
      float frac = (threshold - y0) / (y1 - y0);
      *time_ns = ((float)i + frac) * ts_ns;
      *cross_index = i;
      return true;
    }
  }

  return false;
}

static bool pulse_measure_calc_rise_time(const uint16_t *samples,
                                         uint16_t sample_count,
                                         const pulse_measure_config_t *config,
                                         pulse_measure_result_t *result)
{
  float amplitude = (float)result->amplitude_code;
  float threshold10 = (float)result->low_code + amplitude * 0.1f;
  float threshold90 = (float)result->low_code + amplitude * 0.9f;
  float ts_ns = 1000000000.0f / config->sample_rate_hz;
  uint16_t cross10_index = 0U;
  uint16_t cross90_index = 0U;
  uint16_t rise_span = 0U;
  uint16_t drop_steps = 0U;

  if((config->sample_rate_hz <= 0.0f) ||
     (sample_count <= (uint16_t)(config->edge_guard_samples * 2U + 2U)))
  {
    return false;
  }

  if(!pulse_measure_find_crossing(samples,
                                  sample_count,
                                  threshold10,
                                  ts_ns,
                                  0U,
                                  &result->t10_ns,
                                  &cross10_index))
  {
    return false;
  }

  if(!pulse_measure_find_crossing(samples,
                                  sample_count,
                                  threshold90,
                                  ts_ns,
                                  cross10_index,
                                  &result->t90_ns,
                                  &cross90_index))
  {
    return false;
  }

  if((cross10_index < config->edge_guard_samples) ||
     (cross90_index >= (uint16_t)(sample_count - config->edge_guard_samples - 1U)) ||
     (cross90_index <= cross10_index))
  {
    return false;
  }

  rise_span = (uint16_t)(cross90_index - cross10_index + 1U);
  if(rise_span < config->min_rise_span_samples)
  {
    return false;
  }

  for(uint16_t i = cross10_index; i < cross90_index; i++)
  {
    if(samples[i + 1U] < samples[i])
    {
      drop_steps++;
    }
  }

  if(drop_steps > (rise_span / 4U))
  {
    return false;
  }

  result->rise_time_ns = result->t90_ns - result->t10_ns;

  return (result->rise_time_ns > 0.0f);
}

void pulse_measure_get_default_config(pulse_measure_config_t *config)
{
  if(config == NULL)
  {
    return;
  }

  config->sample_rate_hz = PULSE_MEASURE_DEFAULT_RATE_HZ;
  config->adc_full_scale_vpp = PULSE_MEASURE_DEFAULT_FULL_VPP;
  config->adc_max_code = PULSE_MEASURE_DEFAULT_ADC_MAX_CODE;
  config->level_avg_count = PULSE_MEASURE_DEFAULT_LEVEL_AVG;
  config->bad_zero_limit = PULSE_MEASURE_DEFAULT_BAD_ZERO;
  config->min_amplitude_code = PULSE_MEASURE_DEFAULT_MIN_AMP;
  config->edge_guard_samples = PULSE_MEASURE_DEFAULT_EDGE_GUARD;
  config->min_rise_span_samples = PULSE_MEASURE_DEFAULT_MIN_RISE_SPAN;
}

bool pulse_measure_analyze(uint16_t *samples,
                           uint16_t sample_count,
                           const pulse_measure_config_t *config,
                           pulse_measure_result_t *result)
{
  pulse_measure_config_t default_config;
  const pulse_measure_config_t *active_config = config;

  if((samples == NULL) || (result == NULL) || (sample_count < 2U))
  {
    return false;
  }

  if(active_config == NULL)
  {
    pulse_measure_get_default_config(&default_config);
    active_config = &default_config;
  }

  memset(result, 0, sizeof(*result));
  result->rise_time_ns = -1.0f;
  result->t10_ns = -1.0f;
  result->t90_ns = -1.0f;

  pulse_measure_repair_samples(samples, sample_count, &result->zero_count);
  if((active_config->bad_zero_limit != UINT16_MAX) &&
     (result->zero_count > active_config->bad_zero_limit))
  {
    return false;
  }

  if(!pulse_measure_calc_levels(samples, sample_count, active_config, result))
  {
    return false;
  }

  result->frame_valid = true;
  result->rise_time_valid = pulse_measure_calc_rise_time(samples,
                                                         sample_count,
                                                         active_config,
                                                         result);

  return true;
}
