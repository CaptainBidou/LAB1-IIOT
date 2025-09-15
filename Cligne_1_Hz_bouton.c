/*
 * Cligne_1_Hz_bouton.c
 * 
 * Ce programme fait clignoter un DEL à une fréquence de 1 Hz.
 * Arrêt par appui sur le bouton-poussoir noir
 * Aucun paramètre en entrée.
 *
 */

#include <stdio.h>
#include <bcm2835.h>

#define DEL_Rouge 20   // Définition du GPIO associé au DEL rouge
#define Bouton_Noir 19 // Définition du GPIO associé au bouton noir

int main(void)
{
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(DEL_Rouge, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour Bouton-poussoir 1 (noir)
	bcm2835_gpio_fsel(Bouton_Noir, BCM2835_GPIO_FSEL_INPT);	
	
	// Clignoter DEL:
	// DEL allumée 500 ms et éteinte 500 ms
	// Arrêt lors de l'appui du bouton poussoir noir
	while(!bcm2835_gpio_lev(Bouton_Noir)){
		bcm2835_gpio_write(DEL_Rouge, HIGH);
		bcm2835_delay(500);
		bcm2835_gpio_write(DEL_Rouge, LOW);
		bcm2835_delay(500);
	}
	
	// Libérer les broches du GPIO
	bcm2835_close();
	return 0;
}