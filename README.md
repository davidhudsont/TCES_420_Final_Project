# TCES_420_Final_Project

# Instructions
## Module Installation
> $ git clone https://github.com/hkdavid74/TCES_420_Final_Project.git

> $ cd TCES_420_Final_Project

> $ make

> $ sudo su

> $ insmod turret.ko
* Check the dmesg to see if the turret installed
> $ dmesg

## Module Variables Location
> $ cd /sys/pi/Turret
> $ ls // To see the parameters
* If you want to see the value of each parameter use cat
> $ cat nr_missiles
* If you watn to change the value of a variable e.g.
> $ echo 1 > FIRE_ALL
