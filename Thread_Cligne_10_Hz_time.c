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
double RANGE = 1500; // valeur du RANGE du pwm
double D = BCM2835_PWM_CLOCK_DIVIDER_256; // valeur du d du pwm 
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

/* Fonction commande_Thermo qui sera appelee depuis un thread
 * 
 * Elle active ou desactive le chauffage selon la consigne et la mesure
 *
 * La fonction retourne 0 si le chauffage est desactive et RANGE si il est active
 * La fonction exige en entrée la consigne et la mesure de temperature.
*/
int commande_Thermo(double consigne, double mesure){
	if(consigne < mesure){
		return 0; // Chauffage desactive
	}
	else{
		return RANGE; // Chauffage active
	}
}

/* Fonction commande_PID qui sera appelee depuis un thread
 * 
 * Elle active ou desactive le chauffage selon la consigne et la mesure
 * en utilisant un controleur PID
 *
 * La fonction retourne une valeur entre 0 et RANGE pour le chauffage
 * La fonction exige en entrée la consigne, la mesure de temperature,
 * le gain proportionnel, le gain integrale et le gain derive.
*/
int commande_PID(double consigne, double mesure, double gainKp, double gainKi, double gainKd){
	double erreur = consigne - mesure;
	consigne = consigne + gainKp*(erreur-erreurK_1) + (gainKi*1/2)*(erreur+erreurK_1) + (gainKd/2)*(erreur - 2*erreurK_1 + erreurK_2);
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

/* Fonction consigne_temperature qui sera appelee dans le main
 * 
 * Elle lit la temperature et ajuste la consigne du chauffage
 *
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
*/
int consigne_temperature(){
	/*while(1){
		double mesure = lecture_SPI();
		int consigne = commande_Thermo(25, mesure);
		// int consigne = commande_PID(25, mesure, 5, 0.1, 1);
		printf("Consigne : %d\t /Mesure : %f\n",consigne,mesure);
		bcm2835_pwm_set_data(0,consigne);
		sleep(1);
	}*/
	bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_OUTP);
	while(1){
		bcm2835_gpio_write(18, HIGH);
		sleep(1);
	}

}




/**
 * Fonction pwmThread qui permet de faire varier la luminosite de la lampe
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
void pwmThread(){
	int dutyCycle = 0;
	while(1){
		dutyCycle = 0;
		while(dutyCycle < RANGE){
			bcm2835_pwm_set_data(0,dutyCycle);
			dutyCycle= (RANGE/100) + dutyCycle;
			usleep(10000);
			printf("dutyCycle = %d\n",dutyCycle);
		}
		while(dutyCycle>2){
			bcm2835_pwm_set_data(0,dutyCycle);
			dutyCycle =  dutyCycle -(RANGE/100);
			usleep(10000);
			printf("dutyCycle = %d\n",dutyCycle);
		}
		  }
		
		//bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_OUTP);
		//bcm2835_gpio_write(18, HIGH);
	
}

/* Fonction writeTemptoFile qui sera appelee dans un thread
 * 
 * Elle ecrit la temperature dans un fichier texte
 *
 * La fonction ne retourne aucune valeur.
 * La fonction exige en entrée la temperature a ecrire.
*/
void writeTemptoFile(double temp){
	FILE *fptr;
	fptr = fopen("temperature.txt", "a");
	if(fptr == NULL){
		printf("Erreur lors de l'ouverture du fichier\n");
		return;
	}
	fprintf(fptr, "%f\n", temp);
	fclose(fptr);
}

/**
 * Fonction temperatureThread qui permet de lire la temperature
 * La fonction ne retourne aucune valeur.
 * La fonction n'exige aucun paramètre en entrée.
 */
void temperatureThread(void){
	
	while(1){
		double temp = lecture_SPI();
		printf("Temperature :%f C\n",temp);
		writeTemptoFile(temp);
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
	
	//configuration du GPIO pour la sortie PWM0
	bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_ALT5);
	bcm2835_pwm_set_clock(D);
	bcm2835_pwm_set_mode(0,1,1);
	bcm2835_pwm_set_range(0, RANGE);
	bcm2835_pwm_set_data(0,0);

	// Configuration du GPIO pour MISO
	bcm2835_gpio_fsel(SPI_MISO, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO pour SPI_CS
	bcm2835_gpio_fsel(SPI_CS, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour SPI_SCK
	bcm2835_gpio_fsel(SPI_SCK, BCM2835_GPIO_FSEL_OUTP);
	
	// Création des threads
	pthread_create(&cligne, NULL, &clignote, 20);
	pthread_create(&temperature, NULL, &temperatureThread, NULL);
	//pthread_create(&pwm, NULL, &pwmThread,NULL);
	pthread_create(&pwm, NULL, &consigne_temperature,NULL);
	
	
	
	
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
	// Éteindre le PWM
	bcm2835_pwm_set_data(0,0);
	
	bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(18, HIGH);
	
    // Libérer le GPIO
    bcm2835_close();
    return 0;
}
