/*
 * abonne_MQTT_complet.c
 * 
 * 
 * Tomas Salvado Robalo et Samuel Dupuis
 * 
 */
 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
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

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	char *ptr;
	printf("Topic : %s  --> ", (char*) message->topic);
	printf("Message : %s\n", (char*) message->payload);
	
	double valeur = strtod(message->payload,&ptr);
	if (!strcmp(message->topic,MQTT_TOPIC_TEMPERATURE))
    {
        printf("La temperature de la lampe est de %lf C.\n",valeur);
    }

	if (!strcmp(message->topic,MQTT_TOPIC_BOUTON_ROUGE))
	{
        if(valeur==1){
            printf("Le bouton rouge a été pressé \n");
            mosquitto_destroy(mosq);
	        mosquitto_lib_cleanup();
		    exit(0);
        }
        else{
            printf("Le bouton rouge a été relâché \n");
        }
		
	}
    if (!strcmp(message->topic,MQTT_TOPIC_BOUTON_NOIR))
    {
       if(valeur==1){
            printf("Le bouton noir a été pressé \n");
        }
        else{
            printf("Le bouton noir a été relâché \n");
        }
    }
    if (!strcmp(message->topic,MQTT_TOPIC_PID_CONSIGNE))
    {
        printf("La consigne du PID est de %lf C.\n",valeur);
    }
    if (!strcmp(message->topic,MQTT_TOPIC_PID_PWM))
    {
        printf("La valeur PWM du PID est de %lf \n",valeur);
    }
    if (!strcmp(message->topic,MQTT_TOPIC_LED_ROUGE))
    {
        printf("L'état de la LED rouge est de %lf \n",valeur);
    }
    if (!strcmp(message->topic,MQTT_TOPIC_LED_JAUNE))
    {
        printf("L'état de la LED jaune est de %lf \n",valeur);
    }
}

int main(int argc, char **argv)
{
	int ret;
	struct mosquitto *mosq = NULL;
	
	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, NULL);
	if (!mosq)
	{
		fprintf(stderr,"Ne peut initialiser la librairie de Mosquitto\n");
		exit(-1);
	}
	mosquitto_message_callback_set(mosq, my_message_callback);
	ret = mosquitto_connect(mosq, MQTT_HOSTNAME, MQTT_PORT, 60);
	if (ret)
	{
		fprintf(stderr,"Ne peut se connecter au serveur Mosquitto\n");
		exit(-1);
	}
	
	ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_TEMPERATURE, MQTT_QoS);
    if (ret)
    {
        fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
        exit(-1);
    }
    ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_BOUTON_ROUGE, MQTT_QoS);
    if (ret)
    {
        fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
        exit(-1);
    }
    ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_BOUTON_NOIR, MQTT_QoS);
    if (ret)
    {
        fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
        exit(-1);
    }
    ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_PID_CONSIGNE, MQTT_QoS);
    if (ret)
    {
        fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
        exit(-1);
    }
    ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_PID_PWM, MQTT_QoS);
    if (ret)
    {
        fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
        exit(-1);
    }
    ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_LED_ROUGE, MQTT_QoS);
    if (ret)
    {
        fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
        exit(-1);
    }
    ret = mosquitto_subscribe(mosq, NULL, MQTT_TOPIC_LED_JAUNE, MQTT_QoS);
    if (ret)
    {
        fprintf(stderr,"Ne peut publier sur le serveur Mosquitto\n");
        exit(-1);
    }
	
	
	mosquitto_loop_start(mosq);
	while(1); 
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	return 0;
}

