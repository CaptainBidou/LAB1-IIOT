/*
 * publieur_MQTT.c
 * 
 * 
 * Lokman Sboui et Guy Gauthier
 * 
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

#define SPI_MISO 23 // Définition du GPIO associé au MISO du SPI (broche 16)
#define SPI_CS 24 // Définition du GPIO associé au CS du SPI (broche 18)
#define SPI_SCK 25 // Définition du GPIO associe au SCK du SPI (broche 22)

#define MQTT_HOSTNAME "localhost"
#define MQTT_PORT 1883
#define MQTT_TOPIC1 "capteur/zone1/temperature"
#define MQTT_TOPIC2 "capteur/zone1/pression"
#define MQTT_TOPIC3 "capteur/zone1/bouton_rouge"

double frequence_SPI = 4000000.0; // Hertz : fréquence du SPI

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

int main(int argc, char **argv)
{
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

	char text[40];
	int ret;
	struct mosquitto *mosq = NULL;
	
	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, NULL);
	if (!mosq)
	{
		fprintf(stderr,"Ne peut initialiser la librairie de Mosquitto\n");
		exit(-1);
	}
	
	ret = mosquitto_connect(mosq, MQTT_HOSTNAME, MQTT_PORT, 60);
	if (ret)
	{
		fprintf(stderr,"Ne peut se connecter au serveur Mosquitto\n");
		exit(-1);
	}
	while(bcm2835_gpio_lev(19)){
		double temp = lecture_SPI();
		// Publication de la temperature lue
		sprintf(text, "%5.2f C", temp);
		ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC1, strlen(text), text, 0, false);
		if (ret)
		{
			fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
			exit(-1);
		}
		sleep(1);
	}
	sprintf(text, "1");
	ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC3, strlen(text), text, 0, false);
	if (ret)
		{
			fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
			exit(-1);
		}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	return 0;
}

