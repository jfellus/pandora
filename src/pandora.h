/*
Copyright  ETIS — ENSEA, Université de Cergy-Pontoise, CNRS (1991-2014)
promethe@ensea.fr

Authors: P. Andry, J.C. Baccon, D. Bailly, A. Blanchard, S. Boucena, A. Chatty, N. Cuperlier, P. Delarboulas, P. Gaussier, 
C. Giovannangeli, C. Grand, L. Hafemeister, C. Hasson, S.K. Hasnain, S. Hanoune, J. Hirel, A. Jauffret, C. Joulain, A. Karaouzène,  
M. Lagarde, S. Leprêtre, M. Maillard, B. Miramond, S. Moga, G. Mostafaoui, A. Pitti, K. Prepin, M. Quoy, A. de Rengervé, A. Revel ...

See more details and updates in the file AUTHORS 

This software is a computer program whose purpose is to simulate neural networks and control robots or simulations.
This software is governed by the CeCILL v2.1 license under French law and abiding by the rules of distribution of free software. 
You can use, modify and/ or redistribute the software under the terms of the CeCILL v2.1 license as circulated by CEA, CNRS and INRIA at the following URL "http://www.cecill.info".
As a counterpart to the access to the source code and  rights to copy, modify and redistribute granted by the license, 
users are provided only with a limited warranty and the software's author, the holder of the economic rights,  and the successive licensors have only limited liability. 
In this respect, the user's attention is drawn to the risks associated with loading, using, modifying and/or developing or reproducing the software by the user in light of its specific status of free software, 
that may mean  that it is complicated to manipulate, and that also therefore means that it is reserved for developers and experienced professionals having in-depth computer knowledge. 
Users are therefore encouraged to load and test the software's suitability as regards their requirements in conditions enabling the security of their systems and/or data to be ensured 
and, more generally, to use and operate it in the same conditions as regards security. 
The fact that you are presently reading this means that you have had knowledge of the CeCILL v2.1 license and that you accept its terms.
*/
/** pandora.h
 *
 * Auteurs : Brice Errandonea et Manuel De Palma
 *
 * Pour compiler : make
 *
 **/

#ifndef PANDORA_H
#define PANDORA_H
#define ETIS	//à commenter quand on n'est pas à ETIS
//------------------------------------------------BIBLIOTHEQUES------------------------------------------------------

#include "common.h"
#include "pandora_ivy.h"

#define PANDORA_PORT 1235
#define BROADCAST_IP "127.255.255.255"

//------------------------------------------------LIMITES---------------------------------------------------------------

#define COLOR_MAX 7

//Si ces limites se revelent trop restrictives (ou trop larges), editer les valeurs de cette rubrique
#define NB_WINDOWS_MAX 30 //Nombre maximal de petites fenetres affichables dans le bandeau du bas
#define NB_ROWS_MAX 4 //Nombre maximal de lignes de petites fenetres affichant des neurones
#define NB_BUFFERED_MAX 100 //Nombre maximal d'instantanes stockes
#define NB_PLANES_MAX 7
//Le nombre maximal de plans affichables simultanement dans la zone 3D est limite à 7. Cette limite la est un peu plus
//compliquee a modifier car il faudrait definir de nouvelles couleurs pour chaque valeur de z > 6.
//De toute façon, 8 plans superposes seraient difficilement lisibles (7, deja...).

#define SCRIPT_NAME_MAX 30 //Longueur maximale (en caracteres) du nom d'un script
#define GROUP_NAME_MAX 30

#define IP_LENGTH_MAX 16  //Une adresse IPv4 comporte 15 caracteres, + 1 pour le '\0' final
#define SCRIPT_LINKS_MAX 128

//-------------------------------------------------CONSTANTES----------------------------------------------------------

#define MESSAGE_MAX 256 //Utilis������ par la fonction fatal_error dans pandora.c
#define SHOWCOLOR_SIZE 20
#define GRID_WIDTH 0.2 //������paisseur des lignes de la grille
#define RECEPTION_DELAY 2 //D������lai (en secondes) pendant lequel Pandora attend les scripts qu'il a demand������
//Dimension du rectangle repr������sentant un groupe
#define LARGEUR_GROUPE 96
#define HAUTEUR_GROUPE 40

