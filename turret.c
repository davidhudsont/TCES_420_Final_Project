#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("David Hudson, David Vercillo, Thien Nguyen");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("This module controls the DreamCheeky Turret");  ///< The description -- see modinfo
MODULE_VERSION("0.1");  

static int __init hello_init(void) {

  printk("<1> Hello world!\n");

  return 0;

}



static void __exit hello_exit(void) {

  printk("<1> Bye, cruel world\n");

}



module_init(hello_init);

module_exit(hello_exit);
