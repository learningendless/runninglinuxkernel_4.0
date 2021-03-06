#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/kthread.h>
#include <linux/delay.h>

struct foo {
	int a;
	struct rcu_head rcu;
};

static struct foo *g_ptr;
static void myrcu_reader_thread1(void *data) //读者线程1
{
	struct foo *p1 = NULL;

	while (1) {
		msleep(10);
		rcu_read_lock();
		p1 = rcu_dereference(g_ptr);
		if (p1)
			printk("%s: read a=%d\n", __func__, p1->a);
		rcu_read_unlock();
	}
}

static void myrcu_reader_thread2(void *data) //读者线程2
{
	struct foo *p2 = NULL;

	while (1) {
		msleep(100);
		rcu_read_lock();
		mdelay(1500);
		p2 = rcu_dereference(g_ptr);
		if (p2)
			printk("%s: read a=%d\n", __func__, p2->a);
		
		rcu_read_unlock();
	}
}

static void myrcu_del(struct rcu_head *rh)
{
	struct foo *p = container_of(rh, struct foo, rcu);
	printk("%s: a=%d\n", __func__, p->a);
	kfree(p);
}

static void myrcu_writer_thread(void *p) //写者线程
{
	struct foo *new;
	struct foo *old;
	int value = (unsigned long)p;

	while (1) {
		msleep(100);
		struct foo *new_ptr = kmalloc(sizeof (struct foo), GFP_KERNEL);
		old = g_ptr;
		printk("%s: write to new %d\n", __func__, value);
		*new_ptr = *old;
		new_ptr->a = value;
		rcu_assign_pointer(g_ptr, new_ptr);
		call_rcu(&old->rcu, myrcu_del); 
		value++;
	}
}     

static int __init my_test_init(void)
{   
	struct task_struct *reader_thread1;
	struct task_struct *reader_thread2;
	struct task_struct *writer_thread;
	int value = 5;

	printk("figo: my module init\n");
	g_ptr = kzalloc(sizeof (struct foo), GFP_KERNEL);

	reader_thread1 = kthread_run(myrcu_reader_thread1, NULL, "rcu_reader1");
	reader_thread2 = kthread_run(myrcu_reader_thread2, NULL, "rcu_reader2");
	writer_thread = kthread_run(myrcu_writer_thread, (void *)(unsigned long)value, "rcu_writer");

	return 0;
}
static void __exit my_test_exit(void)
{
	printk("goodbye\n");
	if (g_ptr)
		kfree(g_ptr);
}
MODULE_LICENSE("GPL");
module_init(my_test_init);
module_exit(my_test_exit);
