/*
 * Thread_Cligne_10_Hz_time.c
 * 
 * Ce programme fait clignoter un DEL à une fréquence de 10 Hz.
 * Le clignotement se fait à l'intérieur d'un thread et le délai est
 * ajusté avec des fonctions de la librarie "time.h".
 * L'arrêt sera provoqué par un appui sur le bouton-poussoir 1.
 * Aucun paramètre en entrée.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

#define SPI_MISO 23 // Définition du GPIO associé au MISO du SPI (broche 16)
#define SPI_CS 24 // Définition du GPIO associé au CS du SPI (broche 18)
#define SPI_SCK 25 // Définition du GPIO associe au SCK du SPI (broche 22)

double frequence = 10.0; // Hertz : fréquence de clignotement désirée
double frequence_SPI = 4000000.0; // Hertz : fréquence du SPI
double RANGE = 200; // valeur du RANGE du pwm
double erreurK_1 = 0;
double erreurK_2 = 0;

/* Fonction clignote qui sera appelée par le thread "cligne"
 * 
 * Elle fait clignoter le DEL rouge à 1 Hertz.
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
void *clignote(int broche)
{
	clock_t debut, fin; // Variables de temps
	double demiPer = 1/(2.0*frequence); // Calcul de la demi-période
	int etat = 0; // État du DEL
	
	debut = clock(); // Temps écoulé depuis le lancement du programme
	
	// Boucle infinie pour le clignotement du DEL
	while(1){
		if (etat==0){
			bcm2835_gpio_write(broche, HIGH);
			etat = 1;
		}
		else{
			bcm2835_gpio_write(broche, LOW);
			etat = 0;
		}
		
		fin = clock(); // Temps écoulé depuis le lancement du programme
		
		// La différence (fin-debut) donne le temps d'exécution du code
		// On cherche une durée égale à demiPer. On compense avec un délai
		// CLOCK_PER_SEC est le nombre de coups d'horloges en 1 seconde
		
		usleep(1000000*(demiPer-((double) (fin-debut)/((double) CLOCKS_PER_SEC))));
		debut = fin;
	}
	pthread_exit(NULL);
}


/* Fonction lecture_SPI qui ser appelee dans le main
 * 
 * Elle lit la valeur de la temperature sur le thermocouple
 *
 * La fonction retourne la temperature en degres celsius
 * La fonction n'exige aucun paramètre en entrée.
 */
