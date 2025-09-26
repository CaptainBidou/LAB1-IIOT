/*
 * Thread_Cligne_1_Hz.c
 * 
 * Ce programme fait clignoter un DEL à une fréquence de 1 Hz.
 * Le clignotement se fait à l'intérieur d'un thread.
 * L'arrêt sera provoqué par un appui sur le bouton-poussoir 1.
 * Aucun paramètre en entrée.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>

#define DEL_Rouge 20 // Définition du GPIO associé au DEL rouge (broche 38)
#define Bouton_Noir 19 // Définition du GPIO associé au bouton noir
#define DEL_Orange 21 // Définition du GPIO associé au DEL orange (broche 40)
#define Bouton_Rouge 26 // Définition du GPIO associé au bouton rouge (broche 38)

/* Fonction clignote qui sera appelée par le thread "cligne"
 * 
 * Elle fait clignoter le DEL rouge à 1 Hertz.
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
void *clignote(int broche)
{
	// Boucle infinie pour le clignotement du DEL
	if(broche == DEL_Orange)
		bcm2835_delay(5000);
	
	while(1){
		bcm2835_gpio_write(broche, HIGH);
		bcm2835_delay(5000);
		bcm2835_gpio_write(broche, LOW);
		bcm2835_delay(5000);
	}
}

/* Fonction main
 * 
 * Elle configure le GPIO et le thread.
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
int main(int argc, char **argv)
{
	// Identificateur du thread
	pthread_t cligne;  
	pthread_t cligne2;
	
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(DEL_Rouge, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour bouton-poussoir 1
	bcm2835_gpio_fsel(Bouton_Noir, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO pour bouton-poussoir 2
	bcm2835_gpio_fsel(Bouton_Rouge, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO pour DEL 2 (orange)
	bcm2835_gpio_fsel(DEL_Orange, BCM2835_GPIO_FSEL_OUTP);
	
	// Création du thread "cligne".
	// Lien avec la fonction clignote.
	// Cette dernière n'exige pas de paramètres.
	pthread_create(&cligne, NULL, &clignote, DEL_Rouge);
	pthread_create(&cligne2, NULL, &clignote, DEL_Orange);
	
	// Boucle tant que le bouton-poussoir est non enfoncé
	while((bcm2835_gpio_lev(Bouton_Noir)||bcm2835_gpio_lev(Bouton_Rouge))){
		usleep(1000); // Délai de 1 ms !!!
	}

	// Si bouton-poussoir enfoncé, arrêt immédiat du thread 
    pthread_cancel(cligne);
    pthread_cancel(cligne2);
    // Attente de l'arrêt du thread
    pthread_join(cligne, NULL);
    pthread_join(cligne2, NULL);
    
    // Éteindre le DEL rouge
    bcm2835_gpio_write(DEL_Rouge, LOW);
    
    // Éteindre le DEL orange
    bcm2835_gpio_write(DEL_Orange, LOW);
    // Libérer le GPIO
    bcm2835_close();
    return 0;
}
