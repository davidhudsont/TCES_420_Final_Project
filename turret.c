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
static int rotation_h = 0;
static int rotation_v = 0;
static unsigned int FIRE_ONE = 0;
static unsigned int FIRE_ALL = 0;

module_param(gpio_fire,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_fire, " GPIO Fire pin (default=12)");     ///< parameter description
module_param(gpio_turn_c,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_turn_c, " GPIO Turn Clockwise pin  (default=05)");     ///< parameter description

module_param(gpio_turn_cc,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_turn_cc, " GPIO Turn Counter Clockwise pin (default=06)");     ///< parameter description

module_param(gpio_raise_turret,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_raise_turret, " GPIO Turn Clockwise pin (default=13)");     ///< parameter description

module_param(gpio_lower_turret,uint,S_IRUGO);
MODULE_PARM_DESC(gpio_lower_turret, " GPIO Turn Clockwise pin (default=19)");     ///< parameter description

module_param(nr_missiles,uint,0664);
MODULE_PARM_DESC(nr_missiles, " Amount of missiles remaing (max = 4 and min = 0, default=4)");

module_param(FIRE_ONE,uint,0664);
MODULE_PARM_DESC(FIRE_ONE, " Firing One Interface To fire a missile set to 1");     ///< parameter description

module_param(FIRE_ALL,uint,0664);
MODULE_PARM_DESC(FIRE_ALL, " Firing All Interface To fire all missiles set to 1");


static ssize_t NR_MISSILES_REMAINING(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf,"Number of missiles remaining: %d\n", nr_missiles);
}

static ssize_t SET_NR_MISSILES(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    unsigned int amount;
    sscanf(buf,"%du",&amount);
    if ((amount>=0)&&(amount <=4)){
        nr_missiles = amount;
    } 
    return count;
}

static ssize_t FIRE_ONE_SHOW(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf,"FIRE ONE STATE: %d\n", FIRE_ONE);
}

static ssize_t FIRE_ONE_SET(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    unsigned int amount;
    sscanf(buf,"%du",&amount);
    if (amount == 1){
        FIRE_ONE = amount;
    } 
    return count;
}

static ssize_t FIRE_ALL_SHOW(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf,"FIRE ALL STATE: %d\n", FIRE_ONE);
}

static ssize_t FIRE_ALL_SET(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    unsigned int amount;
    sscanf(buf,"%du",&amount);
    if (amount == 1){
        FIRE_ALL = amount;
    } 
    return count;
}
static struct kobj_attribute nr_missile_attr = __ATTR(nr_missile,(S_IWUSR|S_IRUGO),NR_MISSILES_REMAINING,SET_NR_MISSILES);
static struct kobj_attribute fire_one_attr = __ATTR(FIRE_ONE,(S_IWUSR|S_IRUGO),FIRE_ONE_SHOW,FIRE_ONE_SET);
static struct kobj_attribute fire_all_attr = __ATTR(FIRE_ALL,(S_IWUSR|S_IRUGO),FIRE_ALL_SHOW,FIRE_ALL_SET);


static struct attribute *ebb_attrs[] = {
    &nr_missile_attr.attr,
    &fire_one_attr.attr,
    &fire_all_attr.attr,
    NULL,  
};

static char tname[7] = "Turret";
static struct attribute_group attr_group = {
    .name = tname,
    .attrs = ebb_attrs,
};

static struct kobject *pi_kobj;
static struct task_struct *task_fire;

static int FIRING(void *arg) {
    printk(KERN_INFO "Turret Firing: Thread has started running\n");
    while(!kthread_should_stop()){
        set_current_state(TASK_RUNNING);
        if ((FIRE_ONE == 1) && (nr_missiles > 0)) {
            gpio_set_value(gpio_fire,FIRE_ONE);
            ssleep(6);
            nr_missiles--;
            FIRE_ONE = 0;
            gpio_set_value(gpio_fire,FIRE_ONE);
            set_current_state(TASK_INTERRUPTIBLE);
            
        }
        else if ((FIRE_ALL == 1) && (nr_missiles > 0)) {
            gpio_set_value(gpio_fire,FIRE_ALL);
            ssleep(6*nr_missiles);
            nr_missiles = 0;
            FIRE_ALL = 0;
            gpio_set_value(gpio_fire,FIRE_ALL);
            set_current_state(TASK_INTERRUPTIBLE);
            
        }
        set_current_state(TASK_INTERRUPTIBLE);
    }
    printk(KERN_INFO "Turret Firing: Thread has run to completion\n");
    return 0;
}

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
    gpio_direction_output(gpio_fire,false);
    gpio_export(gpio_fire,false);
    task_fire = kthread_run(FIRING, NULL, "FIRE_THREAD");
    if (IS_ERR(task_fire)) {
        printk(KERN_ALERT "Turret: Failed to create the task\n");
        return PTR_ERR(task_fire);
    }
    printk("<1> Loading Turret Module\n");
    return result;

}



static void __exit turret_exit(void) {
    kthread_stop(task_fire);
    kobject_put(pi_kobj);
    gpio_set_value(gpio_fire,0);
    gpio_unexport(gpio_fire);
    gpio_free(gpio_fire);
    printk(KERN_INFO "Removing Turret Module\n");
}

module_init(turret_init);
module_exit(turret_exit);