double lecture_SPI(void){
	//Variable de enregistrement du signal MISO
	unsigned int sigMiso = 0x00;
	// Variable de lecture du signal
	unsigned int readMiso = 0x00;
	
	// Configuration du GPIO pour MISO
	bcm2835_gpio_fsel(SPI_MISO, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO pour SPI_CS
	bcm2835_gpio_fsel(SPI_CS, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour SPI_SCK
	bcm2835_gpio_fsel(SPI_SCK, BCM2835_GPIO_FSEL_OUTP);
	
	
	// On met le CS a 1
	bcm2835_gpio_write(SPI_CS, HIGH);
	
	
	
	//Depart de la routine
	// On met le CS a 0
	bcm2835_gpio_write(SPI_CS, LOW);
	
	//Routine
	clock_t debut, fin; // Variables de temps
	double demiPer = 1/(2.0*frequence_SPI); // Calcul de la demi-période
	int etat = 0; // État du clock
	int nbClock=0;
	
	debut = clock(); // Temps écoulé depuis le lancement du programme
	
	// Boucle infinie pour le clignotement du clock
	while(nbClock!=32){
		if (etat==0){
			bcm2835_gpio_write(SPI_SCK, HIGH);
			readMiso = bcm2835_gpio_lev(SPI_MISO);
			readMiso = readMiso & 0b00000000000000000000000000000001;
			sigMiso = (sigMiso << 1) + readMiso;
			etat = 1;
		}
		else{
			bcm2835_gpio_write(SPI_SCK, LOW);
			etat = 0;
			nbClock++;
		}
		
		fin = clock(); // Temps écoulé depuis le lancement du programme
		
		// La différence (fin-debut) donne le temps d'exécution du code
		// On cherche une durée égale à demiPer. On compense avec un délai
		// CLOCK_PER_SEC est le nombre de coups d'horloges en 1 seconde
		
		usleep(1000000*(demiPer-((double) (fin-debut)/((double) CLOCKS_PER_SEC))));
		debut = fin;
	}
	
	// Fin de la routine
	// On remonte le CS a 1
	bcm2835_gpio_write(SPI_CS, HIGH);
	
	unsigned int maskENT = 0b01111111111100000000000000000000;
	unsigned int maskDEC = 0b00000000000011000000000000000000;
	unsigned int maskNEG = 0b10000000000000000000000000000000;
	unsigned int tempENT = sigMiso&maskENT;
	unsigned int tempDEC = sigMiso&maskDEC;
	double temp = 0.0;
	// Verification de la temperature negative
	if((sigMiso&maskNEG)>>31==1){
		temp = (double)(-1*(~(unsigned int)((tempENT>>20)*1.0+(tempDEC>>18)*0.25))+1);
		
	}
	else{
		temp = (double)((tempENT>>20)*1.0)+(double)((tempDEC>>18)*0.25);
	}
	
	
	//printf("Signal recupere : %d\n",sigMiso);
	//printf("Signal traite :%f\n",temp);

	return (double)temp;
}


int commande_Thermo(double consigne, double mesure){
	if(consigne < mesure){
		return 0; // Chauffage desactive
	}
	else{
		return RANGE; // Chauffage active
	}
}

int commande_PID(double consigne, double mesure, double gainKp, double gainKi, double gainKd){
	double erreur = consigne - mesure;
	consigne = consigne + gainKp*(erreur-erreurK_1) + (gainKi*1/2)(erreur+erreurK_1) + (gainKd/2)*(erreur - 2*erreurK_1 + erreurK_2);
	erreurK_2 = erreurK_1;
	erreurK_1 = erreur;

	if (consigne < 0){
		return 0; // Chauffage desactive
	}
	else if (consigne > RANGE){
		return RANGE; // Chauffage active
	}
	else{
		return (int)consigne;
	}
}

int consigne_temperature(){
	while(1){
		int consigne = commande_Thermo(25, lecture_SPI());
		// int consigne = commande_PID(25, lecture_SPI(), 5, 0.1, 1);
		printf("Consigne : %d\n",consigne);
		bcm2835_pwm_set_data(0,consigne);
		sleep(1);
	}

}





void pwmThread(){
	int range = RANGE;
	int d = 3;
	int dutyCycle = 0;
	bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_ALT5);
	bcm2835_pwm_set_clock(d);
	bcm2835_pwm_set_mode(0,1,1);
	bcm2835_pwm_set_range(0, range);
	bcm2835_pwm_set_data(0,0);
	
	while(1){
		while(dutyCycle < range){
			bcm2835_pwm_set_data(0,dutyCycle);
			dutyCycle ++;
			usleep(10000);
			printf("dutyCycle = %d\n",dutyCycle);
		}
		while(dutyCycle>0){
			bcm2835_pwm_set_data(0,dutyCycle);
			dutyCycle = dutyCycle-1;
			usleep(10000);
			printf("dutyCycle = %d\n",dutyCycle);
		}
		  }
		
		//bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_OUTP);
		//bcm2835_gpio_write(18, HIGH);
	
}



void temperatureThread(void){
	
	while(1){
		double temp = lecture_SPI();
		printf("Temperature :%f C\n",temp);
		sleep(1);
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
	pthread_t temperature;  
	pthread_t pwm;
	
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(20, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour bouton-poussoir 1
	bcm2835_gpio_fsel(19, BCM2835_GPIO_FSEL_INPT);
	
	int range = 600;
	int d = 3;
	bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_ALT5);
	bcm2835_pwm_set_clock(d);
	bcm2835_pwm_set_mode(0,1,1);
	bcm2835_pwm_set_range(0, range);
	bcm2835_pwm_set_data(0,0);
	bcm2835_pwm_set_data(0,600);
	printf("test\n");
	
	// Création du thread "cligne".
	// Lien avec la fonction clignote.
	// Cette dernière n'exige pas de paramètres.
	pthread_create(&cligne, NULL, &clignote, 20);
	pthread_create(&temperature, NULL, &temperatureThread, NULL);
	pthread_create(&pwm, NULL, &pwmThread,NULL);
	
	
	
	
	// Boucle tant que le bouton-poussoir est non enfoncé
	while(bcm2835_gpio_lev(19)){
		usleep(1000); // Délai de 1 ms !!!
	}

	// Si bouton-poussoir enfoncé, arrêt immédiat du thread 
    pthread_cancel(cligne);
    pthread_cancel(temperature);
    pthread_cancel(pwm);
    // Attente de l'arrêt du thread
    pthread_join(cligne, NULL);
    pthread_join(temperature, NULL);
    pthread_join(pwm, NULL);
    
    // Éteindre le DEL rouge
    bcm2835_gpio_write(20, LOW);
    // Libérer le GPIO
    bcm2835_close();
    return 0;
}
