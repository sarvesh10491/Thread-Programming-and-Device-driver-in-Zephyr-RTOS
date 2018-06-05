#include<device.h>
#include<kernel.h>
#include<gpio.h>
#include<i2c.h>
#include<misc/util.h>
#include<zephyr.h>
#include<flash.h>
#include<board.h>
#include <zephyr/types.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <misc/printk.h>
#include <pinmux.h>

#define FC256_I2C_ADDR 0x50                          // address of eeprom chip

struct fc256_data                                    
{
	struct device *i2c;                             // i2c device to communicate with eeprom
	struct device *gpio_wp;                         // write protection gpio device
} fc256_data;



static int mem_read(struct device *dev, off_t offset, void *data,size_t len) // mem read api function
{
	
	struct fc256_data *mem_read_data = dev->driver_data;
	
	u8_t rd_addr[2];                      
	struct i2c_msg msgs[2];

	/* Now try to read back from FC256 */

	/* FRAM address */
	rd_addr[0] = (offset >> 8) & 0xFF;         // first sending address of memory location and then getting data
	rd_addr[1] = offset & 0xFF;

	/* Setup I2C messages */

	// Send the address to read from 
	msgs[0].buf = rd_addr;                    // sending address in write mode
	msgs[0].len = 2;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = data;						// getting data in read mode
	msgs[1].len = len;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(mem_read_data->i2c, &msgs[0], 2, FC256_I2C_ADDR);  //use i2c for transfer to eeprom

}

static int mem_write(struct device *dev, off_t offset, const void *data,size_t len)  // write api function
{

	struct fc256_data *mem_write_data = dev->driver_data;    
	u8_t wr_addr[2];
	struct i2c_msg msgs[2];

	/* FRAM address */
	wr_addr[0] = (offset >> 8) & 0xFF;
	wr_addr[1] = offset & 0xFF;

	/* Setup I2C messages */

	/* Send the address to write to */
	msgs[0].buf = wr_addr;                                             // sending address and then sending data both in write mode
	msgs[0].len = 2;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Data to be written, and STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = len;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(mem_write_data->i2c, &msgs[0], 2, FC256_I2C_ADDR);
}

static int mem_erase(struct device *dev, off_t offset, size_t size)
{
	u8_t data_erase[64];
	int i;

	for (i = 0; i < sizeof(data_erase); i++)
	{
		data_erase[i] = 0X00;
	}
	
	//const void *data_er=data_erase;

	struct fc256_data *mem_erase_data = dev->driver_data;
	u8_t wr_addr[2];
	struct i2c_msg msgs[2];
    for(i=0;i<512;i++)
    { 
     
		/* FRAM address */
		wr_addr[0] = ((offset+(i*64))>> 8) & 0xFF;
		wr_addr[1] = (offset+(i*64)) & 0xFF;
		
		/* Setup I2C messages */

		/* Send the address to write to */
		msgs[0].buf = wr_addr;
		msgs[0].len = 2;
		msgs[0].flags = I2C_MSG_WRITE;

		/* Data to be written, and STOP after this. */
		msgs[1].buf = data_erase;
		msgs[1].len = size;
		msgs[1].flags = I2C_MSG_WRITE| I2C_MSG_STOP;

		
		i2c_transfer(mem_erase_data->i2c, &msgs[0], 2, FC256_I2C_ADDR);
    }
	return 0;
}

	




static int mem_write_protection(struct device *dev, bool enable)       // enable or disable write protection pin IO0
{
	struct fc256_data *data = dev->driver_data;
	if(enable == true)
		gpio_pin_write(data->gpio_wp,3,1);
	else if(enable == false)
		gpio_pin_write(data->gpio_wp,3,0);
	return 0;
}

static const struct flash_driver_api fc256_api = {
	.read = mem_read,
	.write = mem_write,
	.erase = mem_erase,
	.write_protection = mem_write_protection
};

static int mem_init(struct device *dev)                               // init function
{
	int ret=1;
	struct fc256_data *data = dev->driver_data;
	
	struct device *dev_gpio = device_get_binding("EXP1");             // pin muxing of IO0 for write protection pin
	if(dev_gpio==NULL)
		printk("Error in init while binding GPIO\n");

	gpio_pin_configure(dev_gpio,0,GPIO_DIR_OUT);

	ret = gpio_pin_write(dev_gpio,0,0);
	if(ret!=0)
		printk("Error in writing to gpio 32\n");
	
	gpio_pin_configure(dev_gpio,1,GPIO_DIR_OUT);
	ret = gpio_pin_write(dev_gpio,1,0);
	if(ret!=0)
		printk("Error in writing to gpio 33\n");
	
	data->gpio_wp = device_get_binding("GPIO_0");
	gpio_pin_configure(data->gpio_wp,3,GPIO_DIR_OUT);
	
	struct fc256_data *drv_data = dev->driver_data;
	drv_data->i2c = device_get_binding(CONFIG_FC256_I2C_MASTER);         // getting I2C driver I2C_0

	if (drv_data->i2c == NULL) 
	{
		printk("Failed to get pointer to %s device!",CONFIG_FC256_I2C_MASTER);
		return -EINVAL;
	}
	
	return 0;

}

DEVICE_AND_API_INIT(FC256_NAME,CONFIG_FC256_DRV_NAME,mem_init,&fc256_data, NULL,POST_KERNEL, 90, &fc256_api);