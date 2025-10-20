all: publieur_MQTT_complet abonne_MQTT_complet
publieur_MQTT_complet: publieur_MQTT_complet.c
	gcc -o lab2_publieur publieur_MQTT_complet.c -lbcm2835 -lpthread -lmosquitto
abonne_MQTT_complet: abonne_MQTT_complet.c
	gcc -o lab2_abonne abonne_MQTT_complet.c -lbcm2835 -lpthread -lmosquitto
clean:
	rm -f lab2
