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
#include <linux/sched.h>

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
 
//Now we have to variable of type struct simple dev for two different devices
struct simple_dev simple_device[2]; 

//we also need two wait queues and flags
///declare wait queue
int flag[2];
DECLARE_WAIT_QUEUE_HEAD(my_queue0);
DECLARE_WAIT_QUEUE_HEAD(my_queue1);


int simple_open0(struct inode *inode, struct file *filp)
{
         struct simple_dev *dev; 	/* a pointer to a simple_dev structire */
 
         dev = container_of(inode->i_cdev, struct simple_dev, cdev);
         filp->private_data = dev; /* stored here to be re-used in other system call*/
 
         return 0;           
}

int simple_open1(struct inode *inode, struct file *filp)
{
         struct simple_dev *dev; 	/* a pointer to a simple_dev structire */
 
         dev = container_of(inode->i_cdev, struct simple_dev, cdev);
         filp->private_data = dev; /* stored here to be re-used in other system call*/
 
         return 0;           
}


int simple_release0(struct inode *inode, struct file *filp)
{
         return 0;
}

int simple_release1(struct inode *inode, struct file *filp)
{
         return 0;
}


ssize_t simple_read0(struct file *filp, char __user *buf, size_t count,
                 loff_t *f_pos)
{
         struct simple_dev *dev = filp->private_data; 

         ssize_t retval = 0;

	printk(KERN_NOTICE "/dev/simple");
	 //if in blocking mode
	 //wait on the queue
	printk(KERN_NOTICE "FLAG %d", filp->f_flags & O_NONBLOCK);
	 if ((filp->f_flags & O_NONBLOCK )== 0)
	 {
	 	wait_event_interruptible(my_queue0, flag[0]==1);
	 	flag[0]=0;
	 }

         if (down_interruptible(&dev->sem))
                 return -ERESTARTSYS;
         if (count >= dev->memsize) 
                 goto out;
	    

         if (copy_to_user(buf, dev->data, count)) {
                 retval = -EFAULT;
                 goto out;
         }
         retval = count;

	 out:
	        up(&dev->sem);
		return retval;
}

ssize_t simple_read1(struct file *filp, char __user *buf, size_t count,
                 loff_t *f_pos)
{
         struct simple_dev *dev = filp->private_data; 

         ssize_t retval = 0;

	printk(KERN_NOTICE "/dev/simple1");

	 //if in blocking mode
	 //wait on the queue
	printk(KERN_NOTICE "FLAG %d", filp->f_flags & O_NONBLOCK);
	 if ((filp->f_flags & O_NONBLOCK )== 0)
	 {
	 	wait_event_interruptible(my_queue1, flag[1]==1);
	 	flag[1]=0;
	 }

         if (down_interruptible(&dev->sem))
                 return -ERESTARTSYS;
         if (count >= dev->memsize) 
                 goto out;
	    

         if (copy_to_user(buf, dev->data, count)) {
                 retval = -EFAULT;
                 goto out;
         }
         retval = count;

	 out:
	        up(&dev->sem);
		return retval;
}
 

ssize_t simple_write0(struct file *filp, const char __user *buf, size_t count,
                 loff_t *f_pos)
{
         struct simple_dev *dev = filp->private_data;
         ssize_t retval = 0; /* return value */
 
         if (down_interruptible(&dev->sem))
                 return -ERESTARTSYS;

	 if (count >= dev->memsize) 
                 count = dev->memsize;

         if (copy_from_user(dev->data, buf, count)) {
                 retval = -EFAULT;
                 goto out;
         }
         retval = count;

	out:
         	up(&dev->sem);

//here I am waking up the read
		flag[0]=1;
		wake_up_interruptible(&my_queue0);
         	return retval;
}

