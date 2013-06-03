/* pandora.h
 *
 * Auteurs : Brice Errandonea et Manuel De Palma
 *
 * Pour compiler : make
 *
 */

#ifndef PANDORA_H
#define PANDORA_H
#define ETIS	//À commenter quand on n'est pas à l'ETIS
//------------------------------------------------BIBLIOTHEQUES------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>

#include <net_message_debug_dist.h>
#include "colors.h"
#include "prom_tools/include/xml_tools.h"
#include "enet/include/enet/enet.h"
#include "prom_kernel/include/pandora_connect.h"
#include "prom_kernel/include/reseau.h"
#include "prom_user/include/Struct/prom_images_struct.h"

#define PANDORA_PORT 1235
#define BROADCAST_IP "127.255.255.255"

//------------------------------------------------LIMITES---------------------------------------------------------------

#define COLOR_MAX 7


//Si ces limites se révèlent trop restrictives (ou trop larges), éditer les valeurs de cette rubrique
#define NB_WINDOWS_MAX 30 //Nombre maximal de petites fenêtres affichables dans le bandeau du bas
#define NB_ROWS_MAX 4 //Nombre maximal de lignes de petites fenêtres affichant des neurones
#define NB_BUFFERED_MAX 100 //Nombre maximal d'instantanés stockés

#define NB_PLANES_MAX 7
//Le nombre maximal de plans affichables simultanément dans la zone 3D est limité à 7. Cette limite là est un peu plus
//compliquée à modifier car il faudrait définir de nouvelles couleurs pour chaque valeur de z > 6.
//De toute façon, 8 plans superposés seraient difficilement lisibles (7, déjà...).

#define SCRIPT_NAME_MAX 30 //Longueur maximale (en caractères) du nom d'un script
#define GROUP_NAME_MAX 30

#define IP_LENGTH_MAX 16  //Une adresse IPv4 comporte 15 caractères, + 1 pour le '\0' final
#define SCRIPT_LINKS_MAX 128

//-------------------------------------------------CONSTANTES----------------------------------------------------------

#define MESSAGE_MAX 256 //Utilisé par la fonction fatal_error dans pandora.c
#define SHOWCOLOR_SIZE 20
#define GRID_WIDTH 0.2 //Épaisseur des lignes de la grille
#define RECEPTION_DELAY 2 //Délai (en secondes) pendant lequel Pandora attend les scripts qu'il a demandé
//Dimension du rectangle représentant un groupe
#define LARGEUR_GROUPE 96
#define HAUTEUR_GROUPE 40

//Échelles par défaut
#define REFRESHSCALE_DEFAULT 30
#define XSCALE_DEFAULT 128
#define YSCALE_DEFAULT 58
#define XGAP_DEFAULT 32
#define YGAP_DEFAULT 24
#define DIGITS_DEFAULT 4 //Nombre de caractères pour afficher les valeurs d'un neurone
#define LABEL_MAX 128

#define NB_Max_VALEURS_ENREGISTREES 50

//-----------------------------------------------ENUMERATIONS--------------------------------------------------------

enum {
  NET_LINK_SIMPLE = 0,
  NET_LINK_BLOCK = 1 << 0,
  NET_LINK_ACK = 1 << 1,
};

enum{
  DISPLAY_MODE_SQUARE = 0,
  DISPLAY_MODE_INTENSITY,
  DISPLAY_MODE_BAR_GRAPH,
  DISPLAY_MODE_TEXT,
  DISPLAY_MODE_GRAPH
};

//------------------------------------------------STRUCTURES---------------------------------------------------------

typedef struct coordonnees {
  int x;
  int y;
} coordonnees;

/*
 typedef struct neuron
 {
 struct group *myGroup;
 float s[4]; //Valeurs du neurone (0 : s, 1 : s1, 2 : s2, 3 : pic)
 int x;
 int y;
 float buffer[4][NB_BUFFERED_MAX];
 } neuron;
 */


typedef struct group {
  int id;
  struct script *script;
  char name[SIZE_NO_NAME];
  char function[TAILLE_CHAINE];
  int number_of_neurons;
  int rows;
  int columns;
  type_neurone *neurons; //Tableau des neurones du groupe
  int x, y;
  gboolean knownX; //TRUE si la coordonnée x est connue
  gboolean knownY;
  int number_of_links;
  struct group **previous; //Adresses des groupes ayant des liaisons vers celui-ci

  int firstNeuron; ///Numéro du premier neurone de ce groupe dans le grand tableau de tous les neurones du script
  int nb_update_since_next;
  float val_min, val_max;
  void *widget, *drawing_area, *label;
  void *entry_val_min, *entry_val_max;
  void *ext, *dialog;
  void *path;
  int output_display, display_mode;
  int normalized;

  /// enregistrement des valeurs S, S1 et S2, utilisées pour tracer le graphe. [ligne][colonne][s(0), s1(1) ou s2(2)][numValeur]
  float ****tabValues;
  int **indexDernier, **indexAncien;

  /// variables utilisées pour la fréquence moyenne
  float frequence;
  int nb_affiche;

  sem_t sem_update_neurons;
  int counter;
  GTimer *timer;
  GThread *thread;
} type_group;

