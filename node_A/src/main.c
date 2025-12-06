#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

void main(void)
{
    printk("Program started!\n");

    while (1) {
        printk("Still running...\n");
        k_msleep(1000);
    }
}