//������chelles par d������faut
#define REFRESHSCALE_DEFAULT 30
#define XSCALE_DEFAULT 128
#define XSCALE_MIN 10
#define XSCALE_MAX 350
#define YSCALE_DEFAULT 58
#define YSCALE_MIN 10
#define YSCALE_MAX 350
#define XGAP_DEFAULT 32
#define YGAP_DEFAULT 24
#define DIGITS_DEFAULT 4 //Nombre de caract������res pour afficher les valeurs d'un neurone
#define LABEL_MAX 128

#define GRAPH_HEIGHT 160
#define GRAPH_WIDTH 330
#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 25
#define BIG_GRAPH_MAX_NEURONS_NUMBER 8
#define NB_Max_VALEURS_ENREGISTREES 200
#define FREQUENCE_MAX_VALUES_NUMBER 20
#define ZOOM_GAP 10

#define APPLY_SCRIPT_GROUPS_CARACTERISTICS 1
#define SAVE_SCRIPT_GROUPS_CARACTERISTICS 2

#define STAT_HISTORIC_MAX_NUMBER 50
#define MAX_LENGHT_PATHNAME 200
#define CHEMIN "./pandora.pandora"
//-----------------------------------------------ENUMERATIONS--------------------------------------------------------

enum {
  REFRESH_MODE_AUTO = 0, REFRESH_MODE_MANUAL
};

enum {
  NET_LINK_SIMPLE = 0, NET_LINK_BLOCK = 1 << 0, NET_LINK_ACK = 1 << 1,
};

enum {
  DISPLAY_MODE_SQUARE = 0, DISPLAY_MODE_CIRCLE, DISPLAY_MODE_INTENSITY, DISPLAY_MODE_INTENSITY_FAST, DISPLAY_MODE_BAR_GRAPH, DISPLAY_MODE_TEXT, DISPLAY_MODE_GRAPH, DISPLAY_MODE_BIG_GRAPH
};

enum{
  CHECKBOX=0, VUE_METRE, NUMBER_OF_CONTROL_TYPE,
};


//------------------------------------------------STRUCTURES---------------------------------------------------------

typedef struct type_link_draw type_link_draw;

typedef struct coordonnees {
  int x;
  int y;
} coordonnees;

typedef struct stat_group_execution {
  long last_time_phase_0, last_time_phase_1, last_time_phase_3;
  long somme_temps_executions;
  long somme_tot;
  int nb_executions_tot;
  int nb_executions;
  long first_time;
  gboolean initialiser;
  char message[MESSAGE_MAX];
} stat_group_execution;

typedef struct data_courbe {
  float **values;
  int indice;
  int last_index, old_index;
  int column, line;
  gboolean show;
  GtkWidget *check_box, *check_box_label;
} data_courbe;

typedef struct group_display_save {
  char name[SIZE_NO_NAME];
  float x, y;
  int output_display, display_mode;
  float val_min, val_max;
  int normalized;
} type_group_display_save;

typedef struct type_para_neuro {

  gboolean selected;
  double center_x;
  double center_y;
  gboolean links_ok;
  type_link_draw *links_to_draw;
  type_coeff* coeff;
// int number_of_links_to_draw;

} type_para_neuro;

