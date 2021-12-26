#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "driver/adc.h"
#include <driver/i2s.h>


#define ONE_BLOCK 160
uint16_t sample_data[ONE_BLOCK] = {0};

extern uint16_t g_sample_buff[18*1024];
extern uint16_t g_sample_buff_writer;

i2s_port_t m_i2s_port = I2S_NUM_0;
QueueHandle_t m_i2s_queue;
adc_unit_t adc_unit = ADC_UNIT_1;
adc1_channel_t adc_channel = ADC1_CHANNEL_7; // GPIO 35

void init_audio_sample_adc()
{  
    i2s_config_t adcI2SConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_LSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 40,
        .dma_buf_len = 640,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_driver_install(m_i2s_port, &adcI2SConfig, 0,NULL );
    i2s_set_adc_mode(adc_unit, adc_channel);
    i2s_adc_enable(m_i2s_port);
}


void get_audio_sample_data()
{
    size_t ret_num=0;
    i2s_read(m_i2s_port, sample_data, ONE_BLOCK*2, &ret_num, 10000);
    for (int i = 0; i < ONE_BLOCK; i++) {
        if(g_sample_buff_writer<18*1024)
        {
            g_sample_buff[g_sample_buff_writer++]=sample_data[i]-28672; 
        }
        // printf("%d,%d\n",sample_data[1]-28672,sample_data[20]);
    } 
  
}


