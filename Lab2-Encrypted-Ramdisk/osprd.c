#include <linux/version.h>
#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/random.h>

#include <linux/sched.h>
#include <linux/kernel.h>  /* printk() */
#include <linux/errno.h>   /* error codes */
#include <linux/types.h>   /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/wait.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#include <asm/string.h>

#include "spinlock.h"
#include "osprd.h"

/* The size of an OSPRD sector. */
#define SECTOR_SIZE	512

/* This flag is added to an OSPRD file's f_flags to indicate that the file
 * is locked. */
#define F_OSPRD_LOCKED	0x80000

/* eprintk() prints messages to the console.
 * (If working on a real Linux machine, change KERN_NOTICE to KERN_ALERT or
 * KERN_EMERG so that you are sure to see the messages.  By default, the
 * kernel does not print all messages to the console.  Levels like KERN_ALERT
 * and KERN_EMERG will make sure that you will see messages.) */
#define eprintk(format, ...) printk(KERN_NOTICE format, ## __VA_ARGS__)
#define FILL_SG(sg, ptr, len)   do {(sg)->page = virt_to_page(ptr); (sg)->offset = offset_in_page(ptr); (sg)->length = len;} while(0)

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("CS 111 RAM Disk");
// EXERCISE: Pass your names into the kernel as the module's authors.
MODULE_AUTHOR("Andrew Lee & Michelle Chang");

#define OSPRD_MAJOR	222

/* This module parameter controls how big the disk will be.
 * You can specify module parameters when you load the module,
 * as an argument to insmod: "insmod osprd.ko nsectors=4096" */
static int nsectors = 32;
module_param(nsectors, int, 0);

struct num_list {
  unsigned ticket;
  struct num_list* next;
};

/* The internal representation of our device. */
typedef struct osprd_info {
	uint8_t *data;                  // The data array. Its size is
	                                // (nsectors * SECTOR_SIZE) bytes.

	osp_spinlock_t mutex;           // Mutex for synchronizing access to
					// this block device

	unsigned ticket_head;		// Currently running ticket for
					// the device lock

	unsigned ticket_tail;		// Next available ticket for
					// the device lock
        unsigned num_read;
        unsigned num_write;

        pid_t write_pid;

        struct num_list *killed_list;
        struct num_list* pid_list;
        struct num_list* auth_list;
        char passwd[MAX_PASS_LEN];
        char key[KEY_LENGTH];
        char written_sectors[32];
  //    struct crypto_tfm * tfm;
  //char aes_key[AES_KEY_LENGTH];
  //    char init_vector[AES_KEY_LENGTH];
	wait_queue_head_t blockq;       // Wait queue for tasks blocked on
					// the device lock
	/* HINT: You may want to add additional fields to help
	         in detecting deadlock. */

	// The following elements are used internally; you don't need
	// to understand them.
	struct request_queue *queue;    // The device request queue.
	spinlock_t qlock;		// Used internally for mutual
	                                //   exclusion in the 'queue'.
	struct gendisk *gd;             // The generic disk.
} osprd_info_t;

#define NOSPRD 4
static osprd_info_t osprds[NOSPRD];


// Declare useful helper functions

/*
 * file2osprd(filp)
 *   Given an open file, check whether that file corresponds to an OSP ramdisk.
 *   If so, return a pointer to the ramdisk's osprd_info_t.
 *   If not, return NULL.
 */
static osprd_info_t *file2osprd(struct file *filp);

/*
 * for_each_open_file(task, callback, user_data)
 *   Given a task, call the function 'callback' once for each of 'task's open
 *   files.  'callback' is called as 'callback(filp, user_data)'; 'filp' is
 *   the open file, and 'user_data' is copied from for_each_open_file's third
 *   argument.
 */
static void for_each_open_file(struct task_struct *task,
			       void (*callback)(struct file *filp,
						osprd_info_t *user_data),
			       osprd_info_t *user_data);

/*
 * osprd_process_request(d, req)
 *   Called when the user reads or writes a sector.
 *   Should perform the read or write, as appropriate.
 */
