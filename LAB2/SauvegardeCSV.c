/*
 * SauvegardeCSV.c
 * 
 * Exemple de programme pour céer un fichier CSV
 * avec l'heure et la date actuelle
 *  
 * 
 */

#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

 
char date_et_heure[20];
double frequence_SPI = 4000000.0; // Hertz : fréquence du SPI
#define SPI_MISO 23 // Définition du GPIO associé au MISO du SPI (broche 16)
#define SPI_CS 24 // Définition du GPIO associé au CS du SPI (broche 18)
#define SPI_SCK 25 // Définition du GPIO associe au SCK du SPI (broche 22)

double RANGE = 1500; // valeur du RANGE du pwm
double D = BCM2835_PWM_CLOCK_DIVIDER_256; // valeur du d du pwm 

double erreurK_1 = 0;
double erreurK_2 = 0;
int commande = 0;

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
		
	// On met le CS a 1
	bcm2835_gpio_write(SPI_CS, HIGH);
	
	//Depart de la routine
	// On met le CS a 0
	bcm2835_gpio_write(SPI_CS, LOW);
	
	int nbClock=0;
	
	// Boucle infinie pour le clignotement du clock
	while(nbClock!=32){
		
			//etat haut du sck
			bcm2835_gpio_write(SPI_SCK, HIGH);
			sigMiso = (sigMiso << 1) + bcm2835_gpio_lev(SPI_MISO);// Décalage de sigMiso et ajout du bit lu
			sleep(1/(2.0*frequence_SPI));
			//etat bas du sck
			bcm2835_gpio_write(SPI_SCK, LOW);
			nbClock++;
			sleep(1/(2.0*frequence_SPI));
	}
	
	// Fin de la routine
	// On remonte le CS a 1
	bcm2835_gpio_write(SPI_CS, HIGH);
	

	// On extrait la temperature du signal MISO
	unsigned int maskENT = 0b01111111111100000000000000000000;// Masque pour les bits de la partie entiere
	unsigned int maskDEC = 0b00000000000011000000000000000000;// Masque pour les bits de la partie decimale
	unsigned int maskNEG = 0b10000000000000000000000000000000;// Masque pour le bit de signe
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

	return (double)temp;
}

/*
* Fonction commande_PID qui sera appelee depuis un thread
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
	commande = commande + gainKp*(erreur-erreurK_1) + (gainKi*1/2)*(erreur+erreurK_1) + (gainKd/2)*(erreur - 2*erreurK_1 + erreurK_2);
	erreurK_2 = erreurK_1;
	erreurK_1 = erreur;

	if (commande < 0){
		commande = 0;
		return 0; // Chauffage desactive
	}
	else if (commande > RANGE){
		commande = RANGE;
		return RANGE; // Chauffage active
	}
	else{
		return (int)commande;
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
	while(1){
		double mesure = lecture_SPI();
		int commande_pwm = commande_PID(56, mesure, 10, 10, 0.2);
		printf("commande : %d\t /Mesure : %f\n",commande_pwm,mesure);
		bcm2835_pwm_set_data(0,commande_pwm);
		sleep(1);
	}
}

int main() {
	
		// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	pthread_t pwm;
	
	// Configuration du GPIO pour MISO
	bcm2835_gpio_fsel(SPI_MISO, BCM2835_GPIO_FSEL_INPT);
	// Configuration du GPIO pour SPI_CS
	bcm2835_gpio_fsel(SPI_CS, BCM2835_GPIO_FSEL_OUTP);
	// Configuration du GPIO pour SPI_SCK
	bcm2835_gpio_fsel(SPI_SCK, BCM2835_GPIO_FSEL_OUTP);


  //configuration du GPIO pour la sortie PWM0
	bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_ALT5);
	bcm2835_pwm_set_clock(D);
	bcm2835_pwm_set_mode(0,1,1);
	bcm2835_pwm_set_range(0, RANGE);
	bcm2835_pwm_set_data(0,0);

	pthread_create(&pwm, NULL, &consigne_temperature,NULL);
	


FILE *fp = fopen("EquipeD_Temp_PID_10min.csv", "w");
if (fp == NULL) {
    perror("Erreur lors de l'ouverture du fichier");
    return -1;
}
fprintf(fp, "Date et heure,Adresse MAC,Emplacement,Temperature\n");

for(int i=0; i<300; i++) {
  // Capturer l'heure et la date actuelle
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  strftime(date_et_heure, 20, "%Y-%m-%d %H:%M:%S",tm);

  // Capturer la température
  double temperature = lecture_SPI();

  // Écrire les données dans un fichier CSV 
  fprintf(fp, "%s,%s,%s,%.1f\n", date_et_heure, "b8:27:eb:a2:f4:62", "Poste 4",temperature);
  sleep(2);
}
fclose(fp);

pthread_cancel(pwm);
pthread_join(pwm, NULL);


bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_OUTP);
bcm2835_gpio_write(18, HIGH);
	
    // Libérer le GPIO
    bcm2835_close();

return 0;
}

