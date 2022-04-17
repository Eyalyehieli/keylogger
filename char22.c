#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h> // copy_from_user()
#include <linux/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");

static dev_t my_dev = 0;
static struct cdev *my_cdev = NULL;

/*struct mychar_dev {
  int private_data;
  struct cdev cdev;
};
*/
static int mychar_open(struct inode *inode, struct file *filp)
{
  printk(KERN_ALERT "mychar22_open called\n");
  return 0;
}

static ssize_t mychar_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
  static int written = 0;
  char str[] = "device drivers are cool\n";   
  printk(KERN_ALERT "mychar22_read called\n");
  int wb = (sizeof(str) < count) ? sizeof(str) : count;
  int res;
  
  written = !written;
  if (!written)
    return 0; // EOF
  res = copy_to_user(buff, str, wb);
  if (res == 0)
  {
    printk(KERN_ALERT "mychar22_read called\n");
    res = wb;
  }
  else
  {
    res = -ENOMEM;
  }
  return res;
  
}

static ssize_t mychar_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
  printk(KERN_ALERT "mychar22_write called\n");
  return count;
}
  
static int mychar_release(struct inode *inode, struct file *filp)
{
  printk(KERN_ALERT "mychar22_release called\n");
  return 0;
}


static const struct file_operations mychar_fops = {
  .owner = THIS_MODULE,
  .open = mychar_open,
  .read = mychar_read,
  .write = mychar_write,
  .release = mychar_release
};

static int mychar_init(void)
{
  int res;
  
  res = alloc_chrdev_region(&my_dev, 0, 1, "mychar22");
  if (res < 0)
    goto register_failed;

  printk(KERN_ALERT "mychar22 registered\n");
  my_cdev = cdev_alloc();
  if (NULL == my_cdev)
  {
    res = -ENOMEM;
    goto cdev_fail;
  }
  my_cdev->ops = &mychar_fops;
  res = cdev_add(my_cdev, my_dev, 1);
  if (res < 0)
    goto cdev_fail;
    
  return 0;
  
  cdev_fail:
  printk(KERN_ALERT "cdev registration failed... unregistering\n");
  unregister_chrdev_region(my_dev, 1);
  register_failed:
  return res;
}

static void mychar_exit(void)
{
  cdev_del(my_cdev);
  printk(KERN_ALERT "mychar22 cdev unregistered\n");
  unregister_chrdev_region(my_dev, 1);
  printk(KERN_ALERT "mychar22 unregistered\n");
}

module_init(mychar_init);
module_exit(mychar_exit);



