# TCES_420_Final_Project

# Kernel Tree installation
* Follow this guide > https://www.raspberrypi.org/documentation/linux/kernel/building.md

# Instructions
## Module Installation
 $ git clone https://github.com/hkdavid74/TCES_420_Final_Project.git

 $ cd TCES_420_Final_Project

 $ make

 $ sudo su

 $ insmod turret.ko

* Check the dmesg to see if the turret installed
 $ dmesg

## Module Variables Location
 $ cd /sys/pi/Turret
 
* If you want to see paramters
 
 $ ls 

* If you want to see the value of each parameter use cat

 $ cat nr_missiles
* If you want to change the value of a variable e.g.

 $ echo 1 > FIRE_ALL

## Module Removal
 $ cd /home/pi/TCES_420_Final_Project

 $ rmmod turret.ko
 
* Check dmesg to see if turret uninstalled.
