#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>       // Required for the GPIO functions
#include <linux/kobject.h>    // Using kobjects for the sysfs bindings
#include <linux/kthread.h>    // Using kthreads for the flashing functionality
#include <linux/delay.h>      // Using this header for the msleep() function
#include <linux/mutex.h>


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
static struct mutex firing_lock;
static struct mutex fire_one_lock;
static struct mutex fire_all_lock;
static struct mutex nr_missiles_lock;
static struct mutex rotation_h_lock;
static struct mutex rotation_v_lock;

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

module_param(rotation_h,uint,0664);
MODULE_PARM_DESC(rotation_h, " Firing All Interface To fire all missiles set to 1");

module_param(rotation_v,uint,0664);
MODULE_PARM_DESC(rotation_v, " Firing All Interface To fire all missiles set to 1");


static ssize_t NR_MISSILES_REMAINING(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	return sprintf(buf,"Number of missiles remaining: %d\n", nr_missiles);
}

static ssize_t SET_NR_MISSILES(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
	mutex_lock(&nr_missiles_lock);
	unsigned int amount;
	sscanf(buf,"%du",&amount);
	if ((amount>=0)&&(amount <=4)){
		nr_missiles = amount;
	} 
	mutex_unlock(&nr_missiles_lock);
	return count;
}

static ssize_t FIRE_ONE_SHOW(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	return sprintf(buf,"FIRE ONE STATE: %d\n", FIRE_ONE);
}

static ssize_t FIRE_ONE_SET(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
	mutex_lock(&fire_one_lock);
	unsigned int amount;
	sscanf(buf,"%du",&amount);
	if (amount == 1){
		FIRE_ONE = amount;
	} 
	mutex_unlock(&fire_one_lock);
	return count;
}

static ssize_t FIRE_ALL_SHOW(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	return sprintf(buf,"FIRE ALL STATE: %d\n", FIRE_ONE);
}

static ssize_t FIRE_ALL_SET(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
	mutex_lock(&fire_all_lock);
	unsigned int amount;
	sscanf(buf,"%du",&amount);
	if (amount == 1){
		FIRE_ALL = amount;
	} 
	mutex_unlock(&fire_all_lock);
	return count;
}
static ssize_t ROTATE_H_SHOW(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    	return sprintf(buf,"Rotate H STATE: %d\n", rotation_h);
}

static ssize_t ROTATE_H_SET(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
	mutex_lock(&rotation_h_lock);
	int amount;
	sscanf(buf,"%d",&amount);
	rotation_h = amount;
	mutex_unlock(&rotation_h_lock);
	return count;
}
static ssize_t ROTATE_V_SHOW(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	return sprintf(buf,"Rotate V STATE: %d\n", rotation_v);
}

static ssize_t ROTATE_V_SET(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
	mutex_lock(&rotation_v_lock);
	int amount;    	
	sscanf(buf,"%d",&amount); 
	rotation_v = amount;
	mutex_unlock(&rotation_v_lock);
	return count;
}
static struct kobj_attribute nr_missile_attr = __ATTR(nr_missile,(S_IWUSR|S_IRUGO),NR_MISSILES_REMAINING,SET_NR_MISSILES);
static struct kobj_attribute fire_one_attr = __ATTR(FIRE_ONE,(S_IWUSR|S_IRUGO),FIRE_ONE_SHOW,FIRE_ONE_SET);
static struct kobj_attribute fire_all_attr = __ATTR(FIRE_ALL,(S_IWUSR|S_IRUGO),FIRE_ALL_SHOW,FIRE_ALL_SET);
static struct kobj_attribute rotate_h_attr = __ATTR(rotation_h,(S_IWUSR|S_IRUGO),ROTATE_H_SHOW,ROTATE_H_SET);
static struct kobj_attribute rotate_v_attr = __ATTR(rotation_v,(S_IWUSR|S_IRUGO),ROTATE_V_SHOW,ROTATE_V_SET);

static struct attribute *pi_attrs[] = {
	&nr_missile_attr.attr,
	&fire_one_attr.attr,
	&fire_all_attr.attr,
	&rotate_h_attr.attr,
	&rotate_v_attr.attr,
	NULL,  
};

static char tname[7] = "Turret";
static struct attribute_group attr_group = {
	.name = tname,
	.attrs = pi_attrs,
};

static struct kobject *pi_kobj;
static struct task_struct *task_fire;
static struct task_struct *task_rotate_h;
static struct task_struct *task_rotate_v;

