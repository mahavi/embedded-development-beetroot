#include "app.h"
#include "configuration.h"
#include "main.h"

#include <stdio.h>

static ADC_HandleTypeDef *adc;
static UART_HandleTypeDef *uart;

static volatile uint16_t adc_samples[APP_ADC_SAMPLE_COUNT];
static uint8_t uart_buffer[APP_UART_BUFFER_SIZE];

static uint32_t last_uart_time;

void app_init(ADC_HandleTypeDef *hadc, UART_HandleTypeDef *huart)
{
    adc = hadc;
    uart = huart;
    last_uart_time = HAL_GetTick();

    if (HAL_ADC_Start_DMA(
            adc,
            (uint32_t *)adc_samples,
            APP_ADC_SAMPLE_COUNT) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * We do not use ADC half/full-buffer callbacks.
     * The DMA continues operating in circular mode.
     */
    __HAL_DMA_DISABLE_IT(adc->DMA_Handle, DMA_IT_HT);
    __HAL_DMA_DISABLE_IT(adc->DMA_Handle, DMA_IT_TC);
}

void app_process(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - last_uart_time) < APP_UART_PERIOD_MS)
    {
        return;
    }

    /*
     * Do not overwrite uart_buffer while DMA is still transmitting it.
     */
    if (HAL_UART_GetState(uart) != HAL_UART_STATE_READY)
    {
        return;
    }

    uint16_t adc_min = APP_ADC_MAX_VALUE;
    uint16_t adc_max = 0U;
    uint32_t adc_sum = 0U;

    for (uint32_t i = 0U; i < APP_ADC_SAMPLE_COUNT; ++i)
    {
        uint16_t sample = adc_samples[i];

        if (sample < adc_min)
        {
            adc_min = sample;
        }

        if (sample > adc_max)
        {
            adc_max = sample;
        }

        adc_sum += sample;
    }

    uint16_t adc_average = adc_sum / APP_ADC_SAMPLE_COUNT;
    uint16_t adc_peak_to_peak = adc_max - adc_min;

    uint32_t voltage_min_mv =
        ((uint32_t)adc_min * APP_ADC_REFERENCE_MV) / APP_ADC_MAX_VALUE;
    uint32_t voltage_max_mv =
        ((uint32_t)adc_max * APP_ADC_REFERENCE_MV) / APP_ADC_MAX_VALUE;
    uint32_t voltage_peak_to_peak_mv =
        ((uint32_t)adc_peak_to_peak * APP_ADC_REFERENCE_MV) / APP_ADC_MAX_VALUE;
    uint32_t voltage_average_mv =
        ((uint32_t)adc_average * APP_ADC_REFERENCE_MV) / APP_ADC_MAX_VALUE;

    int length = snprintf(
        (char *)uart_buffer,
        sizeof(uart_buffer),
        "Vmin=%lumV Vmax=%lumV Vpp=%lumV Vavg=%lumV\r\n",
        (unsigned long)voltage_min_mv,
        (unsigned long)voltage_max_mv,
        (unsigned long)voltage_peak_to_peak_mv,
        (unsigned long)voltage_average_mv);

    if (length <= 0)
    {
        return;
    }

    if ((size_t)length >= sizeof(uart_buffer))
    {
        length = sizeof(uart_buffer) - 1U;
    }

    if (HAL_UART_Transmit_DMA(
            uart,
            uart_buffer,
            (uint16_t)length) == HAL_OK)
    {
        last_uart_time = now;
    }
}
