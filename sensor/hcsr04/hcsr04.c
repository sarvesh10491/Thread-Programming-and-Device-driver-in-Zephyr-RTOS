#include <device.h>
#include <zephyr/types.h>
#include <stdlib.h>
#include <kernel.h>
#include <string.h>
#include <board.h>
#include <sensor.h>
#include <gpio.h>
#include <errno.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <asm_inline_gcc.h>


struct sensor_value result;                        // Sensor value to be sent to user on get channel function
int d_inte;                                       // Selected driver instance

long long start=0,stop=0,diff=0;                  // time stamp variables
struct device *hc_1,*hc_2,*hc_sensor;            // devices for echo pin gpios for both deviced

struct hcsr_data {
	struct device *trig_device;                  // trigger pin gpio device
	int pin_t,d_int;							// pin number and device number
	void (*function)();                         // Callback function
};



void handler(struct device *dev, struct gpio_callback *callb, u32_t pin)
{	
	long long tmp = _tsc_read();               // timestamp taken and then decision will be made whether it was start or stop time
	int ret;
	struct hcsr_data *data = hc_sensor->driver_data;
	int v;  
	int f=0;
	//if(d_inte==1)
		ret = gpio_pin_read(hc_1,5,&v);       // Reading to confirm high or low edge
		if(ret)
			printk("Error in reading\n");
	// 1else if(d_inte==2)
	// 	ret = gpio_pin_read(hc_2,4,&v);
	// 	if(ret)
	// 		printk("Error in reading\n");
	
// Callback interrupt handler
		if(v==1)                            // if high edge
		{
			start = tmp;
			// if(d_inte==1)
				gpio_pin_configure(hc_1,5,(GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW));   // reconfigure echo pin
			// else if(d_inte==2)
				// gpio_pin_configure(hc_2,4,(GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW));

		}
	

		else if(v == 0)                  // else if it was low edge
		{
			stop = tmp;
			// if(d_inte==1)
				gpio_pin_configure(hc_1,5,(GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH));
			// else if(d_inte==2)
				// gpio_pin_configure(hc_2,4,(GPIO_DIR_IN|GPIO_INT|GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH));
			f=1; // only calculate distance when low edge
		}

		if(f)
		{
			diff = stop-start;
			result.val1=(diff/(400*58));                           // putting results in sensor_value
			printk("The diff is %lld\ndistance is = %d\n\n\n",diff, result.val1);
		}

	
}


static int hcsr_sample_fetch(struct device *dev, enum sensor_channel chan)   // sensor api function fetch
{
	int flag;
	
	struct hcsr_data *data = dev->driver_data;

	flag=gpio_pin_write(data->trig_device,data->pin_t,1); //triggering based on gpio device and  pin number given by user
	if(flag<0)
		printk("Error in gpio_pin_write 3");

	k_busy_wait(75);
	flag=gpio_pin_write(data->trig_device,data->pin_t,0); 
	if(flag<0)
		printk("Error in gpio_pin_write 3");
	k_busy_wait(100);

	return 0;
}

static int hcsr_channel_get(struct device *dev,enum sensor_channel chan,struct sensor_value *val) // channel get function
{
	val->val1 = result.val1;            // returns value back to program in val 
	return 0;
}

static const struct sensor_driver_api hcsr_driver_api =               // sensor api
{
	.sample_fetch = hcsr_sample_fetch,
	.channel_get = hcsr_channel_get,
};



static int hcsr_init(struct device *dev)              // init function for driver
{

	hc_sensor = dev;
	hc_1 = device_get_binding("GPIO_0");            // getting devices for echo 
	hc_2 = device_get_binding("GPIO_RW");
	struct hcsr_data *drv_data = dev->driver_data;
	d_inte=drv_data->d_int;                         
	drv_data->function = handler;	                 // callback function
	dev->driver_api = &hcsr_driver_api;

	return 0;

}

// DEVICE_DECLARE(HCSR0);

struct hcsr_data hcsr_driver_0, hcsr_driver_1;

DEVICE_INIT(HCSR0, CONFIG_HCSR_NAME_0, hcsr_init,&hcsr_driver_0, NULL,POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);

DEVICE_INIT(HCSR1, CONFIG_HCSR_NAME, hcsr_init,&hcsr_driver_1, NULL,POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);