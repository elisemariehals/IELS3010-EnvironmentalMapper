#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>   // for sys_put_le16 / sys_put_le32
#include <stdlib.h>                 // for abs()

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

/* Payload-format (13 byte):
 * [0]    = seq (uint8)
 * [1-2]  = T*100 (int16, little endian)   -> T = verdi / 100.0
 * [3-4]  = RH*10 (uint16, little endian)  -> RH = verdi / 10.0
 * [5-8]  = lat_x1e5  (int32, little endian) -> lat = verdi / 1e5
 * [9-12] = lon_x1e5  (int32, little endian) -> lon = verdi / 1e5
 */
#define PAYLOAD_LEN 13
static uint8_t payload[PAYLOAD_LEN];

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

/* Bygg payload ut fra seq, temp og RH */
static void build_payload(uint8_t seq,
                          const struct sensor_value *t,
                          const struct sensor_value *rh)
{
    /* Konverter sensor_value til double */
    double t_c  = sensor_value_to_double(t);   /* grader C */
    double rh_p = sensor_value_to_double(rh);  /* prosent */

    int16_t  t_x100 = (int16_t)(t_c * 100.0);   /* 0.01 C */
    uint16_t rh_x10 = (uint16_t)(rh_p * 10.0);  /* 0.1 % */

    payload[0] = seq;
    sys_put_le16((uint16_t)t_x100, &payload[1]);
    sys_put_le16(rh_x10,           &payload[3]);
    sys_put_le32(LAT_X1E5,         &payload[5]);
    sys_put_le32(LON_X1E5,         &payload[9]);
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

    printk("Node A started (env + fake GPS + payload)\n");

    while (1) {
        struct sensor_value t = {0}, rh = {0};
        int ret = 0;

        seq++;
        uint8_t seq_u8 = (uint8_t)seq;  /* ruller rundt etter 255, det er ok */

#if HAVE_ENV_SENSOR
        if (device_is_ready(env_sensor)) {
            ret = sensor_sample_fetch(env_sensor);
            if (ret == 0) {
                sensor_channel_get(env_sensor, SENSOR_CHAN_AMBIENT_TEMP, &t);
                sensor_channel_get(env_sensor, SENSOR_CHAN_HUMIDITY, &rh);
            } else {
                printk("sensor_sample_fetch error: %d\n", ret);
                /* fallback ved feil */
                t.val1 = 22;
                t.val2 = 0;
                rh.val1 = 45;
                rh.val2 = 0;
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

        /*printk("seq=%d\n", seq);
        printk("T = %d.%06d C, RH = %d.%06d %%\n",
               t.val1, t.val2,
               rh.val1, rh.val2);

        print_fixed_latlon(LAT_X1E5, LON_X1E5);

        /* Bygg payload for denne målingen */
        build_payload(seq_u8, &t, &rh);

        /* Send payload som HEX-streng uten mellomrom + newline */
        for (int i = 0; i < PAYLOAD_LEN; i++) {
        printk("%02X", payload[i]);
        }
        printk("\n");

        /* 3 sek – innenfor 2–5 s kravet */
        k_sleep(K_SECONDS(3));
    }
} 