static int FIRING(void *arg) {
	printk(KERN_INFO "Turret Firing: Thread has started running\n");
	while(!kthread_should_stop()){
		mutex_lock(&firing_lock);
		set_current_state(TASK_RUNNING);
		if ((FIRE_ONE == 1) && (nr_missiles > 0)) {
			gpio_set_value(gpio_fire,FIRE_ONE);
			ssleep(6);
			mutex_lock(&nr_missiles_lock);
			nr_missiles--;
			mutex_unlock(&nr_missiles_lock);
			mutex_lock(&fire_one_lock);
			FIRE_ONE = 0;
			mutex_unlock(&fire_one_lock);
			gpio_set_value(gpio_fire,FIRE_ONE);
		}
		if ((FIRE_ALL == 1) &&( nr_missiles > 0)) {
			gpio_set_value(gpio_fire,FIRE_ALL);
			ssleep(6*nr_missiles);
			mutex_lock(&nr_missiles_lock);
			nr_missiles = 0;
			mutex_unlock(&nr_missiles_lock);
			mutex_lock(&fire_all_lock);
			FIRE_ALL = 0;
			mutex_unlock(&fire_all_lock);
			gpio_set_value(gpio_fire,FIRE_ALL);
		}
		mutex_unlock(&firing_lock);
		set_current_state(TASK_INTERRUPTIBLE);
	}
	printk(KERN_INFO "Turret Firing: Thread has run to completion\n");
    return 0;
}
static int Rotation_H(void *arg) {
	printk(KERN_INFO "Turret Rotation_H: Thread has started running\n");
	while(!kthread_should_stop()){
		mutex_lock(&firing_lock);
		set_current_state(TASK_RUNNING);
		if (rotation_h > 0) {
			gpio_set_value(gpio_turn_c,true);
			msleep(rotation_h*10);
			gpio_set_value(gpio_turn_c,false);
			mutex_lock(&rotation_h_lock);
			rotation_h = 0;
			mutex_unlock(&rotation_h_lock);
		}
		else if (rotation_h < 0) {
			gpio_set_value(gpio_turn_cc,true);
			rotation_h = -1*rotation_h;
			msleep(rotation_h*10);
			gpio_set_value(gpio_turn_cc,false);
			mutex_lock(&rotation_h_lock);
			rotation_h = 0;
			mutex_unlock(&rotation_h_lock);
		}
		mutex_unlock(&firing_lock);
		set_current_state(TASK_INTERRUPTIBLE);
	}
	printk(KERN_INFO "Turret Rotation_H: Thread has run to completion\n");
	return 0;
}

static int Rotation_V(void *arg) {
	printk(KERN_INFO "Turret Rotation_V: Thread has started running\n");
	while(!kthread_should_stop()){
		mutex_lock(&firing_lock);
		set_current_state(TASK_RUNNING);
		if (rotation_v > 0) {
			gpio_set_value(gpio_raise_turret,true);
			msleep(rotation_v);
			gpio_set_value(gpio_raise_turret,false);
			mutex_lock(&rotation_v_lock);
			rotation_v = 0;
			mutex_unlock(&rotation_v_lock);
		}
		else if (rotation_v < 0) {
			gpio_set_value(gpio_lower_turret,true);
			rotation_v = -1*rotation_v;
			msleep(rotation_v);
			gpio_set_value(gpio_lower_turret,false);
			mutex_lock(&rotation_v_lock);
			rotation_v = 0;
			mutex_unlock(&rotation_v_lock);
		}
		mutex_unlock(&firing_lock);
		set_current_state(TASK_INTERRUPTIBLE);
	}
	printk(KERN_INFO "Turret Rotation_V: Thread has run to completion\n");
	return 0;
}

static int __init turret_init(void) {
	mutex_init(&firing_lock);
	mutex_init(&nr_missiles_lock);
	mutex_init(&fire_one_lock);
	mutex_init(&fire_all_lock);
	mutex_init(&rotation_h_lock);
	mutex_init(&rotation_v_lock);
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

	gpio_request(gpio_turn_c,"sysfs");
	gpio_direction_output(gpio_turn_c,false);
	gpio_export(gpio_turn_c,false);

	gpio_request(gpio_turn_cc,"sysfs");
	gpio_direction_output(gpio_turn_cc,false);
	gpio_export(gpio_turn_cc,false);

	gpio_request(gpio_raise_turret,"sysfs");
	gpio_direction_output(gpio_raise_turret,false);
	gpio_export(gpio_raise_turret,false);

	gpio_request(gpio_lower_turret,"sysfs");
	gpio_direction_output(gpio_lower_turret,false);
	gpio_export(gpio_lower_turret,false);


	task_fire = kthread_run(FIRING, NULL, "FIRE_THREAD");
	task_rotate_h = kthread_run(Rotation_H,NULL,"Rotation_H_Thread");
	task_rotate_v = kthread_run(Rotation_V,NULL,"Rotation_V_Thread");

	if (IS_ERR((task_fire)|(task_rotate_h)|(task_rotate_v))) {
	printk(KERN_ALERT "Turret: Failed to create the task\n");
	return PTR_ERR((task_fire)|(task_rotate_h)|(task_rotate_v));
	}
	printk("<1> Loading Turret Module\n");
	return result;
}



static void __exit turret_exit(void) {
   	kthread_stop(task_fire);
	kthread_stop(task_rotate_h);
	kthread_stop(task_rotate_v);
	
    	kobject_put(pi_kobj);
    	gpio_set_value(gpio_fire,0);
	gpio_set_value(gpio_turn_c,0);
	gpio_set_value(gpio_turn_cc,0);
	gpio_set_value(gpio_raise_turret,0);
	gpio_set_value(gpio_lower_turret,0);
	
    	gpio_unexport(gpio_fire);
	gpio_unexport(gpio_turn_c);
	gpio_unexport(gpio_turn_cc);
	gpio_unexport(gpio_raise_turret);
	gpio_unexport(gpio_lower_turret);
	
    	gpio_free(gpio_fire);
	gpio_free(gpio_turn_c);
	gpio_free(gpio_turn_cc);
	gpio_free(gpio_raise_turret);
	gpio_free(gpio_lower_turret);
	
    	printk(KERN_INFO "Removing Turret Module\n");
}

module_init(turret_init);
module_exit(turret_exit);