typedef struct group {
  int id;
  struct script *script;
  char name[SIZE_NO_NAME]; //TODO réduire le 1024... ne sert à rien
  char function[TAILLE_CHAINE];
  int number_of_neurons;
  int rows;
  int columns;
  type_neurone *neurons; //Tableau des neurones du groupe
  type_para_neuro *param_neuro_pandora; //Tableau des parametre supplémentaire associés aux neuronnes.

  int x, y, calculate_x;
  int is_in_a_loop, is_currently_in_a_loop;
  gboolean knownX; //TRUE si la coordonnée x est connue
  gboolean knownY;
  int number_of_links;
  int number_of_links_second;
  struct group **previous; //Adresses des groupes ayant des liaisons vers celui-ci
  struct group **previous_second; //Adresses des groupes ayant des liaisons secondaires vers celui-ci

  int firstNeuron; ///Numéro du premier neurone de ce groupe dans le grand tableau de tous les neurones du script
  int nb_update_since_next;
  float val_min, val_max;
  void *widget, *drawing_area, *label;
  void *entry_val_min, *entry_val_max;
  void *ext, *dialog;
  void *path;
  int output_display, display_mode, previous_display_mode, previous_output_display;
  int normalized, image_selected_index;

  /// enregistrement des valeurs S, S1 et S2, utilisées pour tracer le graphe. [ligne][colonne][s(0), s1(1) ou s2(2)][numValeur]
  float ****tabValues;
  int **indexDernier, **indexAncien;
  GtkWidget *button_vbox;
  int **afficher; // utilisé pour le mode big graph uniquement.
  data_courbe *courbes;
  int number_of_courbes;

  /// variables utilisées pour la fréquence moyenne
  float frequence_values[FREQUENCE_MAX_VALUES_NUMBER];
  int frequence_index_last, frequence_index_older;

  sem_t sem_update_neurons;
  int counter;
  GTimer *timer;
  GThread *thread;

  gboolean selected_for_save;
  gboolean on_saving;
  gboolean ok;
  gboolean ok_display;
  gboolean refresh_freq;
  gboolean from_file;

  FILE* associated_file;

  int idDisplay;
  gboolean is_watch;
  float neurons_width;
  float neurons_height;

  // variables utilisées pour le calcul du taux d'activité de chaque groupe.
  stat_group_execution stats;
  int x_event;
  int y_event;
  struct timespec press_time;
  int neuro_select;
  gboolean image_ready;
  cairo_surface_t *surface_image;
  int stride;
  int type_control;
  float step;
  float init;
  float borne_max;
  float borne_min;
  char name_n[256];
} type_group;

typedef struct type_control{
  type_group* associated_group;
  int type_de_controle;
  GtkWidget* associated_control_widget;
}type_control;



typedef struct script_display_save {
  char name[LOGICAL_NAME_MAX];
  int number_of_groups;
  type_group_display_save *groups;
} type_script_display_save;

typedef struct script {
  int id;
  char name[LOGICAL_NAME_MAX];
  char machine[HOST_NAME_MAX];
  int color, y_offset, height, z;
  int number_of_groups;
  type_group *groups; //Tableau des groupes du script
  int displayed; //TRUE s'il faut afficher le script
  ENetPeer *peer;
  void *widget, *label, *checkbox, *z_spinnner, *search_button, *control_button;
  sem_t sem_groups_defined;
  GtkWidget* pWindow;
  type_control** control_group;
  int* number_of_control;
} type_script;

typedef struct script_link {
  char name[TAILLE_CHAINE];
  type_group *previous;
  type_group *next;
  int type;
} type_script_link;

/* En-t������te de Variables Globales */

extern char bus_id[BUS_ID_MAX];
extern double start_time; /* time of start in seconds */
extern int refresh_mode;
extern pthread_mutex_t mutex_script_caracteristics;

extern int period;
extern char const * const liste_controle_associee[];
extern gboolean load_temporary_save;
extern char preferences_filename[PATH_MAX]; //fichier de preferences (*.jap)
extern int stop; // continue l'enregistrement pour le graphe ou non.
extern gboolean calculate_executions_times;

extern int number_of_scripts; //Nombre de scripts ������ afficher
char scriptsNames[NB_SCRIPTS_MAX][SCRIPT_NAME_MAX]; //Tableau des noms des scripts
extern type_script *scripts[NB_SCRIPTS_MAX]; //Tableau des scripts ������ afficher
extern int newScriptNumber; //Num������ro donn������ ������ un script quand on l'ouvre
extern int zMax; //la plus grande valeur de z parmi les scripts ouverts
extern int buffered; //Nombre d'instantan������s actuellement en m������moire

extern int usedWindows;
extern type_group *selected_group; //Pointeur sur le groupe actuellement s������lectionn������
extern int selectedWindow; //Num������ro de la fen������tre s������lectionn������e (entre 0 et NB_WINDOWS_MAX-1). NB_WINDOWS_MAX indique qu'aucune fen������tre n'est s������lectionn������e

extern GtkWidget *pHBox2; //Panneau des neurones
extern GtkWidget *pFrameNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de petites fen������tres pour les neurones
extern GtkWidget *zoneNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de zones o������ dessiner des neurones
extern type_group *windowGroup[NB_WINDOWS_MAX]; //Adresse des groupe affich������ dans la zoneNeurones de m������me indice
extern int windowValue[NB_WINDOWS_MAX]; //Num������ro disant quelle valeur des neurones du groupe il faut afficher dans la fen������tre de m������me indice (0 : s, 1 : s1, 2 : s2, 3 : pic)

