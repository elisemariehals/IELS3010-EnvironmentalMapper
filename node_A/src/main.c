#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>   // sys_put_le16 / sys_put_le32
#include <zephyr/random/random.h>   // sys_rand32_get()
#include <math.h>                   // sin()
#include <stdint.h>

/* Fake GPS-posisjon (Gløshaugen-ish) i 1e-5 grader */
#define LAT_X1E5 6341710    /* 63.41710° */
#define LON_X1E5 1040280    /* 10.40280° */

/* Payload-format (13 byte):
 * [0]    = seq (uint8)
 * [1-2]  = T*100 (int16, little endian)      -> T = verdi / 100.0
 * [3-4]  = RH*10 (uint16, little endian)     -> RH = verdi / 10.0
 * [5-8]  = lat_x1e5  (int32, little endian)  -> lat = verdi / 1e5
 * [9-12] = lon_x1e5  (int32, little endian)  -> lon = verdi / 1e5
 */
#define PAYLOAD_LEN 13
static uint8_t payload[PAYLOAD_LEN];

/* --- Fake temp generator --- */
static double phase = 0.0;

/* Returnerer temperatur i 0.01C (t_x100) */
static int16_t make_fake_temp_x100(void)
{
    /* ca. 1 periode ~ (2*pi / 0.12) * 3s ≈ 157s */
    phase += 0.12;
    if (phase > 6.2831853) {
        phase -= 6.2831853;
    }

    double base  = 22.0;
    double drift = 1.8 * sin(phase); /* ±1.8C */

    /* noise i C: ±0.20C */
    double noise = ((int32_t)(sys_rand32_get() % 41) - 20) / 100.0;

    double temp_c = base + drift + noise;

    /* Konverter til 0.01C */
    int32_t t_x100 = (int32_t)(temp_c * 100.0);

    /* clamp til fornuftig område */
    if (t_x100 < -4000) t_x100 = -4000; /* -40.00C */
    if (t_x100 >  8500) t_x100 =  8500; /*  85.00C */

    return (int16_t)t_x100;
}

static void build_payload(uint8_t seq, int16_t t_x100, uint16_t rh_x10)
{
    payload[0] = seq;

    /* NB: skriv int16 i little endian (bevarer negativt korrekt) */
    sys_put_le16((uint16_t)t_x100, &payload[1]);

    sys_put_le16(rh_x10,           &payload[3]);
    sys_put_le32(LAT_X1E5,         &payload[5]);
    sys_put_le32(LON_X1E5,         &payload[9]);
}

void main(void)
{
    int seq = 0;

    printk("Node A started (fake temp + fake RH + fake GPS -> HEX payload)\n");
    printk("Sends: 26 hex chars + newline\n");

    while (1) {
        seq++;
        uint8_t seq_u8 = (uint8_t)seq;

        /* Fake humidity (fast): 45.0% => rh_x10 = 450 */
        uint16_t rh_x10 = 450;

        /* Fake temp (varierende) */
        int16_t t_x100 = make_fake_temp_x100();

        build_payload(seq_u8, t_x100, rh_x10);

        /* Send payload som HEX-streng uten mellomrom + newline */
        for (int i = 0; i < PAYLOAD_LEN; i++) {
            printk("%02X", payload[i]);
        }
        printk("\n");

        k_sleep(K_SECONDS(3));
    }
}
