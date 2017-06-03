/*
 * gpio.c
 *
 *  Created on: Jun 3, 2017
 *      Author: Han-Chung Tseng
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <asm-generic/io.h>
#include <asm-generic/uaccess.h>
//========================================================================
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Han-Chung Tseng");
MODULE_DESCRIPTION("RPi3 B GPIO Kernel Module");
//========================================================================
#define GPIO_MODULE_NAME    "pi_gpio"
//========================================================================
static dev_t g_DevNum;
static struct cdev *g_GpioCdev;
static struct class *g_GpioClass;
static unsigned int g_OpenCnt = 0;
static DEFINE_SEMAPHORE(g_DrvSem);
//========================================================================
static struct gpio g_GpioTbl[] = {
        { 16    ,  GPIOF_DIR_IN     , "temp"     },
        { 20    ,  GPIOF_DIR_OUT    , "led_1"    },
        { 21    ,  GPIOF_DIR_OUT    , "led_2"    },
};
//========================================================================
static int GPIO_Open(struct inode *inode, struct file *file)
{
    int ret = 0;

    down(&g_DrvSem);
    g_OpenCnt++;
    up(&g_DrvSem);

    return ret;
}
//========================================================================
static int GPIO_Release(struct inode *inode, struct file *file)
{
    int ret = 0;

    down(&g_DrvSem);
    g_OpenCnt--;
    up(&g_DrvSem);

    return ret;
}
//========================================================================
static ssize_t GPIO_Read(struct file *filp, char __user *buf,
                size_t count, loff_t *offset)
{
    int ret = count;
    int major = iminor(filp->f_path.dentry->d_inode);
    char *kbuf = kmalloc(count, GFP_KERNEL), *ptr;

    if(!kbuf)
        return -ENOMEM;

    //printk(KERN_INFO "GPIO: %d; Pin: %d\n", major, g_GpioTbl[major].gpio);

    ptr = kbuf;

    while(count >= 4)
    {
        *(unsigned int *)ptr = gpio_get_value(g_GpioTbl[major].gpio);
        //printk(KERN_INFO "Val: %d, Len: %d\n", *(unsigned int *)ptr, count);
        ptr += 4;
        count -= 4;
    }

    if((ret > 0) && (copy_to_user(buf, kbuf, ret)))
        return -EFAULT;

    kfree(kbuf);
    return ret;
}
//========================================================================
static ssize_t GPIO_Write(struct file *filp, const char __user *buf,
                size_t count, loff_t *offset)
{
    int ret = count;
    int major = iminor(filp->f_path.dentry->d_inode);
    unsigned char *kbuf = kmalloc(count, GFP_KERNEL), *ptr;

    if(!kbuf)
        return -ENOMEM;

    if(copy_from_user(kbuf, buf, count))
        return -EFAULT;

    kbuf[count] = '\0';
    //printk(KERN_INFO "Kbuf: %d; buf: %d\n", *(unsigned int *)kbuf, *(unsigned int *)buf);
    //printk(KERN_INFO "Request from user: %s; Pin: %d; Gpio: %d; Len: %d\n", kbuf, g_GpioTbl[major].gpio, major, count);

    ptr = kbuf;

    while(count >= 4)
    {
        gpio_set_value(g_GpioTbl[major].gpio, *(unsigned int *)ptr);
        count -= 4;
        ptr += 4;
    }

    kfree(kbuf);
    return ret;
}
//========================================================================
static struct file_operations fops = {
        .owner      = THIS_MODULE,
        .open       = GPIO_Open,
        .read       = GPIO_Read,
        .write      = GPIO_Write,
        .release    = GPIO_Release,
};
//========================================================================
static int __init GPIO_Init(void)
{
    int i;
    int ret = 0;

    ret = gpio_request_array(g_GpioTbl, ARRAY_SIZE(g_GpioTbl));
    if(ret < 0)
    {
        printk(KERN_INFO "GPIO: Registering failed\n");
        goto exit_gpio_request;
    }

    ret = alloc_chrdev_region(&g_DevNum, 0, ARRAY_SIZE(g_GpioTbl), GPIO_MODULE_NAME);
    if(ret)
    {
        printk(KERN_INFO "GPIO: Alloc chrdev failed\n");
        goto exit_gpio_request;
    }

    g_GpioCdev = cdev_alloc();
    if(g_GpioCdev == NULL)
    {
        printk(KERN_INFO "GPIO: Alloc cdev failed\n");
        ret = -ENOMEM;
        goto exit_unregister_chrdev;
    }

    cdev_init(g_GpioCdev, &fops);
    g_GpioCdev->owner=THIS_MODULE;

    ret = cdev_add(g_GpioCdev, g_DevNum, ARRAY_SIZE(g_GpioTbl));
    if(ret)
    {
        printk(KERN_INFO "GPIO: Add cdev failed\n");
        goto exit_cdev;
    }

    g_GpioClass = class_create(THIS_MODULE, GPIO_MODULE_NAME);
    if(IS_ERR(g_GpioClass))
    {
        printk(KERN_INFO "GPIO: Creation class failed\n");
        ret = PTR_ERR(g_GpioClass);
        goto exit_cdev;
    }

    for(i = 0; i < ARRAY_SIZE(g_GpioTbl); i++)
    {
        if(device_create(g_GpioClass, NULL, MKDEV(MAJOR(g_DevNum), MINOR(g_DevNum) + i), NULL, g_GpioTbl[i].label) == NULL)
        {
            printk(KERN_INFO "GPIO: Device creation failed\n");
            ret = -1;
            goto exit_cdev;
        }
    }

    printk(KERN_INFO "GPIO: Kernel modules loaded\n");
    return 0;

exit_cdev:
    cdev_del(g_GpioCdev);

exit_unregister_chrdev:
    unregister_chrdev_region(g_DevNum, ARRAY_SIZE(g_GpioTbl));

exit_gpio_request:
    gpio_free_array(g_GpioTbl, ARRAY_SIZE(g_GpioTbl));
    return ret;
}
//========================================================================
static void __exit GPIO_Exit(void)
{
    int i;

    for(i = 0; i < ARRAY_SIZE(g_GpioTbl); i++)
    {
        gpio_set_value(g_GpioTbl[i].gpio, 0);
        device_destroy(g_GpioClass, MKDEV(MAJOR(g_DevNum), MINOR(g_DevNum) + i));
    }

    class_destroy(g_GpioClass);
    cdev_del(g_GpioCdev);
    unregister_chrdev_region(g_DevNum, ARRAY_SIZE(g_GpioTbl));
    gpio_free_array(g_GpioTbl, ARRAY_SIZE(g_GpioTbl));

    printk(KERN_INFO "GPIO: Kernel modules unloaded\n");
}
//========================================================================
module_init(GPIO_Init);
module_exit(GPIO_Exit);