static void osprd_process_request(osprd_info_t *d, struct request *req)
{
        int sector_beg, write_size;
	//    int current_write;
	if (!blk_fs_request(req) || req->sector < 0 
	    || req->sector > nsectors) {
		end_request(req, 0);
		return;
	}

	// EXERCISE: Perform the read or write request by copying data between
	// our data array and the request's buffer.
	// Hint: The 'struct request' argument tells you what kind of request
	// this is, and which sectors are being read or written.
	// Read about 'struct request' in <linux/blkdev.h>.
	// Consider the 'req->sector', 'req->current_nr_sectors', and
	// 'req->buffer' members, and the rq_data_dir() function.
	sector_beg = req->sector  * SECTOR_SIZE;
	write_size = SECTOR_SIZE * req->current_nr_sectors;

	if(rq_data_dir(req) == READ)
	  {
	    memcpy(req->buffer, &(d->data[sector_beg]), write_size);
	    /*for(current_write = 0; current_write < write_size; current_write += KEY_LENGTH)
	      AxorB(req->buffer, d->key, 0, KEY_LENGTH, 0);*/

	  }
	else if(rq_data_dir(req) == WRITE)
	  {
	    /*for(current_write = 0; current_write < write_size; current_write += KEY_LENGTH)
	      AxorB(req->buffer, d->key, 0, KEY_LENGTH, 0);*/
	    memcpy(&(d->data[sector_beg]), req->buffer, write_size);
	  }
	else
	  {
	    end_request(req, 0);
	  }
	// Your code here.
	//	eprintk("Should process request...\n");

	end_request(req, 1);
}


// This function is called when a /dev/osprdX file is opened.
// You aren't likely to need to change this.
static int osprd_open(struct inode *inode, struct file *filp)
{
	// Always set the O_SYNC flag. That way, we will get writes immediately
	// instead of waiting for them to get through write-back caches.
	filp->f_flags |= O_SYNC;
	return 0;
}

int osprd_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg);

// This function is called when a /dev/osprdX file is finally closed.
// (If the file descriptor was dup2ed, this function is called only when the
// last copy is closed.)
static int osprd_close_last(struct inode *inode, struct file *filp)
{
	if (filp) {
	   	osprd_info_t *d = file2osprd(filp);
	        int filp_writable = filp->f_mode & FMODE_WRITE;

		// EXERCISE: If the user closes a ramdisk file that holds
		// a lock, release the lock.  Also wake up blocked processes
		// as appropriate.

		// Your code here.
		
		osprd_ioctl(inode, filp, OSPRDIOCRELEASE, 0);
		// This line avoids compiler warnings; you may remove it.
		(void) filp_writable, (void) d;

	}

	return 0;
}

/*
 * osprd_lock
 */

/*
 * osprd_ioctl(inode, filp, cmd, arg)
 *   Called to perform an ioctl on the named file.
 */
