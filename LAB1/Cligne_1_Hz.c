/*
 * Cligne_1_Hz.c
 * 
 * Ce programme fait clignoter un DEL à une fréquence de 1 Hz.
 * Aucun paramètre en entrée.
 *
 */

#include <stdio.h>
#include <bcm2835.h>

#define DEL_Rouge 20 // Définition du GPIO associé au DEL rouge (broche 38)
#define Bouton_Noir 19 // Définition du GPIO associé au bouton noir
#define DEL_Orange 21 // Définition du GPIO associé au DEL orange (broche 40)
#define Bouton_Rouge 26 // Définition du GPIO associé au bouton rouge (broche 38)

int main(void)
{
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(DEL_Rouge, BCM2835_GPIO_FSEL_OUTP);
	
	// Configuration du GPIO pour DEL 2 (orange)
	bcm2835_gpio_fsel(DEL_Orange, BCM2835_GPIO_FSEL_OUTP);
	

	// Appuie sur les deux boutons pour continuer
	//while(!bcm2835_gpio_lev(Bouton_Rouge) && !bcm2835_gpio_lev(Bouton_Noir)){


	// Clignoter DEL:
	// DEL allumée 500 ms et éteinte 500 ms
	while((bcm2835_gpio_lev(Bouton_Noir)&&bcm2835_gpio_lev(Bouton_Rouge))){
		bcm2835_gpio_write(DEL_Rouge, HIGH);
		bcm2835_gpio_write(DEL_Orange, LOW);
		//bcm2835_delay(500);

		//Fréquence de clignotement à 0.1Hz
		bcm2835_delay(5000);

		bcm2835_gpio_write(DEL_Rouge, LOW);
		bcm2835_gpio_write(DEL_Orange, HIGH);
		//bcm2835_delay(500);

		// Fréquence de clignotement à 0.1 Hz
		bcm2835_delay(5000);
	}
	
	//S'assurer que les DEL sont éteintes avant de quitter
	bcm2835_gpio_write(DEL_Rouge, LOW);
	bcm2835_gpio_write(DEL_Orange, LOW);

	// Libérer les broches du GPIO
	bcm2835_close();
	return 0;
}
