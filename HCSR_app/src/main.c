//################################################################################################
//
// Program     : Distance Measurement and EEPROM Driver in Zephyr RTOS
// Assignment  : 4
// Source file : main.c
// Authors     : Sarvesh Patil & Vishwakumar Doshi
// Date        : 29 April 2018
//
//################################################################################################


#include <device.h>
#include <zephyr.h>
#include <gpio.h>
#include <board.h>
#include <misc/util.h>
#include <misc/printk.h>
#include <stdio.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <pinmux.h>
#include <sensor.h>
#include <flash.h>
#include <asm_inline_gcc.h>
#include <misc/__assert.h>
#include <kernel.h>




#define MUX CONFIG_PINMUX_NAME                       // pinmux driver name
#define MY_SHELL_MODULE "shell_mod"
#define EDGE (GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH) // Active High edge confuguration macros

static struct gpio_callback callb1,callb2;	// callback objects 

struct sensor_value hcsr_value;                     //value structure to get value on channel get call
struct device *dev1,*dev2,*fc_dev,*pinmux,*hcsr_dev;
int ret,i,j,num,n,p,p1,p2;               
long long t1;                                   // Time stamp 1

struct eeprom_data                              // data to be stored in eeprom
{
	int dist;                                   
	int timestamp;
} data[8];                                     // 8 of these structures fill 1 page = 64 bytes

struct hcsr_data                               // HCSr04 Driver data
{
	struct device *trig_device;                // GPIO device used for trigger
	int pin_t,d_int;                           // pin number for trig and device number
	void (*function)();                        // Callback function for GPIO
};

struct fc256_data                              // Driver data for EEPROM
{
	struct device *i2c;                        // i2c device to bind for communication with eeprom
	struct device *gpio_wp;						// Write protection gpio device
};

void erase_mydata();
void write_mydata(int);
void read_mydata(int, int);

static void myshell_init(void);                                 // init function for shell
void init_sensor_1();
void init_sensor_2();


static int shell_command_1(int argc, char *argv[]);             // shell module command function 1
static int shell_command_2(int argc, char *argv[]);             // shell module command function 2
static int shell_command_3(int argc, char *argv[]);             // shell module command function 3


int main()
{
	pinmux = device_get_binding(MUX);
    if(pinmux==NULL)
     	printk("Error in pinmux\n");
	
	printk("Setting up GPIO\n");                              
	
	pinmux_pin_set(pinmux, 18, PINMUX_FUNC_C);                // GPIO for I2C pins
	pinmux_pin_set(pinmux, 19, PINMUX_FUNC_C);

	dev1 = device_get_binding("EXP2");
	if(dev1==NULL)
     	printk("Error in EXP2\n");

	ret=gpio_pin_write(dev1,9,1);                            // Making those pins pullup
    if(ret<0)
           printk("\nError in pin write of EXP2");

    ret=gpio_pin_write(dev1,11,1);
      if(ret<0)
           printk("\nError in pin write of EXP2");


       printk("Binding 24FC256 driver.\n");

    fc_dev = device_get_binding("FC256_NAME");              // getting eeprom device
    if(fc_dev==NULL)
    	printk("fc256 driver not found\n");
    printk("Binding success!\n");

    myshell_init();                                         // initialising shell module 

	return 0;
}


static int shell_command_1(int argc, char *argv[])              // shell module command function 1 (enable sensor?)
{
    n=atoi(argv[1]);
    if(n)
    	{
    		printk("\n\nEnabled sensor : %d\n",n);
    		if(n==1)
				init_sensor_1();
			else if(n==2)
				init_sensor_2();
    	}
    else 
    	printk("\n\nNo sensor selected\n");
    return 0;
}


static int shell_command_2(int argc, char *argv[])              // shell module command function (Start command)
{
    p=atoi(argv[1]);
    printk("\n\nNumber : %d\n", p);
    	
    erase_mydata();
    write_mydata(p);

    return 0;
}

static int shell_command_3(int argc, char *argv[])              // shell module command function
{
    p1=atoi(argv[1]);
    p2=atoi(argv[2]);
    printk("\n\nStart: %d\n", p1);
    printk("\n\nFinish: %d\n", p2);
    	
    read_mydata(p1,p2);

    return 0;
}

static struct shell_cmd commands[] = {  
    { "enable", shell_command_1, "enable argc" },   
    { "start", shell_command_2, "start argc" },
    { "dump", shell_command_3, "dump argc" },
    { NULL, NULL, NULL }
};                                                                              //structure to hold command

static void myshell_init(void)                                                  // init function for shell
{
    printk("\n\n###################################################################\n");
    printk("###################################################################\n");
    printk("Press Enter to initialise SHELL.\n\nEnter command : \nenable n (To enable None[0]/HCSR0[1]/HCSR1[2])\nstart p (To erase EEPROM & write p pages)\ndump p1 p2 (To read pages starting from page no. p1 to p2)\n\n");
    printk("###################################################################\n");
    printk("###################################################################\n");
    
    SHELL_REGISTER(MY_SHELL_MODULE, commands);
}