int osprd_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	osprd_info_t *d = file2osprd(filp);	// device info
	int r = 0;			// return value: initially 0
	unsigned cur_ticket;
	struct num_list* temp = NULL;
	struct num_list* a = NULL;
	// is file open for writing?
	int filp_writable = (filp->f_mode & FMODE_WRITE) != 0;

	// This line avoids compiler warnings; you may remove it.
	//	(void) filp_writable, (void) d;

	// Set 'r' to the ioctl's return value: 0 on success, negative on error

	if (cmd == OSPRDIOCACQUIRE) {
                osp_spin_lock(&d->mutex);
		cur_ticket = d->ticket_head;
	        d->ticket_head++;
		if(d->write_pid == current->pid)
		{
		    d->ticket_head--;
		    osp_spin_unlock(&d->mutex);
		    return -EDEADLK;
		}

	      temp = (struct num_list*) kmalloc(sizeof(struct num_list), GFP_ATOMIC);
	      temp->ticket = cur_ticket;
	      temp->next = NULL;
     		if(filp_writable)
		{
			a = d->pid_list;			
			while(a != NULL && a->ticket != current->pid)
			  a = a->next;
			if(a != NULL && a->ticket == current->pid)
			{
			  kfree(temp);
			  d->ticket_head--;
			  osp_spin_unlock(&d->mutex);
			  return -EDEADLK;
			}

		        if(d->num_write != 0 
			  || d->num_read != 0 || cur_ticket != d->ticket_tail) 
			 {
			   osp_spin_unlock(&d->mutex);
			     r = wait_event_interruptible(d->blockq,
			       d->num_write == 0 && d->num_read == 0
				&& d->ticket_tail == cur_ticket
				);
			     osp_spin_lock(&d->mutex);
			 }
			if(r < 0) {
			  if(cur_ticket == d->ticket_tail){
			    kfree(temp);
			    d->ticket_tail++;
			  }
			  else if(d->killed_list == NULL){
			     d->killed_list = temp;
			   }
			   else {
			     struct num_list* prev = d->killed_list;
			     a = d->killed_list->next;
			     while(a != NULL && a->ticket < cur_ticket)
			       {
				 prev = a;
				 a = a->next;
			       }
			     if(a == NULL)
			       {
				 if(cur_ticket < prev->ticket)
				 {
				   temp->next = prev;
				   d->killed_list = temp;
				 }
				 else
				   prev->next = temp;
			       }
			     else
			     {
			       temp->next = a;
			       prev->next = temp;
			     }
			   }
			   osp_spin_unlock(&d->mutex);
			   
			 return r;
			}
			d->ticket_tail++;
			d->num_write++;
			d->write_pid = current->pid;
			filp->f_flags |= F_OSPRD_LOCKED;
			a = d->killed_list;
			while(a != NULL)
			  {
			    if(a->ticket == d->ticket_tail) {
			      d->ticket_tail++;
			      a = a->next;
			      kfree(d->killed_list);
			      d->killed_list = a;
			    }
			    else if(a->ticket < d->ticket_tail)
			    {
			      a = a->next;
			      kfree(d->killed_list);
			      d->killed_list = a;
			    }

			    else
			      a = a->next;
			  }
			osp_spin_unlock(&d->mutex);
			kfree(temp);
		}
		else
		{
		        if(d->num_write != 0 || cur_ticket != d->ticket_tail ) {
			  osp_spin_unlock(&d->mutex);
			  r = wait_event_interruptible(d->blockq, 
			     d->num_write == 0 
			      && d->ticket_tail == cur_ticket);
			  osp_spin_lock(&d->mutex);
			}
			if (r < 0){
			  if(cur_ticket == d->ticket_tail)
			  {
  			    d->ticket_tail++;
			    kfree(temp);
			  }
			  else if(d->killed_list == NULL){
			     d->killed_list = temp;
			   }
			  else {
			     struct num_list* prev = d->killed_list;
			     a = d->killed_list->next;
			     while(a != NULL && a->ticket < cur_ticket)
			       {
				 prev = a;
				 a = a->next;
			       }
			     if(a == NULL)
			       {
				 if(cur_ticket < prev->ticket)
				 {
				   temp->next = prev;
				   d->killed_list = temp;
				 }
				 else
				   prev->next = temp;
			       }
			     else
			     {
			       temp->next = a;
			       prev->next = temp;
			     }
			   }
			  osp_spin_unlock(&d->mutex);
			  return r;
			}			
			a = d->pid_list;
			struct num_list* pid_node;
			pid_node = (struct num_list*) kmalloc(sizeof(struct num_list), GFP_ATOMIC);
		        pid_node->ticket = current->pid;
  		        pid_node->next = NULL;
			while(a != NULL && a->ticket != current->pid && a->next != NULL)
			  a = a->next;
			if(a == NULL) {
			  d->pid_list = pid_node;
			}
			else
			{
			  a->next = pid_node;
			}
			d->ticket_tail++;
			d->num_read++;
			filp->f_flags |= F_OSPRD_LOCKED;
			a = d->killed_list;
			while(a != NULL)
			  {
			    if(a->ticket == d->ticket_tail) {
			      d->ticket_tail++;
			      a = a->next;
			      kfree(d->killed_list);
			      d->killed_list = a;
			    }
			    else if(a->ticket < d->ticket_tail)
			    {
			      a = a->next;
			      kfree(d->killed_list);
			      d->killed_list = a;
			    }
			    else
			      a = a->next;
			  }
			osp_spin_unlock(&d->mutex);
			kfree(temp);
		}
		// EXERCISE: Lock the ramdisk.
		//
		// If *filp is open for writing (filp_writable), then attempt
		// to write-lock the ramdisk; otherwise attempt to read-lock
		// the ramdisk.
		//
                // This lock request must block using 'd->blockq' until:
		// 1) no other process holds a write lock;
		// 2) either the request is for a read lock, or no other process
		//    holds a read lock; and
		// 3) lock requests should be serviced in order, so no process
		//    that blocked earlier is still blocked waiting for the
		//    lock.
		//
		// If a process acquires a lock, mark this fact by setting
		// 'filp->f_flags |= F_OSPRD_LOCKED'.  You also need to
		// keep track of how many read and write locks are held:
		// change the 'osprd_info_t' structure to do this.
		//
		// Also wake up processes waiting on 'd->blockq' as needed.
		//
		// If the lock request would cause a deadlock, return -EDEADLK.
		// If the lock request blocks and is awoken by a signal, then
		// return -ERESTARTSYS.
		// Otherwise, if we can grant the lock request, return 0.

		// 'd->ticket_head' and 'd->ticket_tail' should help you
		// service lock requests in order.  These implement a ticket
		// order: 'ticket_tail' is the next ticket, and 'ticket_head'
		// is the ticket currently being served.  You should set a local
		// variable to 'd->ticket_head' and increment 'd->ticket_head'.
		// Then, block at least until 'd->ticket_tail == local_ticket'.
		// (Some of these operations are in a critical section and must
		// be protected by a spinlock; which ones?)

		// Your code here (instead of the next two lines).

		//		r = -ENOTTY;

	} else if (cmd == OSPRDIOCTRYACQUIRE) {

		// EXERCISE: ATTEMPT to lock the ramdisk.
		//
		// This is just like OSPRDIOCACQUIRE, except it should never
		// block.  If OSPRDIOCACQUIRE would block or return deadlock,
		// OSPRDIOCTRYACQUIRE should return -EBUSY.
		// Otherwise, if we can grant the lock request, return 0.
	  		//eprintk("Attempting to try acquire\n");
		// Your code here (instead of the next two lines).
	       	if(filp_writable)
		{
		       if(d->num_write != 0 
			  || d->num_read != 0) 
			 return -EBUSY;
		       	osp_spin_lock(&d->mutex);
			//eprintk("Incrementing write\n");
			d->num_write++;
			filp->f_flags |= F_OSPRD_LOCKED;
			osp_spin_unlock(&d->mutex);
		}
		else
		{
		        if(d->num_write != 0)
			  return -EBUSY;
			osp_spin_lock(&d->mutex);
			//eprintk("Incrementing read\n");
			d->num_read++;
			filp->f_flags |= F_OSPRD_LOCKED;
			osp_spin_unlock(&d->mutex);
		}
	  //	r = -ENOTTY;

	} else if (cmd == OSPRDIOCRELEASE) {

		// EXERCISE: Unlock the ramdisk.
		//
		// If the file hasn't locked the ramdisk, return -EINVAL.
		// Otherwise, clear the lock from filp->f_flags, wake up
		// the wait queue, perform any additional accounting steps
		// you need, and return 0.

		// Your code here (instead of the next line).
	  //		r = -ENOTTY;
		//eprintk("Releasing\n");
		if ((filp->f_flags & F_OSPRD_LOCKED) == 0) {
		    //eprintk("Check failed!\n");
		  return -EINVAL;
		}
		//		//eprintk("Check passed\n");
		if(filp_writable)
		  {
		    //eprintk("Release write: locking mutex\n");
		    osp_spin_lock(&d->mutex);
		    //eprintk("Release write: locked mutex\n");
		    d->num_write = 0;
		    d->write_pid = -1;
		    if(d->num_read == 0)
		    filp->f_flags ^= F_OSPRD_LOCKED;

		    osp_spin_unlock(&d->mutex);
		    //eprintk("Release write: unlocked mutex, write is now %u.\n", d->num_write);
		  }
		else
		  {
		    //eprintk("Locking release read mutex\n");
		    osp_spin_lock(&d->mutex);
		    //eprintk("Trying to decrement read: %u\n", d->num_read);
		    if(d->num_read)
		      d->num_read--;
		    if(d->num_read == 0 && d->num_write == 0)
		      filp->f_flags ^= F_OSPRD_LOCKED;
		    struct num_list* prev = NULL;
		    a = d->pid_list;
		    while(a != NULL && a->ticket != current->pid)
		    {
		      prev = a;
		      a = a->next;
		    }
		    if(a == NULL)
		      ;
		    else if(prev == NULL)
		    {
		      d->pid_list = a->next;
		      kfree(a);
		    }
		    else if(a->next == NULL)
		    {
		      prev->next = NULL;
		      kfree(a);
		    }
		    else
		    {
		      prev->next = a->next;
		      kfree(a);
		    }
		      osp_spin_unlock(&d->mutex);
		  }
		wake_up_all(&d->blockq);
	}
	else if(cmd == OSPRDSETPASS)
	  {
	    
	    if(arg == 0)
	      r = -ENOTTY;
	    else
	      {
                char* buf = (char*) kmalloc(MAX_PASS_LEN, GFP_ATOMIC);
		r = copy_from_user(buf, (const char __user*) arg, MAX_PASS_LEN);
	        if(r) {
		  kfree(buf);
		  return -EFAULT;
		}
		osp_spin_lock(&d->mutex);
		if(buf[MAX_PASS_LEN - 1] == '\0')
		  memcpy(d->passwd, buf, MAX_PASS_LEN);
                osp_spin_unlock(&d->mutex);
		//		eprintk("Return value is %d, the string is %s\n", r, (char*) arg);
		kfree(buf);
	      }
	  }
	else if(cmd == OSPRDAUTHORIZE)
	  {
	    char* buf = (char*) kmalloc(MAX_PASS_LEN, GFP_ATOMIC);
	    int flag = 0;
	    if(buf == NULL)
	      return -ENOMEM;
	    r = copy_from_user(buf, (const char __user*) arg, MAX_PASS_LEN);
	    if(r != 0) {
	      kfree(buf);
	      return -EFAULT;
	    }
	    osp_spin_lock(&d->mutex);
	    flag = strcmp(buf, d->passwd);
	    osp_spin_unlock(&d->mutex);
	    if(flag)
	    {
	      buf[MAX_PASS_LEN - 1] = '8';
	      r = copy_to_user((char __user*) arg, buf, MAX_PASS_LEN);
	      if(r != 0)
	      {
		kfree(buf);
		return -EFAULT;
	      }
	    }
	    if(!flag)
	    {
	      //                eprintk("Adding a pid\n");
		temp = (struct num_list*) kmalloc(sizeof(struct num_list), GFP_ATOMIC);
		temp->ticket = current->pid;
		temp->next = NULL;
		osp_spin_lock(&d->mutex);
		a = d->auth_list;
		while(a != NULL && a->next != NULL && a->ticket != current->pid)
		  a = a->next;
		if(a == NULL)
		  d->auth_list = temp;
		else if(a->next == NULL)
		  if(a->ticket != current->pid)
		    a->next = temp;
		  else
		    kfree(temp);
		else
		  kfree(temp);
		osp_spin_unlock(&d->mutex);
	    }
	    kfree(buf);
	  }
	else if(cmd == OSPRDDEAUTHORIZE)
	  {
	    osp_spin_lock(&d->mutex);
	    temp = NULL;
	    a = d->auth_list;
	    while(a != NULL && a->ticket != current->pid)
	    {
	      temp = a;
	      a = a->next;
	    }
	    if(a == NULL)
	      ;
	    else if(temp == NULL)
	    {
	      d->auth_list = a->next;
	      //eprintk("Auth free\n");
	      kfree(a);
	    }
	    else if(a->next == NULL)
	    {
	      temp->next = NULL;
	      //eprintk("Auth free\n");
	      kfree(a);
	    }
	    else
	    {
	      temp->next = a->next;
	      //eprintk("Auth free\n");
	      kfree(a);
	    }
	    osp_spin_unlock(&d->mutex);
	  }
	else if(cmd == OSPRDPASSEXISTS)
	{
	   r = strlen(d->passwd);
	   r = copy_to_user((char __user*) arg, &r, sizeof(int));
	   if(r != 0)
	     return -EBADE;
	}
	else
		r = -ENOTTY; /* unknown command */
	return r;
}


