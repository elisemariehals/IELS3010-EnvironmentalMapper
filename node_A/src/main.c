#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&led)) {
        printk("GPIO not ready\n");
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to configure LED\n");
        return;
    }

    while (1) {
        printk("Hello from Thingy!\n");
        k_msleep(1000);   // vent 1 sekund
    }
}
