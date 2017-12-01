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
static unsigned int gpio_turn_c = 05;
static unsigned int gpio_turn_cc = 06;
static unsigned int gpio_raise_turret = 13;
static unsigned int gpio_lower_turret = 19;

static unsigned int nr_missiles = 4;

module_param(gpio_fire,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_fire, " GPIO Fire number (default=12)");     ///< parameter description
module_param(gpio_turn_c,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_turn_c, " GPIO Turn Clockwise number (default=05)");     ///< parameter description

module_param(gpio_turn_cc,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_turn_cc, " GPIO Turn Counter Clockwise number (default=06)");     ///< parameter description

module_param(gpio_raise_turret,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_raise_turret, " GPIO Turn Clockwise number (default=13)");     ///< parameter description

module_param(gpio_lower_turret,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_lower_turret, " GPIO Turn Clockwise number (default=19)");     ///< parameter description

module_param(nr_missiles,uint,0664);
MODULE_PARM_DESC(nr_missiles, " Amount of missiles remaing (max = 4 and min = 0, default=4)");


static ssize_t NR_MISSILES_REMAINING(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf,"Number of missiles remaining: %d\n", nr_missiles);
}

static ssize_t SET_NR_MISSILES(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    unsigned int amount;
    sscanf(buf,"%du",&amount);
    if ((amount>0)&&(amount <=4)){
        nr_missiles = amount;
    } 
    return amount;
}

static struct kobj_attribute nr_missile_attr = __ATTR(nr_missile,(S_IWUSR|S_IRUGO),NR_MISSILES_REMAINING,SET_NR_MISSILES);

static struct attribute *ebb_attrs[] = {
    &nr_missile_attr.attr,
    NULL,  
};

static char tname[7] = "Turret";
static struct attribute_group attr_group = {
    .name = tname,
    .attrs = ebb_attrs,
};

static struct kobject *pi_kobj;
//static struct task_struct *task;

static int __init turret_init(void) {
    int result = 0;
    pi_kobj = kobject_create_and_add("pi", kernel_kobj->parent);
    if (!pi_kobj){
        printk(KERN_ALERT "Failed to create kobject\n");
        kobject_put(pi_kobj);
        return -ENOMEM;
    }
    result = sysfs_create_group(pi_kobj, &attr_group);
    if (result){
        printk(KERN_ALERT "Turret : failed to create sysfs group\n");
        kobject_put(pi_kobj);
        return result;
    }
    gpio_request(gpio_fire,"sysfs");
    gpio_direction_output(gpio_fire,true);
    gpio_export(gpio_fire,false);
    printk("<1> Loading Turret Module\n");
    return result;

}



static void __exit turret_exit(void) {
    kobject_put(pi_kobj);
    gpio_set_value(gpio_fire,0);
    gpio_unexport(gpio_fire);
    gpio_free(gpio_fire);
    printk(KERN_INFO "Removing Turret Module\n");
}

module_init(turret_init);
module_exit(turret_exit);
