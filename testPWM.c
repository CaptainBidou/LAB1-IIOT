#include <stdio.h>
#include <bcm2835.h>

#define PIN_PWM0 18
#define CAN_PWM0 0
#define RANGE 384

int main(void) {
    if (!bcm2835_init()) {
        printf("BCM2835 init fail\n");
        return 1;
    }

    bcm2835_gpio_fsel(PIN_PWM0, BCM2835_GPIO_FSEL_ALT5);
    bcm2835_pwm_set_clock(100);
    bcm2835_pwm_set_range(CAN_PWM0, RANGE);
    bcm2835_pwm_set_mode(CAN_PWM0, 1, 1);
    bcm2835_pwm_set_data(CAN_PWM0, RANGE / 2);

    printf("Lamp start at 50%% \n");
    bcm2835_delay(5000);

    bcm2835_pwm_set_data(CAN_PWM0, 0);
    bcm2835_close();
    return 0;
}
