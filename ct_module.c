/**
 * @file ct_module.c
 *
 * @program to calculate RMS current measured using a Current Transformer

 * @author Rohit Gaarg (rohitg1)
 */



#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/adc.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/timer.h"
#include "math.h"

static intr_handle_t s_timer_handle;

#define TIMER_DIVIDER   80

#define TIMER_INTR_SEL TIMER_INTR_LEVEL


int timer_group = TIMER_GROUP_0;
int timer_idx = TIMER_0;

int sample_count=0;

float adc_val[50];

int flag = 0;



void IRAM_ATTR timer_group0_isr(void *para)             //Timer ISR
{

	TIMERG0.int_clr_timers.t0 = 1;

	TIMERG0.hw_timer[timer_idx].config.alarm_en = 1;

	flag=1;


}


void adc_task(void* arg)
{
	while(1){

    	if (flag == 1){
    
    		adc_val[i] = adc1_get_voltage(ADC1_CHANNEL_0) * 0.00087;       //ADC value scaled according to system voltage(3.3V)
    		adc_val[i] = adc_val[i]*adc_val[i];                     
    		sample_count++;

    		flag = 0;

    	}
    	if(sample_count == 50)                             //RMS calculation for 50 samples (320us * 50 = 16 ms => approx one AC cycle at 60Hz)
        {
    		int sum = 0;
    		for(int j=0; j<50; j++){
    			sum += adc_val[j];
    		}
    		float curr = sqrt(sum/50.0);       //RMS current 
    		printf("%2.2f\n", curr);

    
    	}
    }



static void ct_init(void *arg)
{
	adc1_config_width(ADC_WIDTH_12Bit);                             //ADC resolution
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_11db);       //ADC configuration

    timer_config_t config;                                          //Timer configuration
    config.alarm_en = 1;
    config.auto_reload = 1;
    config.counter_dir = TIMER_COUNT_UP;
    config.divider = TIMER_DIVIDER;
    config.intr_type = TIMER_INTR_SEL;
    config.counter_en = false;

    timer_init(timer_group, timer_idx, &config);

    timer_set_alarm_value(timer_group, timer_idx, 320);             //timer alarm set for 320us
 
    timer_enable_intr(timer_group, timer_idx);                      

    timer_isr_register(timer_group, timer_idx, &timer_group0_isr, NULL, 0, &s_timer_handle);     //initialize timer interrupt

    timer_start(timer_group, timer_idx);

    xTaskCreate(adc_task, "adc_task", 2048, NULL, 10, NULL);

}