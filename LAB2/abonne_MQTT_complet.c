/*
 * abonne_MQTT.c
 * 
 * 
 * Lokman Sboui et Guy Gauthier
 * 
 */
 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mosquitto.h>

#define MQTT_HOSTNAME "localhost"
#define MQTT_QoS 2
#define MQTT_PORT 1883
#define MQTT_TOPIC_TEMPERATURE "capteur/PID/temperature"// on écoute
#define MQTT_TOPIC_PID_CONSIGNE "capteur/PID/consigne"  // on écoute
#define MQTT_TOPIC_PID_PWM "capteur/PID/pwm"// on écoute
#define MQTT_TOPIC_LED_ROUGE "capteur/led/rouge"// on écoute
#define MQTT_TOPIC_LED_JAUNE "capteur/led/jaune"// on écoute
#define MQTT_TOPIC_ACTION_LED_ROUGE "actionneur/led/rouge"// on écoute pas
#define MQTT_TOPIC_ACTION_LED_JAUNE "actionneur/led/jaune"// on écoute pas
#define MQTT_TOPIC_ACTION_BOUTON_ROUGE "actionneur/bouton/rouge"// on écoute pas 
#define MQTT_TOPIC_BOUTON_ROUGE "capteur/bouton/rouge"// On écoute
#define MQTT_TOPIC_BOUTON_NOIR "capteur/bouton/noir" // On écoute 

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

	if (!strcmp(message->topic,MQTT_BOUTON_ROUGE))
	{
		printf("Le bouton rouge a été pressé \n");
        mosquitto_destroy(mosq);
	    mosquitto_lib_cleanup();
		exit(0);
	}
    if (!strcmp(message->topic,MQTT_BOUTON_NOIR))
    {
        printf("Le bouton noir a été pressé \n");
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

