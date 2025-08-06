/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_main, CONFIG_LORA_MAIN_LOG_LEVEL);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
         "No default LoRa radio specified in DT");

#define MAX_DATA_LEN 12

char data[MAX_DATA_LEN] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', ' ', '0'};

#define PWM_LED_PERIOD NSEC_PER_MSEC

static const struct pwm_dt_spec ui_led = PWM_DT_SPEC_GET(DT_ALIAS(uiled));

static uint32_t current_ui_led_pulse = 0;

int main(void)
{
    int ret = 0;

    LOG_INF("main starting");

    if (!pwm_is_ready_dt(&ui_led)) {
        LOG_ERR("Error: PWM device %s is not ready", ui_led.dev->name);
        return -ENODEV;
    }
    ret = pwm_set_dt(&ui_led, PWM_LED_PERIOD, 0);
    if (ret < 0) {
        LOG_ERR("Error %d: failed to set pulse width", ret);
        return ret;
    }
    LOG_INF("PWM LEDs init done");


    const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
    struct lora_modem_config config;

    if (!device_is_ready(lora_dev)) {
        LOG_ERR("%s Device not ready", lora_dev->name);
        return 0;
    }

    config.frequency = 868000000;
    config.bandwidth = BW_125_KHZ;
    config.datarate = SF_10;
    config.preamble_len = 8;
    config.coding_rate = CR_4_5;
    config.iq_inverted = false;
    config.public_network = false;
    config.tx_power = 4;
    config.tx = true;

    ret = lora_config(lora_dev, &config);
    if (ret < 0) {
        LOG_ERR("LoRa config failed");
        return 0;
    }

    while (true) {
        ret = lora_send(lora_dev, data, MAX_DATA_LEN);
        if (ret < 0) {
            LOG_ERR("LoRa send failed %d", ret);
            // return 0;
        }

        LOG_INF("Data sent %c!", data[MAX_DATA_LEN - 1]);

        /* Send data at 1s interval */
        k_sleep(K_MSEC(1000));

        /* Increment final character to differentiate packets */
        if (data[MAX_DATA_LEN - 1] == '9') {
            data[MAX_DATA_LEN - 1] = '0';
        } else {
            data[MAX_DATA_LEN - 1] += 1;
        }


        /* Blink */
        if (current_ui_led_pulse > 0) {
            pwm_set_pulse_dt(&ui_led, 0);
            current_ui_led_pulse = 0;
        }
        else {
            pwm_set_pulse_dt(&ui_led, PWM_LED_PERIOD);
            current_ui_led_pulse = PWM_LED_PERIOD;
        }

    }

    return 0;
}