// Initialize internal fields for an osprd_info_t.

static void osprd_setup(osprd_info_t *d)
{
	/* Initialize the wait queue. */
	init_waitqueue_head(&d->blockq);
	osp_spin_lock_init(&d->mutex);
	d->ticket_head = d->ticket_tail = 0;
	d->num_read = 0;
	d->num_write = 0;
	d->write_pid = -1;
	d->killed_list = NULL;
	d->pid_list = NULL;
	d->auth_list = NULL;
	get_random_bytes(d->key, KEY_LENGTH);
	d->passwd[0] = '\0';
	memset(d->written_sectors, 0, 32);
	/* Add code here if you add fields to osprd_info_t. */
}


/*****************************************************************************/
/*         THERE IS NO NEED TO UNDERSTAND ANY CODE BELOW THIS LINE!          */
/*                                                                           */
/*****************************************************************************/

// Process a list of requests for a osprd_info_t.
// Calls osprd_process_request for each element of the queue.

static void osprd_process_request_queue(request_queue_t *q)
{
	osprd_info_t *d = (osprd_info_t *) q->queuedata;
	struct request *req;

	while ((req = elv_next_request(q)) != NULL)
		osprd_process_request(d, req);
}

static void AxorB(char* a, char* b, int offset, unsigned size, int add)
{
  int i;
  for(i = 0; i < size; i++)
  {
    a[i] ^= b[i + offset];
    a[i] += add;
  }
}

