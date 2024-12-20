/*
* simple.c -- a simple module
*
*/
 
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
 
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
 
//#include <asm/system.h>         /* cli(), *_flags */
#include <asm/uaccess.h>        /* copy_*_user */
 
 
/*
* Our parameters which can be set at load time.
*/

 
int simple_major =   0;
int simple_minor =   0;
int memsize	= 255;

module_param(simple_major, int, S_IRUGO);
module_param(simple_minor, int, S_IRUGO);
module_param(memsize, int, S_IRUGO);

MODULE_AUTHOR("Antonio Sgorbissa");
MODULE_LICENSE("Dual BSD/GPL");


struct simple_dev {
         char *data;  /* Pointer to data area */
	 int memsize;
         struct semaphore sem;     /* mutual exclusion semaphore    */
         struct cdev cdev;         /* structure for char devices */
};

 
struct simple_dev simple_device; 



int simple_open(struct inode *inode, struct file *filp)
{
	 struct simple_dev *dev; /* a pointer to a simple_dev structire */
         dev = container_of(inode->i_cdev, struct simple_dev, cdev);

	 // update private data pointer to be re-used in other system call

         return 0;           
}


int simple_release(struct inode *inode, struct file *filp)
{
         return 0;
}


ssize_t simple_read(struct file *filp, char __user *buf, size_t count,
                 loff_t *f_pos)
{
	 // declare and fill a simple_dev structure that points to private_data

         ssize_t retval = 0;

	// take the semaphore

        // if count is bigger that memsize saturate it
	
	// copy the memory from kernel memory to user memory in buf

	// update retval

	// free the semaphore

return retval;
}
 

ssize_t simple_write(struct file *filp, const char __user *buf, size_t count,
                 loff_t *f_pos)
{
	 // declare and fill a simple_dev structure that points to private_data

        ssize_t retval = 0; /* return value */
 
        // take semaphore
 
        // if count is bigger that memsize saturate it

	// copy the memory from user memory in buf to kernel memory 

         // update retval

	// free the semaphore
	
       	return retval;
}

struct file_operations simple_fops = {
         .owner =    THIS_MODULE,
         .read =     simple_read,
         .write =    simple_write,
         .open =     simple_open,
         .release =  simple_release,
};

void simple_cleanup_module(void)
{
 	// fill a dev_t variable with major and minor number
 
        // tell the kernel to remove the cden
 
	// free the memory for data exchange

	// Notify the kernel that the device with the corresponding major and minor number is no more present
}

int simple_init_module(void)
{
         int result, err;
         dev_t dev = 0;
 

	// if the major number has been passed as an input parameter, register the device

	// otherwise allocate the device in order to receive a free major number


	//copy the parameter memsize in the structure simple_dev

	//allocate memory for data exchange

	// Initialize semaphore 

	// Inizialize cdev 

	// Set the corresponding file operations

	// Notify to the kernel the presence of cdev

	// Print in the log the corresponding major and minor number

        return 0; 

}
module_init(simple_init_module);
module_exit(simple_cleanup_module);
