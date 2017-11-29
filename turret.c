#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>       // Required for the GPIO functions
#include <linux/kobject.h>    // Using kobjects for the sysfs bindings
#include <linux/kthread.h>    // Using kthreads for the flashing functionality
#include <linux/delay.h>      // Using this header for the msleep() function

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("David Hudson, David Vercillo, Thien Nguyen");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("This module controls the DreamCheeky Turret");  ///< The description -- see modinfo
MODULE_VERSION("0.1");  

// 5V Pins 2 and 4 are for powering the turret
// GND Pins 6,9,14,20,25,30,34,39
// GPIO Pins 05,06,12,13,16,19,20,21,26
static unsigned int gpio_fire = 12;
module_param(gpio_fire,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_fire, " GPIO LED number (default=12)");     ///< parameter description

//enum modes { TURN, RAISE, FIRE, STANDBY};
//static enum modes mode = STANDBY;

static int __init turret_init(void) {
  gpio_request(gpio_fire,"sysfs");
  gpio_direction_output(gpio_fire,true);
  gpio_export(gpio_fire,false);
  printk("<1> Loading Turret Module\n");
  return 0;

}



static void __exit turret_exit(void) {
  gpio_set_value(gpio_fire,0);
  gpio_unexport(gpio_fire);
  gpio_free(gpio_fire);
  printk("<1> Remove Turret Module\n");
}



module_init(turret_init);

module_exit(turret_exit);
