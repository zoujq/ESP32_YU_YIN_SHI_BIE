/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../components/tfmicro/third_party/kissfft/tools/kiss_fftr.h"


#define FFT_STRIDE 160
#define FFT_INPUT_SIZE 512
#define FFT_OUT_SIZE (FFT_INPUT_SIZE/2+1)
#define POOLING_SIZE 6
#define POOLING_BUF_SIZE (FFT_OUT_SIZE+1)/6
#define EPSILON 1e-6

float g_hanning_window[FFT_INPUT_SIZE];

kiss_fft_scalar g_fft_input[FFT_INPUT_SIZE];
kiss_fft_cpx g_fft_out[FFT_OUT_SIZE];
float g_fft_out_square[FFT_OUT_SIZE];
float g_pool_out[POOLING_BUF_SIZE];

kiss_fftr_cfg g_fftr_cfg={};
uint8_t g_fft_mem[10*1024];
size_t g_fft_mem_len=10*1024;

float g_feature_buffer[98*43];
uint16_t g_sample_buff[18*1024];
int g_sample_buff_writer=0;
int g_sample_buff_reader=0;
float g_sample_buff_max=0;
float g_sample_buff_mean=0;

float* g_tf_input=0;
int g_tf_input_counter=0;
uint8_t g_feature_done_flag=0;
void init_feature_provide()
{
    g_fftr_cfg= kiss_fftr_alloc(FFT_INPUT_SIZE,0,g_fft_mem, &g_fft_mem_len);
    for (int i = 0; i < FFT_INPUT_SIZE; i++)
    {
        g_hanning_window[i] = 0.5-0.5*cos(2*M_PI*i/(FFT_INPUT_SIZE-1));
    }
}
void get_one_fearure()
{
    for(int i=0;i<FFT_INPUT_SIZE;i++)
    {
        float temp=0;
        
        temp=(float)g_sample_buff[g_sample_buff_reader++];

        temp-=g_sample_buff_mean;
        temp/=g_sample_buff_max;
        
        
        //2加hanning窗
        g_fft_input[i]=temp*g_hanning_window[i];
    }
   
    //傅里叶变换
    kiss_fftr(g_fftr_cfg,g_fft_input,g_fft_out);
    //计算幅值平方
    for(int i=0;i<FFT_OUT_SIZE;i++)
    {
        g_fft_out_square[i]=g_fft_out[i].r*g_fft_out[i].r+g_fft_out[i].i*g_fft_out[i].i;
    }

    //池化6，same
    for(int i=0,index=0,k=0;i<FFT_OUT_SIZE;i+=POOLING_SIZE)
    {
        float sum = 0;
        for(int j=0;j<POOLING_SIZE;j++)
        {
            if (index < FFT_OUT_SIZE)
            {
                sum += g_fft_out_square[index++];
            }
        }
        g_pool_out[k++]=sum/POOLING_SIZE;
    }
    
    // 取对数值
    for (int i = 0; i < POOLING_BUF_SIZE; i++)
    {
        //赋值到g_tf_input
        g_feature_buffer[g_tf_input_counter++]=log10f(g_pool_out[i] + EPSILON);
    }    
}


void get_fearure_buffer()
{

    if(g_sample_buff_writer<16000)
    {
        return;//数量不够
    }
    
    //求平均值
    float sum=0;
    for (int i = 0; i < 16000; ++i)
    {
        sum+=(float)g_sample_buff[i];
    }
    g_sample_buff_mean=sum/16000.0;

    //求最大值
    g_sample_buff_max=1;
    for (int i = 0; i < 16000; ++i)
    {
        float temp=(float)g_sample_buff[i]-g_sample_buff_mean;
        temp=fabs(temp);
        if(g_sample_buff_max<temp)
        {
            g_sample_buff_max=temp;
        }
    }
    //采集数据切片，分别获取频谱图，并赋值给tensorflow
    g_sample_buff_reader=0;
    g_tf_input_counter=0;    
    for (int i = 0; i < 98; ++i)
    {
        g_sample_buff_reader=i*160;
        get_one_fearure();
    }

    //赋值给TensorFlow
    for (int i = 0; i < 98*43; ++i)
    {
        g_tf_input[i]=g_feature_buffer[i];
    }
    g_feature_done_flag=1; 
    
    // printf("g_sample_buff start\n\r");
    // printf("[");
    // for (int i = 0; i < 16000; ++i)
    // {
    //     printf("%d,", g_sample_buff[i]);
    // }
    // printf("]\n\r");

    // printf("g_feature_buffer start\n\r");
    // printf("[");
    // for (int i = 0; i < 98*43; ++i)
    // {
    //     printf("%f,", g_feature_buffer[i]);
    // }
    // printf("]\n\r");
    
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // printf("start8\n\r");
    // vTaskDelay(2000 / portTICK_PERIOD_MS);
    // printf("start0\n\r");
    // g_sample_buff_writer=0;

    //采集数据移动1600个数据
    vTaskSuspendAll();
    g_sample_buff_writer-=3200;
    for (int i = 0; i < g_sample_buff_writer ; ++i)
    {
        g_sample_buff[i]=g_sample_buff[i+3200];
    }
    xTaskResumeAll();  
    // printf("x3-%d\n\r",xTaskGetTickCount());
}

