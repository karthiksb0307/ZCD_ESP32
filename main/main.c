#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32/rom/ets_sys.h"
#include "esp32/rom/gpio.h"
#include "esp_log.h"
#include <stddef.h>
#include "esp_intr_alloc.h"
#include "esp_attr.h"
#include "driver/timer.h"
#include "driver/gpio.h"

#define ZERO_CROSSING_PIN	25
#define PWM_PIN1				14
#define PWM_PIN2				26


static const char* TAG = "mol_lamp_vidula";
static intr_handle_t s_timer_handle;

volatile int i1=0;
volatile int i2=0;
volatile uint8_t zero_cross1=0;
volatile uint8_t zero_cross2=0;
int dim1 = 128;
int dim2 = 128;
int dim_width1=128;
int dim_width2=128;
int inc = 0;


//*****************************************
//*****************************************
//********** TIMER TG0 INTERRUPT **********
//*****************************************
//*****************************************
static void timer_tg0_isr(void* arg)
{
	static int io_state = 0;




    //----- HERE EVERY #uS -----

	//Toggle a pin so we can verify the timer is working using an oscilloscope
	io_state ^= 1;									//Toggle the pins state
	gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_27, io_state);
	
	//ets_printf("timer interrupt triggered\n");
	if(zero_cross1==1)
	{
		if(i1>=dim1)
		{
			gpio_set_level(PWM_PIN1, 1);
			i1=0;
			zero_cross1 =0;
		}
		else
		{
			i1++;
		}
	}

	if(zero_cross2==1)
	{
		if(i2>=dim2)
		{
			gpio_set_level(PWM_PIN2, 1);
			i2=0;
			zero_cross2 =0;
		}
		else
		{
			i2++;
		}
	} 
	//Reset irq and set for next time
	
	//timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
	//timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
	
	TIMERG0.int_clr_timers.t0 = 1;
	TIMERG0.hw_timer[0].config.alarm_en = 1;
}



//******************************************
//******************************************
//********** TIMER TG0 INITIALISE **********
//******************************************
//******************************************
void timer_tg0_initialise (int timer_period_us)
{
    timer_config_t config = {
            .alarm_en = true,				//Alarm Enable
            .counter_en = false,			//If the counter is enabled it will start incrementing / decrementing immediately after calling timer_init()
            .intr_type = TIMER_INTR_LEVEL,	//Is interrupt is triggered on timer’s alarm (timer_intr_mode_t)
            .counter_dir = TIMER_COUNT_UP,	//Does counter increment or decrement (timer_count_dir_t)
            .auto_reload = true,			//If counter should auto_reload a specific initial value on the timer’s alarm, or continue incrementing or decrementing.
            .divider = 80   				//Divisor of the incoming 80 MHz (12.5nS) APB_CLK clock. E.g. 80 = 1uS per timer tick
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);//check this value
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_period_us);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_tg0_isr, NULL, 0, &s_timer_handle);

    timer_start(TIMER_GROUP_0, TIMER_0);
}


/* In your initialise function somewhere
	//----- CREATE OUR HARDWARE TIMER -----
	timer_tg0_initialise(100);
*/

void IRAM_ATTR zero_crossing_isr_handler(void* arg)
{
	zero_cross1 =1;
	zero_cross2 =1;
	i1=0;
	i2=0;
	gpio_set_level(PWM_PIN1, 0);
	gpio_set_level(PWM_PIN2, 0);
}


void dimmer1(int val)
{
	dim1 = dim_width1 - val;
}

void dimmer2(int val)
{
	dim2 = dim_width2 - val;
}

void gpio_init()
{
	gpio_pad_select_gpio(ZERO_CROSSING_PIN);
	gpio_pad_select_gpio(PWM_PIN1);
	gpio_pad_select_gpio(PWM_PIN2);

	
	gpio_set_direction(PWM_PIN1, GPIO_MODE_OUTPUT);
	gpio_set_direction(PWM_PIN2, GPIO_MODE_OUTPUT);

	gpio_set_direction(ZERO_CROSSING_PIN, GPIO_MODE_INPUT);
	
	gpio_set_intr_type(ZERO_CROSSING_PIN, GPIO_INTR_POSEDGE);
	
	gpio_set_pull_mode(ZERO_CROSSING_PIN, GPIO_PULLUP_ONLY);
	
	gpio_install_isr_service(0);
	
	gpio_isr_handler_add(ZERO_CROSSING_PIN, zero_crossing_isr_handler, (void *) ZERO_CROSSING_PIN);
}

void app_main()
{
	//dim1 = 115;
	//dim2 = 0;
	gpio_init();
	
	//ESP_LOGI(TAG, "MAIN STARTED dim1:%d, dim2:%d", dim1,dim2);
	timer_tg0_initialise(75);

	dimmer1(128);
	dimmer2(0);
	
	//dim1=50;
	//  while(1)
	// {	
	// 	inc = inc +1;
	// 	dim1=inc;
	// 	if((dim1>=128) || (dim1<0))
	// 	inc=0;
	// 	vTaskDelay(10);
	// 	ESP_LOGI(TAG, "1 sec Delay %d\n",dim1);
	// } 
	

	/* dim+=inc;
	if((dim>=128) || (dim<=0))
	inc*=-1;
	vTaskDelay(18); */
	ESP_LOGI(TAG, "MAIN END");
}