ssize_t simple_write1(struct file *filp, const char __user *buf, size_t count,
                 loff_t *f_pos)
{
         struct simple_dev *dev = filp->private_data;
         ssize_t retval = 0; /* return value */
 
         if (down_interruptible(&dev->sem))
                 return -ERESTARTSYS;

	 if (count >= dev->memsize) 
                 count = dev->memsize;

         if (copy_from_user(dev->data, buf, count)) {
                 retval = -EFAULT;
                 goto out;
         }
         retval = count;

	out:
         	up(&dev->sem);

//here I am waking up the read
		flag[1]=1;
		wake_up_interruptible(&my_queue1);
         	return retval;
}

//We now need two sets of file operations, one set for each minor number/device
struct file_operations simple_fops0 = {
         .owner =    THIS_MODULE,
         .read =     simple_read0,
         .write =    simple_write0,
         .open =     simple_open0,
         .release =  simple_release0,
};

struct file_operations simple_fops1 = {
         .owner =    THIS_MODULE,
         .read =     simple_read1,
         .write =    simple_write1,
         .open =     simple_open1,
         .release =  simple_release1,
};

void simple_cleanup_module(void)
{
         dev_t devno = MKDEV(simple_major, simple_minor);
 
	 //Now we have two cdev to remove
         /* Free the cdev entries  */
         cdev_del(&simple_device[0].cdev);
         cdev_del(&simple_device[1].cdev);

	 /* Free the memory */
         kfree(simple_device[0].data);
         kfree(simple_device[1].data);

	 unregister_chrdev_region(devno, 2);
}

int simple_init_module(void)
{
         int result, err;
         dev_t dev = 0;


	 if (simple_major) {   //the major number is given as a parameter
	            dev = MKDEV(simple_major, simple_minor);
	
	//Now we register two minor numbers
	            result = register_chrdev_region(dev, 2, "simple");
	 } 
	 else {		// otherwise

	//Now we register two minor numbers
                 result = alloc_chrdev_region(&dev, simple_minor, 2, "simple");
                 simple_major = MAJOR(dev);
	 }
	if (result < 0) {
                 printk(KERN_WARNING "simple: can't get major %d\n", simple_major);
                 return result;
        }


	//Now we prepare the memory areas for the two different areas 
	//simple_device[0] and simple_device[1]

	/* Prepare the memory area */
	simple_device[0].memsize = memsize;
	simple_device[0].data = kmalloc(memsize * sizeof(char), GFP_KERNEL);
        memset(simple_device[0].data, 0, memsize * sizeof(char));

        /* Initialize the semaphore */
        sema_init(&simple_device[0].sem,1);

	/* Prepare the memory area */
	simple_device[1].memsize = memsize;
	simple_device[1].data = kmalloc(memsize * sizeof(char), GFP_KERNEL);
        memset(simple_device[1].data, 0, memsize * sizeof(char));

        /* Initialize the semaphore */
        sema_init(&simple_device[1].sem,1);


	//Now we initialize cdev for the two memory areas
	/* Initialize cdev */
        cdev_init(&simple_device[0].cdev, &simple_fops0);
        simple_device[0].cdev.owner = THIS_MODULE;
        simple_device[0].cdev.ops = &simple_fops0;
        err = cdev_add (&simple_device[0].cdev, dev, 1);

        cdev_init(&simple_device[1].cdev, &simple_fops1);
        simple_device[1].cdev.owner = THIS_MODULE;
        simple_device[1].cdev.ops = &simple_fops1;

        err = cdev_add (&simple_device[1].cdev, MKDEV(simple_major, simple_minor+1), 1);

        if (err)  printk(KERN_NOTICE "Error %d adding simple", err);
	else
	 {
        	printk(KERN_NOTICE "Simple Added major: %d minor: %d", simple_major, simple_minor);
        	printk(KERN_NOTICE "Simple Added major: %d minor: %d", simple_major, simple_minor+1);

        }
 

//	initialize queue and flag
	flag[0]=0;
	flag[1]=0;
	//init_waitqueue_head(&my_queue);
        return 0; 

}
module_init(simple_init_module);
module_exit(simple_cleanup_module);
