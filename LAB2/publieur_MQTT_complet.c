/*
 * publieur_MQTT_complet.c
 *
 * Tomas Salvado Robalo et Samuel Dupuis
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <mosquitto.h>

#define MQTT_HOSTNAME "10.187.27.153"//Adresse IP du serveur Mosquitto
#define MQTT_QoS 2
#define MQTT_PORT 1883
#define MQTT_TOPIC_TEMPERATURE "capteur/PID/temperature"//TOPIC de la temperature mesurée
#define MQTT_TOPIC_PID_CONSIGNE "capteur/PID/consigne"  // TOPIC de la consigne du PID définie dans le code
#define MQTT_TOPIC_PID_PWM "capteur/PID/pwm"//TOPIC de la valeur PWM du PID calculée
#define MQTT_TOPIC_LED_ROUGE "capteur/led/rouge"// value de la LED rouge mesurée
#define MQTT_TOPIC_LED_JAUNE "capteur/led/jaune"// value de la LED jaune mesurée
#define MQTT_TOPIC_ACTION_LED_ROUGE "actionneur/led/rouge"// action sur la lED rouge demandée
#define MQTT_TOPIC_ACTION_LED_JAUNE "actionneur/led/jaune"// action sur la LED jaune demandée
#define MQTT_TOPIC_ACTION_BOUTON_ROUGE "actionneur/bouton/rouge"// action sur le bouton rouge demandée
#define MQTT_TOPIC_BOUTON_ROUGE "capteur/bouton/rouge"//   value du bouton rouge mesurée
#define MQTT_TOPIC_BOUTON_NOIR "capteur/bouton/noir" //value du bouton noir mesurée
#define MQTT_TOPIC_ACTION_PID_CONSIGNE "actionneur/PID/consigne"// action sur la consigne du PID demandée


#define SPI_MISO 23 // Définition du GPIO associé au MISO du SPI (broche 16)
#define SPI_CS 24 // Définition du GPIO associé au CS du SPI (broche 18)
#define SPI_SCK 25 // Définition du GPIO associe au SCK du SPI (broche 22)

double frequence = 10.0; // Hertz : fréquence de clignotement désirée
double frequence_SPI = 4000000.0; // Hertz : fréquence du SPI
double RANGE = 1500; // valeur du RANGE du pwm
double D = BCM2835_PWM_CLOCK_DIVIDER_256; // valeur du d du pwm 
double erreurK_1 = 0;
double erreurK_2 = 0;
int commande = 0;//valeur de la commande calculée 
int ARRET=0;//commande d'arret

double consigne_PID_save = 0;//valeur de la consigne à l'état n-1
double consigne_PID =24;//valeur de la consigne à l'état n

double temperature_PID_save = 0;// valeur de la température à l'état n-1
double temperature_PID =0;// valeur de la température à l'état n

double pwm_PID =0;//valeur de la commande à l'état n
double pwm_PID_save=24;// valeur de la commande à l'état n-1

int led_rouge_save = 1;//valeur de la led rouge à l'état n-1
int led_rouge=0;//valeur de la led rouge à l'état n

int led_jaune_save = 1;//valeur de la led jaune à l'état n-1
int led_jaune=0;//valeur de la led jaune à l'état n

int bouton_rouge_save=1;//valeur du bouton rouge à l'état n-1
int bouton_rouge=0;//valeur du bouton rouge à l'état n

int bouton_noir_save=0;//valeur du bouton noir à l'état n-1
int bouton_noir=0;//valeur du bouton noir à l'état n

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
        temperature_PID = mesure;
		//int consigne = commande_Thermo(28, mesure);
		int commande_pwm = commande_PID(consigne_PID, mesure, 10, 10, 0.2);
		pwm_PID = commande_pwm*100/RANGE;
        printf("commande : %d\t /Mesure : %f\n",commande_pwm,mesure);
		bcm2835_pwm_set_data(0,commande_pwm);
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
		}
		while(dutyCycle>2){
			bcm2835_pwm_set_data(0,dutyCycle);
			dutyCycle =  dutyCycle -(RANGE/100);
			usleep(10000);
		}
		  }
}



/**
 * Fonction my_message_callback qui sera appelee a la reception d'un message MQTT
 * La fonction ne retourne aucune valeur.
 * La fonction exige en entrée le pointeur vers la structure mosquitto, un pointeur vers un objet utilisateur
 * et un pointeur vers la structure mosquitto_message contenant le message recu.
 * 
 * La fonction traite les messages recus sur les topics d'action et met a jour les variables globales
 * en consequence.
 */
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	char *ptr;
	printf("Topic : %s  --> ", (char*) message->topic);
	printf("Message : %s\n", (char*) message->payload);
	
	double valeur = strtod(message->payload,&ptr);

	// On demande un changement d'état de la LED rouge
	if (!strcmp(message->topic,MQTT_TOPIC_ACTION_LED_ROUGE))
    {
        printf("Changement état LED rouge : %lf \n",valeur);
        led_rouge = (int)valeur;
		//On veut allumer la LED rouge
        if (led_rouge==1){
            bcm2835_gpio_write(20, HIGH);
        }
		//On veut éteindre la LED rouge
        else{
            bcm2835_gpio_write(20, LOW);
        }
    }
	// On demande un changement d'état de la LED jaune
	if (!strcmp(message->topic,MQTT_TOPIC_ACTION_LED_JAUNE))
    {
        printf("Changement état LED jaune : %lf \n",valeur);
        led_jaune = (int)valeur;
		//On veut allumer la LED jaune
        if (led_jaune==1){
            bcm2835_gpio_write(21, HIGH);
        }
		//On veut éteindre la LED jaune
        else{
            bcm2835_gpio_write(21, LOW);
        }
    }
	// On demande un changement d'état du bouton rouge
    if (!strcmp(message->topic,MQTT_TOPIC_ACTION_BOUTON_ROUGE))
	{
		bouton_rouge=(int)valeur;
		//On veut simuler l'appui sur le bouton rouge
		if (bouton_rouge==1){
            printf("Le bouton rouge a été pressé \n");
        }
		//On veut simuler le relachement du bouton rouges
        else{
            printf("Le bouton rouge a été relaché \n");
        }
	
	}
	// On demande un changement de la consigne du PID
    if (!strcmp(message->topic,MQTT_TOPIC_ACTION_PID_CONSIGNE))
    {
        printf("Changement de la consigne du PID : %lf C.\n",valeur);
        consigne_PID = valeur;
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
	pthread_t pwm;  
	
	// Initialisation du bcm2835
	if (!bcm2835_init()){
		return 1;
	}
	
	
	// Configuration du GPIO pour DEL 1 (rouge)
	bcm2835_gpio_fsel(20, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(21, BCM2835_GPIO_FSEL_OUTP);
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
	//pthread_create(&temperature, NULL, &temperatureThread, NULL);
	//pthread_create(&pwm, NULL, &pwmThread,NULL);
	pthread_create(&pwm, NULL, &consigne_temperature,NULL);


	
	
	
	
	char text[40];//chaine de caractere pour stocker le message a publier
	int ret;//variable de retour des fonctions mosquitto
	struct mosquitto *mosq = NULL;//structure mosquitto
    mosquitto_lib_init();//initialisation de la librairie mosquitto
	mosq = mosquitto_new(NULL, true, NULL);//creation d'une nouvelle instance mosquitto
	if (!mosq)// verification de la creation de l'instance
	{
		fprintf(stderr,"Ne peut initialiser la librairie de Mosquitto\n");
		exit(-1);
	}

	//definition de la fonction de rappel pour la reception des messages
	mosquitto_message_callback_set(mosq, my_message_callback);

	//connexion au serveur mosquitto
	ret = mosquitto_connect(mosq, MQTT_HOSTNAME, MQTT_PORT, 60);
	if (ret)
	{
		fprintf(stderr,"Ne peut se connecter au serveur Mosquitto\n");
		exit(-1);
	}
	
	//abonnement au topic d'action de la LED jaune
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_ACTION_LED_JAUNE , MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	//abonnement au topic d'action de la LED rouge
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_ACTION_LED_ROUGE, MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	//abonnement au topic d'action du bouton rouge
    ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_ACTION_BOUTON_ROUGE, MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	//abonnement au topic d'action de la consigne du PID
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_ACTION_PID_CONSIGNE, MQTT_QoS);
	if (ret)
	{
		fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
		exit(-1);
	}
	//demarrage de la boucle de traitement de la réception des messages MQTT
	mosquitto_loop_start(mosq);

	//debut de la boucle principale de publication des messages MQTT
	while(ARRET==0){
		// On publie uniquement si la valeur a changé depuis la dernière publication pour ne pas surcharger le brocker MQTT



		// Publication de la température lue
		if(temperature_PID != temperature_PID_save){//si la température a changé
			temperature_PID_save = temperature_PID;//on met à jour la valeur sauvegardée
			sprintf(text, "%5.2f", temperature_PID_save);//on prépare le message à publier
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_TEMPERATURE, strlen(text), text, MQTT_QoS, false);//on publie le message
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
		}
		
		// Publication de la consigne du PID
		if(consigne_PID != consigne_PID_save){//si la consigne a changé
			//on met à jour la valeur sauvegardée
			consigne_PID_save = consigne_PID;
			// Publication de la consigne
       		sprintf(text, "%5.2f", consigne_PID);
        	ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_PID_CONSIGNE, strlen(text), text, MQTT_QoS, false);
        	if (ret)
        	{
            	fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
            	exit(-1);
        	}
		}
        
		// Publication de la PWM du PID
		if(pwm_PID != pwm_PID_save){//si la PWM a changé
			pwm_PID_save = pwm_PID;
			// Publication de la PWM
        	sprintf(text, "%5.2f", pwm_PID);
        	ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_PID_PWM, strlen(text), text, MQTT_QoS, false);
        	if (ret)
        	{
            	fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
            	exit(-1);
        	}
		}
        
		// Publication de l'état de la LED rouge
		if(led_rouge != led_rouge_save){//si l'état de la LED rouge a changé
			led_rouge_save = led_rouge;
			// Publication de l'état de la LED rouge
        	sprintf(text, "%d", led_rouge);
        	ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_LED_ROUGE, strlen(text), text, MQTT_QoS, false);
        	if (ret)
        	{
            	fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
           		exit(-1);
        	}
		}
        
		// Publication de l'état de la LED jaune
		if(led_jaune != led_jaune_save){//si l'état de la LED jaune a changé
			led_jaune_save = led_jaune;
			// Publication de l'état de la LED jaune
			sprintf(text, "%d", led_jaune);
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_LED_JAUNE, strlen(text), text, MQTT_QoS, false);
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
		}
		// Publication de l'état du bouton noir
		if(!bcm2835_gpio_lev(19) != bouton_noir_save){//si l'état du bouton noir a changé
			bouton_noir_save = !bcm2835_gpio_lev(19);
			
			sprintf(text, "%d", bouton_noir_save);
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_BOUTON_NOIR, strlen(text), text, MQTT_QoS, false);
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
		}

		// Publication de l'état du bouton rouge si le bouton physique ou virtuel a changé
		if((!bcm2835_gpio_lev(26) != bouton_rouge_save)||(bouton_rouge != bouton_rouge_save) ){
			//arrêt si le bouton physique est pressé ou si le bouton virtuel est pressé
			if(!bcm2835_gpio_lev(26)==1 || bouton_rouge==1)
				ARRET=1;
			//mise à jour de l'état du bouton rouge si le bouton physique a changé
			if(!bcm2835_gpio_lev(26) != bouton_rouge_save)
				bouton_rouge_save = !bcm2835_gpio_lev(26);
			//mise à jour de l'état du bouton rouge si le bouton virtuel a changé
			else if(bouton_rouge != bouton_rouge_save)
				bouton_rouge_save = bouton_rouge;
			
			
			
			sprintf(text, "%d", bouton_rouge_save); //préparation du message à publier
			ret = mosquitto_publish(mosq, NULL, MQTT_TOPIC_BOUTON_ROUGE, strlen(text), text, MQTT_QoS, false);
			if (ret)
			{
				fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
				exit(-1);
			}
		}

		sleep(1);//attente avant la prochaine itération pour ne pas surcharger le CLient MQTT
	}

	//arrêt de la boucle de traitement des messages MQTT
	mosquitto_loop_stop(mosq, true);
	//libération de la structure mosquitto
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	//arrêt des threads
    pthread_cancel(pwm);
    pthread_join(pwm, NULL);
    
	// Éteindre le PWM
	bcm2835_pwm_set_data(0,0);
	
	bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(18, HIGH);
	
    // Libérer le GPIO
    bcm2835_close();
    return 0;
}