// Some particularly horrible stuff to get around some Linux issues:
// the Linux block device interface doesn't let a block device find out
// which file has been closed.  We need this information.

static struct file_operations osprd_blk_fops;
static int (*blkdev_release)(struct inode *, struct file *);
static ssize_t (*blkdev_read)(struct file *, char __user *, size_t, loff_t *);
static ssize_t (*blkdev_write)(struct file *, const char __user *, size_t, loff_t *);
static int _osprd_release(struct inode *inode, struct file *filp)
{
	if (file2osprd(filp))
		osprd_close_last(inode, filp);
	return (*blkdev_release)(inode, filp);
}

static void new_key(osprd_info_t* d, struct file* filp)
{
  int read_ret, i, j;
  char rand_key[KEY_LENGTH];
  char* stuff_buf;
  loff_t offset = 0;
  mm_segment_t oldfs = get_fs();
  get_random_bytes(rand_key, KEY_LENGTH);
  set_fs(get_ds());
  stuff_buf = (char*) kmalloc(KEY_LENGTH, GFP_USER);
  if(stuff_buf == NULL)
  {
    eprintk("Stuff buf allocation failed!\n");
    return;
  }
  for(i = 0; i < 32; i++)
  {
    for(j = 0; j < 8; j++)
    {
      if ((d->written_sectors[i] & (1 << j))) {
	offset = (i * 8 + j) * KEY_LENGTH;
	//eprintk("Recrypting sector at position %lld\n", offset);
        read_ret =(*blkdev_read)(filp, stuff_buf, KEY_LENGTH, &offset);
	offset = (i* 8 + j) * KEY_LENGTH;
	if(read_ret >= 0)
	{
	  AxorB(stuff_buf, d->key, 0, KEY_LENGTH, 0);
	  AxorB(stuff_buf, rand_key, 0, KEY_LENGTH, 0);
	  read_ret = (*blkdev_write)(filp, stuff_buf, KEY_LENGTH, &offset);
	  if(read_ret < 0)
	    break;
	}
	else
	  break;
      }
    }
  }
  kfree(stuff_buf);
  set_fs(oldfs);
  memcpy(d->key, rand_key, KEY_LENGTH);
}

