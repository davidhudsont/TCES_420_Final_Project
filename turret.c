#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#i

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("David Hudson, David Vercillo, Thien Nguyen");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("This module controls the DreamCheeky Turret");  ///< The description -- see modinfo
MODULE_VERSION("0.1");  

enum modes { TURN, RAISE, FIRE, STANDBY};
static enum modes mode = STANDBY;

static int __init turret_init(void) {

  printk("<1> Loading Turret Module\n");

  return 0;

}



static void __exit turret_exit(void) {

  printk("<1> Remove Turret Module\n");

}



module_init(turret_init);

module_exit(turret_exit);
