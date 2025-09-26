/*
 * SauvegardeCSV.c
 * 
 * Exemple de programme pour céer un fichier CSV
 * avec l'heure et la date actuelle
 *  
 * 
 */

#include <stdio.h> 
#include <time.h>

 
char date_et_heure[20];
 

int main() {

// Capturer l'heure et la date actuelle
time_t t = time(NULL);
struct tm *tm = localtime(&t);
strftime(date_et_heure, 20, "%Y-%m-%d %H:%M:%S",tm);

// Écrire les données dans un fichier CSV 

FILE *fp = fopen("temperatures.csv", "w");
fprintf(fp, "Date et heure,Adresse MAC,Emplacement,Temperature\n");
fprintf(fp, "%s,%s,%s,%.1f\n", date_et_heure, "11:22:33:44:55:66", "Poste 13", 24.5);

fclose(fp);

  return 0;
}

