/*
 * FenetreHeure.c
 *
 * Exemple de programme pour Afficher l'heure dans une fenêtre 
 *
 *
 */
 
 #include <gtk/gtk.h>
#include <time.h>

static gboolean update_time(gpointer label) {
time_t now = time(0);
char time_str[100];
strftime(time_str, 100, "%H:%M:%S", localtime(&now));
gtk_label_set_text(GTK_LABEL(label), time_str);
return TRUE;
}

static void destroy(GtkWidget *widget, gpointer data) {
gtk_main_quit();
}

int main(int argc, char *argv[]) {
gtk_init(&argc, &argv);

GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
gtk_window_set_title(GTK_WINDOW(window), "Heure");
gtk_container_set_border_width(GTK_CONTAINER(window), 10);
gtk_widget_set_size_request(window, 100, 75);

GtkWidget *label = gtk_label_new("----");
gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
gtk_container_add(GTK_CONTAINER(window), label);

g_timeout_add(1000, update_time, label);

gtk_widget_show_all(window);
g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

gtk_main();

return 0;
}