void erase_mydata()
{
		
		ret=flash_write_protection_set(fc_dev, false);                          //write protection off for writing
		if(ret<0)
		printk("\nError in write protection");
		k_sleep(10);
		
		ret = flash_erase(fc_dev,0x00,sizeof(data));                           // Erase eeprom

		if (ret) 
		{
			printk("Error erasing from FC256! error code (%d)\n", ret);
		} 

		else 
		{
			printk("Erased all pages\n");
		}

 }
  
   void write_mydata(int num)
   {   
		
       ret=flash_write_protection_set(fc_dev, false);
		if(ret<0)
		printk("\nError in write protection");
		k_sleep(5);


    for(j=0;j<num;j++)                                               // for number of pages given by user
    {   
    	
		for (int i = 0; i < 8; i++)                                 // put 8 structures in each page
		{
			
			t1 = _tsc_read();                                       // timestamp 1                                       
			ret = sensor_sample_fetch(hcsr_dev);                   // fetch data
			if (ret) {
				printf("sensor_sample_fetch failed return: %d\n", ret);
				break;
			}
			ret = sensor_channel_get(hcsr_dev, SENSOR_CHAN_ALL,&hcsr_value); // get channel function
			if (ret) {
				printf("sensor_channel_get failed return: %d\n", ret);
				break;
			}
			data[i].dist=hcsr_value.val1;                        // distance received from channel get
			data[i].timestamp = (_tsc_read()-t1)/400;            // timestamp in microseconds
		}
        
		/* write them to the FRAM */
		ret = flash_write(fc_dev, 0x00+(j*64),data,sizeof(data));   // Write to eeprom 1 page
		if (ret) 
		{
			printk("Error writing to FC256! error code (%d)\n", ret);
		} 

		else 
		{
			printk("Wrote %zu bytes to address 0x00.\n", sizeof(data));
		}
    }
}

void read_mydata(int a1,int a2)                                     // Reading data from eeprom
{
	
 struct eeprom_data cmp_data[8]; 
 for(j=a1-1;j<a2;j++)                                                 // from page range given by user
   {

        printk("Page %d\n\n",j);
        ret=flash_write_protection_set(fc_dev, true);             
		if(ret<0)
			printk("\nError in write protection");
			k_sleep(5);
        

		ret = flash_read(fc_dev, 0x00+(j*64),cmp_data,sizeof(cmp_data));    // Read page by page
		if (ret) 
		{
			printk("Error reading from FC256! error code (%d)\n", ret);
		} 

		else 
		{
			printk("Read %zu bytes from address 0x00.\n", sizeof(cmp_data));
		}
		
		for (i = 0; i < 8; i++)                                               //printing data
		{
			 printk("num %d: dist : %d\ttimestamp : %d\n", i,cmp_data[i].dist,cmp_data[i].timestamp); 
	    }
    }
}

void init_sensor_1()                                                  // Initialising sensor 1 if chosen by user 
{
	hcsr_dev = device_get_binding("HCSR0");
	if (hcsr_dev==NULL) {
		printf("error: no HCSR0 device\n");
		return;
	}

	printk("Ultrasonic device is %p, name is %s\n",
	       hcsr_dev, hcsr_dev->config->name);

	struct hcsr_data *data = hcsr_dev->driver_data;
	ret = pinmux_pin_set(pinmux,1,PINMUX_FUNC_A);                     // Pinmux for trigger=1 and echo=2
	if(ret!=0)
		printk("pinmux 1 error\n");

	ret = pinmux_pin_set(pinmux,2,PINMUX_FUNC_B);
	if(ret!=0)
		printk("pinmux 2 error\n");

		
	dev2 = device_get_binding("GPIO_0");
	data->trig_device = dev2;                                         // Sending trigger pin gpio device in driver
	data->pin_t = 4;												  // sending pin number in driver
	data->d_int=1;                                                    // sensor select number
	
	gpio_pin_configure(dev2,5,(GPIO_DIR_IN|GPIO_INT|EDGE));
	gpio_init_callback(&callb1,data->function, BIT(5));// intializing callback on the IO2
	ret =gpio_add_callback(dev2, &callb1);	//binding call back with device pointer
	if(ret)
		printk("Error in adding callback\n");

	ret = gpio_pin_enable_callback(dev2,5);
	if(ret)
		printk("Error in enable callback\n");
}

void init_sensor_2()                                                //Initialising sensor 2
{
	hcsr_dev = device_get_binding("HCSR1");                         
	if (hcsr_dev==NULL) {
		printf("error: no HCSR1 device\n");
		return;
	}

	printk("Ultrasonic device is %p, name is %s\n",
	       hcsr_dev, hcsr_dev->config->name);

	struct hcsr_data *data = hcsr_dev->driver_data;
	
	ret = pinmux_pin_set(pinmux,3,PINMUX_FUNC_A);                   // Trigger = 3, echo=4
	if(ret!=0)
		printk("pinmux 3 error\n");

	ret = pinmux_pin_set(pinmux,4,PINMUX_FUNC_B);
	if(ret!=0)
		printk("pinmux 4 error\n");

		
	dev2 = device_get_binding("GPIO_0");
	data->trig_device = dev2;
	data->pin_t = 6;
	data->d_int=2;
	
	dev2 = device_get_binding("GPIO_RW");
	gpio_pin_configure(dev2,4,(GPIO_DIR_IN|GPIO_INT|EDGE));
	gpio_init_callback(&callb2,data->function, BIT(4));// intializing callback on the IO4
	gpio_add_callback(dev2, &callb2);	//binding call back with device pointer
	gpio_pin_enable_callback(dev2,4);
}