static ssize_t _osprd_read(struct file * filp, char __user * usr, size_t size, loff_t * loff)
{
  char* buf;
  int copy_ret;
  loff_t old_off = *loff;
  unsigned long sec_offset = old_off % KEY_LENGTH;
  long sec_size = size;
  unsigned to_write = KEY_LENGTH - sec_offset;
  osprd_info_t *d = file2osprd(filp);
  struct num_list* node;
  ssize_t ret;
  if(!d)
      return (*blkdev_read)(filp, usr, size, loff);

  //Check for authentication
  osp_spin_lock(&d->mutex);
  node = d->auth_list;
  while(node != NULL && node->ticket != current->pid)
    node = node->next;
  osp_spin_unlock(&d->mutex);
  if(node == NULL)
  {
    new_key(d, filp);
    return (*blkdev_read)(filp, usr, size, loff);
  }
  buf = (char*) kmalloc(size, GFP_KERNEL);
  if(buf == NULL)
    return -ENOMEM;
  ret = (*blkdev_read)(filp, usr, size, loff);
  copy_ret = copy_from_user(buf, usr, size);
  if(copy_ret < 0)
    {
      kfree(buf);
      return copy_ret;
    }

  //Encrypt by XORing plain text with key
  if(sec_offset)
  {
    AxorB(buf, d->key, sec_offset, to_write, 0);
    sec_size -= to_write;
  }
  sec_offset = sec_size / KEY_LENGTH;
  while(sec_size > 0)
  {
    to_write = sec_offset ? KEY_LENGTH : sec_size;
    AxorB(buf + (size - sec_size), d->key, 0, to_write, 0);
    sec_size -= to_write;
    sec_offset--;
  }
  copy_ret = copy_to_user(usr, buf, size);
  kfree(buf);
  if(copy_ret)
    return -1;

  //After every read, get a new encryption key, to make sure the
  //data is not susceptible to a "known-plaintext attack"
  new_key(d, filp);
  return ret;
}
static ssize_t _osprd_write(struct file * filp, const char __user * usr, size_t size, loff_t * loff)
{
  char* buf;
  char empty[KEY_LENGTH];
  int copy_ret;
  loff_t old_off = *loff;
  unsigned long sec_offset = old_off % KEY_LENGTH;
  long sec_size = size;
  unsigned to_write = KEY_LENGTH - sec_offset;

  osprd_info_t *d = file2osprd(filp);
  if(!d)
    return (*blkdev_write)(filp, usr, size, loff);
  memset(empty, 0, KEY_LENGTH);

  // Get data from user buffer to write into disk
  buf = (char*) kmalloc(size, GFP_KERNEL);
  if(buf == NULL)
    return -ENOMEM;
  copy_ret = copy_from_user(buf, usr, size);
  if(copy_ret)
  {
    kfree(buf);
    return -1;
  }
  
  // Encrypt plain text by using XOR to obtain cipher text
  // First, encrypt misaligned data in beginning of buffer
  copy_ret = old_off / KEY_LENGTH;
  if(sec_offset)
  {
    if(memcmp(buf, empty, to_write)) {
      d->written_sectors[copy_ret / 8] |= 1 << (copy_ret % 8);
      //eprintk("Sector was written: Divide is %d, Mod is %d\n", copy_ret / 8, copy_ret % 8);
      copy_ret++;
    }
    AxorB(buf, d->key, sec_offset, to_write, 0);
    sec_size -= to_write;
  }
  sec_offset = sec_size / KEY_LENGTH;

  //Now, encrypt the rest of the data that is aligned with the block
  //size. Along the way, keep track of what sectors have been written
  while(sec_size > 0)
  {
    to_write = sec_offset ? KEY_LENGTH : sec_size;
    if(memcmp(buf + (size - sec_size), empty, to_write)) {
      d->written_sectors[copy_ret / 8] |= 1 << (copy_ret % 8);
      //eprintk("Sector was written: Divide is %d, Mod is %d\n", copy_ret / 8, copy_ret % 8);
    }
    else if(sec_offset || to_write == KEY_LENGTH) {
      d->written_sectors[copy_ret / 8] &= ~(1 << copy_ret % 8);
      //eprintk("Sector was cleared: Divide is %d, Mod is %d\n", copy_ret / 8, copy_ret % 8);
    }
    AxorB(buf + (size - sec_size), d->key, 0, to_write, 0);
    sec_size -= to_write;
    sec_offset--;
    copy_ret++;
  }
  //eprintk("Out of loop\n");
  copy_ret = copy_to_user(usr, buf, size);
  kfree(buf);
  if(copy_ret)
    return -1;
  return (*blkdev_write)(filp, usr, size, loff);
}
static int _osprd_open(struct inode *inode, struct file *filp)
{
	if (!osprd_blk_fops.open) {
		memcpy(&osprd_blk_fops, filp->f_op, sizeof(osprd_blk_fops));
		blkdev_release = osprd_blk_fops.release;
		blkdev_read = osprd_blk_fops.read;
		blkdev_write = osprd_blk_fops.write;
		osprd_blk_fops.release = _osprd_release;
		osprd_blk_fops.read = _osprd_read;
		osprd_blk_fops.write = _osprd_write;
	}
	filp->f_op = &osprd_blk_fops;
	return osprd_open(inode, filp);
}