extern int nbColonnesTotal; //Nombre total de colonnes de neurones dans les fen������tres du bandeau du bas
extern int nbLignesMax; //Nombre maximal de lignes de neurones ������ afficher dans l'une des fen������tres du bandeau du bas
extern type_group *groups_to_display[NB_WINDOWS_MAX];
extern int number_of_groups_to_display;

extern type_script_link net_links[SCRIPT_LINKS_MAX];
extern int number_of_net_links;

extern pthread_t enet_thread;

extern pthread_mutex_t mutex_loading;
extern pthread_cond_t cond_loading;
extern pthread_mutex_t mutex_script_caracteristics;

extern char bus_id[];
extern char bus_ip[];

extern GtkWidget *window;
extern GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale;
extern GtkWidget *vpaned, *scrollbars;
extern GtkWidget *neurons_frame, *zone_neurons;
extern gboolean saving_press;
extern gboolean draw_links_info;

extern pthread_t new_window_thread;
extern type_group *open_group;
extern GtkWidget *compress_button;

extern gint format_mode;
//------------------------------------------------PROTOTYPES--------------------------------------------------------

void init_pandora(int argc, char** argv);
//extern void prom_bus_init(const char *ip);
void script_update_positions(type_script *script);
void resize_group(type_group *group);

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
void neurons_frame_drag_group(GtkWidget *pWidget, GdkEvent *event, gpointer pdata);
void on_release(GtkWidget *zone_neurons, GdkEventButton *event, gpointer data);
void on_group_display_show_or_mask_neuron(GtkWidget *pWidget, gpointer pData);
void key_press_event(GtkWidget *pWidget, GdkEventKey *event);
void save_preferences(GtkWidget *pWidget, gpointer pData);
void save_preferences_as(GtkWidget *pWidget, gpointer pData);
void pandora_load_preferences(GtkWidget *pWidget, gpointer pData);
void defaultScale(GtkWidget *pWidget, gpointer pData);
void on_toggled_saving_button(GtkWidget *save_button, gpointer pdata);
void on_toggled_compress_button(GtkWidget *compress_button, gpointer pData);

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
void window_title_update();

//Autres
gboolean neurons_refresh_display();
gfloat niveauDeGris(float val, float valMin, float valMax);
void resizeNeurons();
int get_width_height(int nb_row);
void pandora_file_save(const char *filename);
void pandora_file_load(const char *filename);
void pandora_file_load_script(const char *filename, type_script *script);
gboolean script_caracteristics(type_script *script, int action);

void fatal_error(const char *name_of_file, const char *name_of_function, int numero_of_line, const char *message, ...);
void pandora_bus_send_message(char *id, const char *format, ...);
void server_for_promethes();
float**** createTab4(int nbRows, int nbColumns);
void destroy_tab_4(float **** tab, int nbColumns, int nbRows);
int** createTab2(int nbRows, int nbColumns, int init_value);
void destroy_tab_2(int **tab, int nbRows);

void on_search_group(int index);
gboolean neurons_refresh_display_without_change_values();
//gboolean button_press_neurons(GtkStatusIcon *status_icon, GdkEvent *event, type_group *group);

void zoom_out(GdkDevice *pointer);
void zoom_in(GdkDevice *pointer);
void phases_info_start_or_stop(GtkToggleButton *pWidget, gpointer pData);
void debug_grp_mem_info_start_or_stop(GtkToggleButton *pWidget, gpointer pData);
void on_group_display_clicked(GtkButton *button, type_group *group);
void on_button_draw_links_info_pressed(GtkToggleButton *pWidget, gpointer pData);
void format_combo_box_changed(GtkComboBox *comboBox, gpointer data);
void on_destroy_control_window(GtkWidget *pWidget, gpointer pdata);
void on_toggled_affiche_control_button(GtkWidget *affiche_control_button, gpointer pData);
void search_control_in_script_and_allocate_control(type_script* script_actu);
void create_range_controls(type_script* script_actu,GtkWidget *box1);

#endif