typedef struct script {
  int id;
  char name[LOGICAL_NAME_MAX];
  char machine[HOST_NAME_MAX];
  int color, y_offset, height, z;
  int number_of_groups;
  type_group *groups; //Tableau des groupes du script
  int displayed; //TRUE s'il faut afficher le script
  ENetPeer *peer;
  void *widget, *label, *checkbox, *z_spinnner, *search_button;
  sem_t sem_groups_defined;
} type_script;

typedef struct script_link {
  char name[TAILLE_CHAINE];
  type_group *previous;
  type_group *next;
  int type;
} type_script_link;




extern int period;




extern int number_of_scripts; //Nombre de scripts à afficher
char scriptsNames[NB_SCRIPTS_MAX][SCRIPT_NAME_MAX]; //Tableau des noms des scripts
extern type_script *scripts[NB_SCRIPTS_MAX]; //Tableau des scripts à afficher
extern int newScriptNumber; //Numéro donné à un script quand on l'ouvre
extern int zMax; //la plus grande valeur de z parmi les scripts ouverts
extern int buffered; //Nombre d'instantanés actuellement en mémoire

extern int usedWindows;
extern type_group *selected_group; //Pointeur sur le groupe actuellement sélectionné
extern int selectedWindow; //Numéro de la fenêtre sélectionnée (entre 0 et NB_WINDOWS_MAX-1). NB_WINDOWS_MAX indique qu'aucune fenêtre n'est sélectionnée

extern GtkWidget *pHBox2; //Panneau des neurones
extern GtkWidget *pFrameNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de petites fenêtres pour les neurones
extern GtkWidget *zoneNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de zones où dessiner des neurones
extern type_group *windowGroup[NB_WINDOWS_MAX]; //Adresse des groupe affiché dans la zoneNeurones de même indice
extern int windowValue[NB_WINDOWS_MAX]; //Numéro disant quelle valeur des neurones du groupe il faut afficher dans la fenêtre de même indice (0 : s, 1 : s1, 2 : s2, 3 : pic)

extern int nbColonnesTotal; //Nombre total de colonnes de neurones dans les fenêtres du bandeau du bas
extern int nbLignesMax; //Nombre maximal de lignes de neurones à afficher dans l'une des fenêtres du bandeau du bas

//Pour pandora_receive_from_prom.c
extern int ivyServerNb;

extern ENetHost *enet_server;

extern type_script_link net_links[SCRIPT_LINKS_MAX];
extern int number_of_net_links;
//------------------------------------------------PROTOTYPES--------------------------------------------------------

void init_pandora(int argc, char** argv);
void prom_bus_init(const char *ip);
void script_update_positions(type_script *script);

//Signaux
void drag_drop_neuron_frame(GtkWidget *zone_neurons, GdkEventButton *event, gpointer data);
void expose_event_zone_neuron(GtkWidget *zone_neurons, gpointer pData);
void on_script_displayed_toggled(GtkWidget *pWidget, gpointer pData);
void Close(GtkWidget *pWidget, gpointer pData);
void change_plan(GtkWidget *pWidget, gpointer pData); //Un script change de plan
void changeValue(GtkWidget *pWidget, gpointer pData);
void on_search_group_button_active(GtkWidget *pWidget, type_script *script);
void on_hide_see_scales_button_active(GtkWidget *hide_see_scales_button, gpointer pData);
void on_check_button_draw_active(GtkWidget *check_button, gpointer data);

void button_press_event(GtkWidget *pWidget, GdkEventButton *event);
void key_press_event(GtkWidget *pWidget, GdkEventKey *event);
void save_preferences(GtkWidget *pWidget, gpointer pData);
void save_preferences_as(GtkWidget *pWidget, gpointer pData);
void pandora_load_preferences(GtkWidget *pWidget, gpointer pData);
void defaultScale(GtkWidget *pWidget, gpointer pData);
void askForScripts(GtkWidget *pWidget, gpointer pData);

void group_expose_neurons(type_group *group);

//"Constructeurs"
void init_script(type_script *s, char *name, char *machine, int z, int nbGroups, int id);
void new_neuron(int script_id);
void destroyAllScripts();
void group_init(type_group *g, int group_id, type_script *myScript, char *name, char *function, float learningSpeed, int nbNeurons, int rows, int columns, int nbLinksTo);
//Mise un jour d'un neurone quand Prométhé envoie de nouvelles données
void neuron_update(type_neurone *n, float s, float s1, float s2, float pic);
//Mise un jour d'un groupe quand Prométhé envoie de nouvelles données
void updateGroup(type_group *g, float learningSpeed, float execTime);

void script_update_display(type_script *script);
void script_destroy(type_script *script);

//Autres
gboolean neurons_refresh_display();
const char* tcolor(type_script *script);
void color(cairo_t *cr, type_group *g);
void clearColor(cairo_t *cr, type_group g);
void group_new_display(type_group *g, float pos_x, float pos_y);
gfloat niveauDeGris(float val, float valMin, float valMax);
void resizeNeurons();
void pandora_file_save(char *filename);
void pandora_file_load(char *filename);

void fatal_error(const char *name_of_file, const char *name_of_function, int numero_of_line, const char *message, ...);
void pandora_bus_send_message(char *id, const char *format, ...);
void server_for_promethes();
#endif