// The device operations structure.

static struct block_device_operations osprd_ops = {
	.owner = THIS_MODULE,
	.open = _osprd_open,
	// .release = osprd_release, // we must call our own release
	.ioctl = osprd_ioctl
};


// Given an open file, check whether that file corresponds to an OSP ramdisk.
// If so, return a pointer to the ramdisk's osprd_info_t.
// If not, return NULL.

static osprd_info_t *file2osprd(struct file *filp)
{
	if (filp) {
		struct inode *ino = filp->f_dentry->d_inode;
		if (ino->i_bdev
		    && ino->i_bdev->bd_disk
		    && ino->i_bdev->bd_disk->major == OSPRD_MAJOR
		    && ino->i_bdev->bd_disk->fops == &osprd_ops)
			return (osprd_info_t *) ino->i_bdev->bd_disk->private_data;
	}
	return NULL;
}


// Call the function 'callback' with data 'user_data' for each of 'task's
// open files.

static void for_each_open_file(struct task_struct *task,
		  void (*callback)(struct file *filp, osprd_info_t *user_data),
		  osprd_info_t *user_data)
{
	int fd;
	task_lock(task);
	spin_lock(&task->files->file_lock);
	{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 13)
		struct files_struct *f = task->files;
#else
		struct fdtable *f = task->files->fdt;
#endif
		for (fd = 0; fd < f->max_fds; fd++)
			if (f->fd[fd])
				(*callback)(f->fd[fd], user_data);
	}
	spin_unlock(&task->files->file_lock);
	task_unlock(task);
}


