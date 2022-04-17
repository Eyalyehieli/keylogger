#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>


MODULE_LICENSE("Dual BSD/GPL");

static dev_t my_dev = 0;

static int hello_init(void)
{
  int res;

  res = alloc_chrdev_region(&my_dev, 0, 1, "mychar");
  if (res < 0)
    return res;
  printk(KERN_ALERT "mychar registered\n");
  
  return 0;
}

static void hello_exit(void)
{
  unregister_chrdev_region(my_dev, 1);
  printk(KERN_ALERT "mychar unregistered\n");
}

module_init(hello_init);
module_exit(hello_exit);

