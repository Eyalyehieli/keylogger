#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/uaccess.h> // copy_from_user()

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h> /* O_ACCMODE */




MODULE_LICENSE("Dual BSD/GPL");

struct mychar_struct {
  int opens;
  int releases;
  int writes;
  int reads;
  char *kernelbuff;
  dev_t dev;
  struct cdev my_cdev;
};

static struct mychar_struct *mydev = NULL;


static int mychar_open(struct inode *inode, struct file *filp)
{
  struct mychar_struct *mydev;
  
  mydev = container_of(inode->i_cdev, struct mychar_struct, my_cdev);
  filp->private_data = mydev;
    
  printk(KERN_ALERT "mychar_open called %d times\n", ++mydev->opens);
  return 0;
}

static ssize_t mychar_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
  static int written = 0;
  char str[] = "device drivers are cool\n";
  int wb = (sizeof(str) < count) ? sizeof(str) : count;
  int res;
  struct mychar_struct *mydev = filp->private_data;
    
  written = !written;
  if (!written)
    return 0; // EOF
  
  res = copy_to_user(buff, str, wb);
  if (res == 0)
  {
    printk(KERN_ALERT "mychar_read called %d timed\n", ++mydev->reads);
    res = wb;
    *offp = wb;
  }
  else
  {
    res = -ENOMEM;
  }
  return res;
}

static ssize_t mychar_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
  char kernelbuff[128];
  int rb = (127 < count) ? 127 : count;
  int res;
  struct mychar_struct *mydev = filp->private_data;

  res = copy_from_user(kernelbuff, buff, rb);
  if (!res)
  {
    kernelbuff[rb]=0;
    printk(KERN_ALERT "mychar_write called %d times. buff: %s\n", ++mydev->writes, kernelbuff);
    res = rb;
  }
  return res;
}
  
static int mychar_release(struct inode *inode, struct file *filp)
{
  struct mychar_struct *mydev = filp->private_data;

  printk(KERN_ALERT "mychar_release called %d times\n", ++mydev->releases);
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
  
  mydev = kmalloc(sizeof(*mydev), GFP_KERNEL);
  if (NULL == mydev)
  {
    res = -ENOMEM;
    goto register_failed;
  }
  memset(mydev, 0, sizeof(*mydev));

  res = alloc_chrdev_region(&mydev->dev, 0, 1, "mychar4");
  if (res < 0)
    goto register_failed;

  
  printk(KERN_ALERT "mychar4 registered\n");
  cdev_init(&mydev->my_cdev, &mychar_fops);

  res = cdev_add(&mydev->my_cdev, mydev->dev, 1);
  if (res < 0)
    goto cdev_fail;
    
  return 0;
  
  cdev_fail:
  printk(KERN_ALERT "cdev registration failed... unregistering\n");
  unregister_chrdev_region(mydev->dev, 1);
  register_failed:
  return res;
}

static void mychar_exit(void)
{
  cdev_del(&mydev->my_cdev);
  printk(KERN_ALERT "mychar3 cdev unregistered\n");
  unregister_chrdev_region(mydev->dev, 1);
  printk(KERN_ALERT "mychar3 unregistered\n");
  kfree(mydev);
}

module_init(mychar_init);
module_exit(mychar_exit);

