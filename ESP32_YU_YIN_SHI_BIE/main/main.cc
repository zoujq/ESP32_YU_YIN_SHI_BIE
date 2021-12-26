/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "main_functions.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"

extern void init_audio_sample_adc();
extern void init_feature_provide();
extern void get_audio_sample_data();
extern void get_fearure_buffer();

#define GPIO_OUTPUT_IO_2    (gpio_num_t)2
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO_2) 
int g_led_switch=0;

int tf_main(int argc, char* argv[]) {

  while (true) {
   
    loop();    
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}
int feature_task(int argc, char* argv[]) {

  while (true) {
    get_fearure_buffer();
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}
extern "C" void app_main() {
  int tt=0;

  gpio_config_t io_conf;
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  //disable pull-down mode
  io_conf.pull_down_en = (gpio_pulldown_t)0;
  //disable pull-up mode
  io_conf.pull_up_en = (gpio_pullup_t)0;
  //configure GPIO with the given settings
  gpio_config(&io_conf);


  setup();  
  init_audio_sample_adc();
  init_feature_provide();  

  xTaskCreate((TaskFunction_t)&tf_main, "tensorflow", 5 * 1024, NULL, 1, NULL);
  xTaskCreate((TaskFunction_t)&feature_task, "feature_task", 5 * 1024, NULL, 2, NULL);
  while(1)
  {
    get_audio_sample_data();    
    vTaskDelay(3 / portTICK_PERIOD_MS);  

    gpio_set_level(GPIO_OUTPUT_IO_2, g_led_switch);
    if(tt++>500)
    {
      tt=0;
      printf("heap:%d\n\r",heap_caps_get_free_size(MALLOC_CAP_8BIT));
    }  
  }
}