// Destroy a osprd_info_t.

static void cleanup_device(osprd_info_t *d)
{
	wake_up_all(&d->blockq);
	if (d->gd) {
		del_gendisk(d->gd);
		put_disk(d->gd);
	}
	if (d->queue)
		blk_cleanup_queue(d->queue);
	if (d->data)
		vfree(d->data);
}


// Initialize a osprd_info_t.

static int setup_device(osprd_info_t *d, int which)
{
	memset(d, 0, sizeof(osprd_info_t));

	/* Get memory to store the actual block data. */
	if (!(d->data = vmalloc(nsectors * SECTOR_SIZE)))
		return -1;
	memset(d->data, 0, nsectors * SECTOR_SIZE);

	/* Set up the I/O queue. */
	spin_lock_init(&d->qlock);
	if (!(d->queue = blk_init_queue(osprd_process_request_queue, &d->qlock)))
		return -1;
	blk_queue_hardsect_size(d->queue, SECTOR_SIZE);
	d->queue->queuedata = d;

	/* The gendisk structure. */
	if (!(d->gd = alloc_disk(1)))
		return -1;
	d->gd->major = OSPRD_MAJOR;
	d->gd->first_minor = which;
	d->gd->fops = &osprd_ops;
	d->gd->queue = d->queue;
	d->gd->private_data = d;
	snprintf(d->gd->disk_name, 32, "osprd%c", which + 'a');
	set_capacity(d->gd, nsectors);
	add_disk(d->gd);

	/* Call the setup function. */
	osprd_setup(d);

	return 0;
}

static void osprd_exit(void);


// The kernel calls this function when the module is loaded.
// It initializes the 4 osprd block devices.

static int __init osprd_init(void)
{
	int i, r;

	// shut up the compiler
	(void) for_each_open_file;
#ifndef osp_spin_lock
	(void) osp_spin_lock;
	(void) osp_spin_unlock;
#endif

	/* Register the block device name. */
	if (register_blkdev(OSPRD_MAJOR, "osprd") < 0) {
		printk(KERN_WARNING "osprd: unable to get major number\n");
		return -EBUSY;
	}

	/* Initialize the device structures. */
	for (i = r = 0; i < NOSPRD; i++)
		if (setup_device(&osprds[i], i) < 0)
			r = -EINVAL;

	if (r < 0) {
		printk(KERN_EMERG "osprd: can't set up device structures\n");
		osprd_exit();
		return -EBUSY;
	} else
		return 0;
}


// The kernel calls this function to unload the osprd module.
// It destroys the osprd devices.

static void osprd_exit(void)
{
	int i;
	for (i = 0; i < NOSPRD; i++)
		cleanup_device(&osprds[i]);
	unregister_blkdev(OSPRD_MAJOR, "osprd");
}


// Tell Linux to call those functions at init and exit time.
module_init(osprd_init);
module_exit(osprd_exit);
