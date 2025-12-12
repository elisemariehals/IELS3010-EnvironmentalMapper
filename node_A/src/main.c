#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>   // for abs()

/* Sjekk om vi har noen sensor med compat "bosch,bme680"
 * (BME688 bruker ofte samme driver/compat).
 */
#if DT_HAS_COMPAT_STATUS_OKAY(bosch_bme680)
const struct device *const env_sensor = DEVICE_DT_GET_ONE(bosch_bme680);
#define HAVE_ENV_SENSOR 1
#else
const struct device *const env_sensor = NULL;
#define HAVE_ENV_SENSOR 0
#endif

/* Fake GPS-posisjon (Gløshaugen-ish) i 1e-5 grader */
#define LAT_X1E5 6341710    /* 63.41710° */
#define LON_X1E5 1040280    /* 10.40280° */

static void print_fixed_latlon(int32_t lat_x1e5, int32_t lon_x1e5)
{
    int32_t lat_deg  = lat_x1e5 / 100000;
    int32_t lat_frac = abs(lat_x1e5 % 100000);

    int32_t lon_deg  = lon_x1e5 / 100000;
    int32_t lon_frac = abs(lon_x1e5 % 100000);

    printk("GPS: lat=%d.%05d, lon=%d.%05d\n",
           lat_deg, lat_frac,
           lon_deg, lon_frac);
}

void main(void)
{
    int seq = 0;

#if HAVE_ENV_SENSOR
    if (!device_is_ready(env_sensor)) {
        printk("Env sensor NOT ready: %s\n", env_sensor->name);
        printk("Fortsetter med fake T/RH-verdier.\n");
    } else {
        printk("Env sensor OK: %s\n", env_sensor->name);
    }
#else
    printk("Ingen BME680/BME688 sensor aktiv i devicetree på denne kjernen.\n");
    printk("Fortsetter med fake T/RH-verdier.\n");
#endif

    printk("Node A started (env + fake GPS)\n");

    while (1) {
        struct sensor_value t = {0}, rh = {0};
        int ret = 0;

        seq++;

#if HAVE_ENV_SENSOR
        if (device_is_ready(env_sensor)) {
            ret = sensor_sample_fetch(env_sensor);
            if (ret == 0) {
                sensor_channel_get(env_sensor, SENSOR_CHAN_AMBIENT_TEMP, &t);
                sensor_channel_get(env_sensor, SENSOR_CHAN_HUMIDITY, &rh);
            } else {
                printk("sensor_sample_fetch error: %d\n", ret);
            }
        } else {
            /* fallback til fake verdier */
            t.val1 = 22;
            t.val2 = 0;
            rh.val1 = 45;
            rh.val2 = 0;
        }
#else
        /* Helt fake verdier hvis vi ikke har sensor i DT */
        t.val1 = 22;
        t.val2 = 0;
        rh.val1 = 45;
        rh.val2 = 0;
#endif

        printk("seq=%d\n", seq);
        printk("T = %d.%06d C, RH = %d.%06d %%\n",
               t.val1, t.val2,
               rh.val1, rh.val2);

        print_fixed_latlon(LAT_X1E5, LON_X1E5);

        /* 3 sek – innenfor 2–5 s kravet */
        k_sleep(K_SECONDS(3));
    }
}
