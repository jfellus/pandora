/** pandora.h
 *
 * Auteurs : Brice Errandonea et Manuel De Palma
 *
 * Pour compiler : make
 *
 **/

#ifndef PANDORA_H
#define PANDORA_H
#define ETIS	//À commenter quand on n'est pas à l'ETIS
//------------------------------------------------BIBLIOTHEQUES------------------------------------------------------

#include "common.h"
#include "pandora_ivy.h"

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
#define XSCALE_MIN 10
#define XSCALE_MAX 350
#define YSCALE_DEFAULT 58
#define YSCALE_MIN 10
#define YSCALE_MAX 350
#define XGAP_DEFAULT 32
#define YGAP_DEFAULT 24
#define DIGITS_DEFAULT 4 //Nombre de caractères pour afficher les valeurs d'un neurone
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
  REFRESH_MODE_AUTO = 0, REFRESH_MODE_SEMI_AUTO, REFRESH_MODE_MANUAL
};

enum {
  NET_LINK_SIMPLE = 0, NET_LINK_BLOCK = 1 << 0, NET_LINK_ACK = 1 << 1,
};

enum {
  DISPLAY_MODE_SQUARE = 0, DISPLAY_MODE_INTENSITY, DISPLAY_MODE_BAR_GRAPH, DISPLAY_MODE_TEXT, DISPLAY_MODE_GRAPH, DISPLAY_MODE_BIG_GRAPH
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
} type_group;

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
  void *widget, *label, *checkbox, *z_spinnner, *search_button;
  sem_t sem_groups_defined;
} type_script;

typedef struct script_link {
  char name[TAILLE_CHAINE];
  type_group *previous;
  type_group *next;
  int type;
} type_script_link;

/* En-tête de Variables Globales */

extern char bus_id[BUS_ID_MAX];
extern double start_time; /* time of start in seconds */
extern int refresh_mode;
extern pthread_mutex_t mutex_script_caracteristics;

extern int period;
extern gboolean load_temporary_save;
extern char preferences_filename[PATH_MAX]; //fichier de préférences (*.jap)
extern int stop; // continue l'enregistrement pour le graphe ou non.
extern gboolean calculate_executions_times;

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
gboolean neurons_display_refresh_when_semi_automatic();
//gboolean button_press_neurons(GtkStatusIcon *status_icon, GdkEvent *event, type_group *group);

void zoom_out(GdkDevice *pointer);
void zoom_in(GdkDevice *pointer);
void phases_info_start_or_stop(GtkToggleButton *pWidget, gpointer pData);
void on_group_display_clicked(GtkButton *button, type_group *group);
void on_button_draw_links_info_pressed(GtkToggleButton *pWidget, gpointer pData);
#endif
