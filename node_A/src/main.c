#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

void main(void)
{
    while (1) {
        printk("Hello from Thingy!\n");
        k_msleep(1000);   // vent 1 sekund
    }
}
