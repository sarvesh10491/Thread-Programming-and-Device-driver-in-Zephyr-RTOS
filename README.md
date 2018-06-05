Thread Programming and Device driver in Zephyr RTOS 
----------------------------------------------------------------------------------------------------------------------------------------- 

   Following project is used to develop an application to develop a distance sensor channel for HC-SR04 and an I2C-based EEPROM device driver Zephyr RTOS (version 1.10.0) on Galileo Gen 2 board.

Getting Started :

    These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 
    See deployment for notes on how to deploy the project on a live system.

Prerequisites :

  zephyr 1.10.0 source code
  zephyr sdk 0.9.2
  CMake version 3.8.2 or higher is required
  HC-SR04 distance sensor 
  Microchip 24FC256 CMOS EEPROM 

Installing :

Unzip & download below files in user directory on your machine running linux distribution.

   1) README.md
   2) sensor (directory)
   3) Report_4
   4) flash(directory)
   5) HCSR_app(directory)

Deployment :

   Install zephyr source code with patched version & sdk. 
   
   Create the following directories
   efi
   efi/boot
   kernel

   Locate binary at $ZEPHYR_BASE/boards/x86/galileo/support/grub/bin/grub.efi and copy it to $SDCARD/efi/boot 
   
   Rename grub.efi to bootia32.efi

   Create a $SDCARD/efi/boot/grub.cfg file wich contains following :
   set default=0
   set timeout=10

   menuentry "Zephyr Kernel" {
   multiboot /kernel/zephyr.strip
   }

   Make following connections of GPIO : 
   IO1 => Trigger pin for HC-SR04 distance sensor 1
   IO2 => Echo pin for HC-SR04 distance sensor 1
   IO3 => Trigger pin for HC-SR04 distance sensor 2
   IO4 => Echo pin for HC-SR04 distance sensor 2

   IO18 => SDA pin for Microchip 24FC256 CMOS EEPROM 
   IO19 => SCL pin for Microchip 24FC256 CMOS EEPROM 
   IO0 => Write-protection pin for Microchip 24FC256 CMOS EEPROM 

   A0, A1, A2 set to ground.

Execution :
   
   From sensor directory, copy and replace contents in /zephyr/drivers/sensor directory
   From flash directory, copy and replace contents in /zephyr/drivers/flash directory
   Copy HCSR_app in /zephyr/samples directory
 
   Change directory to following :
   cd $ZEPHYR_BASE/samples/HCSR_app/build

   Use cmake command as below :
   cmake -DBOARD=galileo ..

   Make using following :
   make

   Locate zephyr.strip file generated in $ZEPHYR_BASE/samples/HCSR_app/build/zephyr 

   Copy this to $SDCARD/kernel 

   Insert SD card in Galileo board and reboot.

-----------------------------------------------------------------------------------------------

Expected results :

On execution program asks user to enter command in shell prompt.
Select respective shell module e.g.
select shell_mod

1] To enable particular sensor,use enable command with suitable n value. 
n can take {0-None, 1-HCSR0, 2-HCSR1}
shell_mod> enable n

2] To write p pages of EEPROM, use start command with suitable p 
where p can take number between {1 to 512}.
For example with "start 10" command, it will write 10 pages to EEPROM.
shell_mod> start p

3] To read pages of EEPROM, use dump command with suitable p1 & p2 that defines range of pages to be read. 
p1 & p2 can take number between {1 to 512} & p1 < p2
For example with "dump 2 5" command, it will read page 2 to 5 from EEPROM.
shell_mod> dump p1 p2


Built With :

  Source : Zephyr 1.10.0
  SDK : Zephyr-0.9.2
  64 bit x86 machine

Authors :

Sarvesh Patil (1213353386)
Vishwakumar Doshi(1213322381)

Reference :
http://docs.zephyrproject.org/kernel/kernel.html

License :

This project is licensed under the ASU License

