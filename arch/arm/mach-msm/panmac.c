// jwjeon100419@DS2

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <asm/system.h>

#include <linux/ctype.h> // tolower, isdigit

// 20120623 lcj@LS3 dumping for mac
//#define USE_DUMP_MAC

#define TAG "panmac:"

#define PANMAC_MAJOR    60
#define MAX_MAC_STR_LEN  	(17)
#define MAX_MAC_LEN		(6)
static char mac_str_[MAX_MAC_STR_LEN+1]={""};
module_param_string(mac_addr,mac_str_, sizeof(mac_str_),0640);
MODULE_PARM_DESC(mac_addr,"mac address in string format. e.g. 00:11:22:33:44:55" );

static unsigned char mac_addr_[MAX_MAC_LEN];

int panmac_read_mac(unsigned char* mac, int size) {
	int max = size < MAX_MAC_LEN ? size : MAX_MAC_LEN;
	if ( mac_addr_[0] == 0 
		&& mac_addr_[1] == 0 
		&& mac_addr_[2] == 0 
		&& mac_addr_[3] == 0 
		&& mac_addr_[4] == 0 
		&& mac_addr_[5] == 0 ) {
		printk(KERN_ERR TAG " mac address not initialized yet\n") ;
		return 0 ;
	}

	memcpy(mac, mac_addr_, max) ;
	return max;
}
EXPORT_SYMBOL(panmac_read_mac);

// ascii mac address to mac address
static int a2m(const char* mac_str, unsigned char* mac_addr) 
{
	int a=0;
	char ch=0;
	unsigned char num=0;

	for (a=0; a<MAX_MAC_LEN; ++a) {
		ch = tolower(*mac_str++);
		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			printk(KERN_ERR TAG " invalid char in mac '%c'\n", ch ) ;
			return -1;
		}
		num = (isdigit(ch) ? (ch - '0') : (ch - 'a' + 10))*16;

		ch = tolower(*mac_str++);
		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			printk(KERN_ERR TAG " invalid char in mac '%c'\n", ch ) ;
			return -1;
		}
		num = num + (isdigit(ch) ? (ch - '0') : (ch - 'a' + 10));

		*mac_addr++= num ;

		ch = *mac_str++;
		if( !( ch == ':' || ch == '\0')) {
			printk(KERN_ERR TAG " invalid deliminator '%c'\n", ch); 
			return -2;
		}
	}
	return 0;
}

static const char* m2a(const unsigned char* mac_addr, char* mac_str) {
	sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac_addr[0],
		mac_addr[1],
		mac_addr[2],
		mac_addr[3],
		mac_addr[4],
		mac_addr[5]);
	return mac_str ;
}

#ifdef USE_DUMP_MAC
static void dump_mac_addr(const unsigned char* addr) {
	char temp[MAX_MAC_STR_LEN+1];
	printk(KERN_INFO TAG " dump mac addr=%s\n", m2a( addr, temp));
}
#endif

static int panmac_open(struct inode *inode, struct file *filp)
{
	//printk(KERN_INFO TAG " open\n");

	if( strlen(mac_str_) !=0 ) {
		// update mac address from dd parameter.
		if( a2m(mac_str_, (unsigned char*)mac_addr_)<0) {
			printk(KERN_ERR TAG " invalid mac address %s\n", mac_str_) ;
			return -EINVAL;
		}
	}
#ifdef USE_DUMP_MAC
	dump_mac_addr(mac_addr_) ;
#endif 
	return 0;
}

static int panmac_release(struct inode *inode, struct file *filp)
{
	//printk(KERN_INFO TAG " relelase\n");
	return 0;
}

static ssize_t panmac_read(struct file *filp, char* buf, size_t count, loff_t *f_pos)
{
	char temp[MAX_MAC_STR_LEN+1];
	int copy_len=0 ;

   	//printk(KERN_INFO TAG " read\n") ;

	m2a(mac_addr_, temp);	
  	copy_len = count < MAX_MAC_STR_LEN ? count: MAX_MAC_STR_LEN;
  	if(copy_to_user(buf, temp, copy_len)) {
		printk(KERN_ERR TAG " failed to copy\n");
  		return -EFAULT;
 	} 
	if (*f_pos == 0)
	{
		*f_pos += copy_len;
		return copy_len;
	}
	return 0;
}

static ssize_t panmac_write(struct file* filp, const char* buf, size_t count, loff_t* f_ops) {
	char user[MAX_MAC_STR_LEN+1];
	unsigned char mac_addr[MAX_MAC_LEN];
	int max=0;

   	//printk(KERN_INFO TAG " write count=%d\n",count) ;
	max = count < MAX_MAC_STR_LEN ? count : MAX_MAC_STR_LEN ;

	memset(user, 0, sizeof(user));
	if(copy_from_user((void*)user, buf, max)) {
		printk(KERN_ERR TAG " failed to copy parameter\n");
		return -EFAULT;
	}
	printk(KERN_INFO TAG " mac from user %s\n", (char*)user) ;
	memset(mac_addr, 0, sizeof(mac_addr));
	if(a2m(user, mac_addr)<0) {
		return -EINVAL ;
	}
#ifdef USE_DUMP_MAC
	dump_mac_addr(mac_addr) ;
#endif
	memcpy(mac_addr_, mac_addr, MAX_MAC_LEN) ;
	return max ;
}

struct file_operations panmac_fops = {
	.open = panmac_open,
	.release = panmac_release,
	.read = panmac_read,
	.write = panmac_write,
};

static int panmac_init(void)
{
	int result;
 
	//printk(KERN_INFO TAG " panmac init\n");

	// initialize mac address
	memset(mac_addr_, 0, sizeof(mac_addr_));

	result = register_chrdev(PANMAC_MAJOR, "panmac", &panmac_fops);
	if (result < 0)
	{
		printk(KERN_INFO "Failed to register chrdev (%d).\n", result);
		return result;
	}

  return 0;
}

static void panmac_exit(void)
{
	//printk(KERN_INFO TAG " panmac exit\n");
	unregister_chrdev(PANMAC_MAJOR, "panmac");
}

module_init(panmac_init);
module_exit(panmac_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pantech MAC r/w driver");
