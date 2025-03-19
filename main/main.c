#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "hardware/timer.h"
#include "pico/util/datetime.h"

#define TRIGGER_PIN 2
#define ECHO_PIN 3

void setup_rtc() {
    datetime_t t = {
        .year  = 2024,
        .month = 3,
        .day   = 19,
        .dotw  = 2, 
        .hour  = 22,
        .min   = 10,
        .sec   = 0
    };
    rtc_init();
    rtc_set_datetime(&t);
}

void trigger_sensor() {
    gpio_put(TRIGGER_PIN, 1);
    sleep_us(10); 
    gpio_put(TRIGGER_PIN, 0);
}

bool measure_distance(uint32_t *distance_cm) {
    uint64_t start_time = 0;
    uint64_t end_time = 0;
    bool echo_done = false;
    bool timeout = false;

    trigger_sensor();

    uint64_t wait_start = time_us_64();
    while (!gpio_get(ECHO_PIN)) {
        if (time_us_64() - wait_start > 30000) { 
            timeout = true;
            break;
        }
    }
    if (!timeout) {
        start_time = time_us_64();
    }

    wait_start = time_us_64();
    while (gpio_get(ECHO_PIN) && !timeout) {
        if (time_us_64() - wait_start > 30000) { 
            timeout = true;
            break;
        }
    }
    if (!timeout) {
        end_time = time_us_64();
        echo_done = true;
    }

    if (echo_done && !timeout) {
        uint64_t pulse_time = end_time - start_time;
        *distance_cm = pulse_time / 58; 
        return true;
    }
    return false; 
}

int main() {
    stdio_init_all();

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
    gpio_put(TRIGGER_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_pull_down(ECHO_PIN);

    setup_rtc();

    bool reading_active = false; 
    uint32_t distance_cm = 0;

    printf("Digite 's' para iniciar as leituras.\n");

    while (true) {
        // Declara 'command' aqui, onde é usado
        char command = getchar_timeout_us(100000);
        if (command == 's' && !reading_active) {
            reading_active = true;
            printf("Iniciando leituras...\n");
        } else if (command == 'p' && reading_active) {
            reading_active = false;
            printf("Parando leituras...\n");
        }


        if (reading_active) {
            datetime_t now;
            rtc_get_datetime(&now);

            if (measure_distance(&distance_cm)) {
                printf("%02d:%02d:%02d - %u cm\n", 
                       now.hour, now.min, now.sec, distance_cm);
            } else {
                printf("%02d:%02d:%02d - Falha\n", 
                       now.hour, now.min, now.sec);
            }

            sleep_ms(1000); 
        }
    }

    return 0;
}