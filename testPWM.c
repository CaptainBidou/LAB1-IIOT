/*
 * rampe_PWM.c
 *
 * Utilisation du PWM pour allumer la lampe 12 V
 * progressivement de 0 à 100 % d’intensité.
 * Aucun paramètre d’entrée.
 */

#include <stdio.h>
#include <bcm2835.h>

#define PIN_PWM0 18   // Définition de la broche du PWM0 (lampe 12 V)
#define CAN_PWM0 0    // Définition du canal PWM utilisé
#define RANGE 384     // Définition de la plage de variation du PWM

int z = 0;            // Valeur de l’intensité
int cycle = 10;       // Nombre de cycles

int main(void)
{
    // Initialisation du BCM2835
    if (!bcm2835_init()) {
        return 1;
    }

    // Configuration du GPIO pour la sortie PWM0
    bcm2835_gpio_fsel(PIN_PWM0, BCM2835_GPIO_FSEL_ALT5);

    // On désire une fréquence du PWM de 500 Hertz
    // Cela implique d’ajuster d * range à 38400
    // On ajuste le range à 384 et d à 100
    bcm2835_pwm_set_clock(100);             // d ajusté à 100
    bcm2835_pwm_set_range(CAN_PWM0, RANGE); // range ajusté à 384

    bcm2835_pwm_set_data(CAN_PWM0, 0);      // Valeur initiale du duty cycle à 0%

    // Ajusté en mode "mark space" et activation de la sortie PWM
    bcm2835_pwm_set_mode(CAN_PWM0, 1, 1);

    // On répète la rampe un certain nombre de fois
    while (cycle > 0) {
        bcm2835_delay(500);                   // Délai entre deux changements d’intensité
        bcm2835_pwm_set_data(CAN_PWM0, z++);  // Nouvelle intensité (z / range) * 100 %

        if (z > RANGE) {
            z = 0;     // Remise à 0 du duty cycle lorsque le 100% est dépassé
            cycle--;   // Décrémentation du nombre de cycles restants
        }
    }

    // On remet le duty cycle à 0% avant de quitter le programme
    bcm2835_pwm_set_data(CAN_PWM0, 0);

    // Libérer les broches du GPIO
    bcm2835_close();

    return 0;
}
