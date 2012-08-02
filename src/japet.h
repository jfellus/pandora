/* japet.h
 *
 * Auteurs : Brice Errandonea et Manuel De Palma
 * 
 * Pour compiler : make
 * 
 */

#ifndef JAPET_H
#define JAPET_H
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

#include "colors.h"
#include "prom_tools/include/xml_tools.h"
#include "enet/include/enet/enet.h"
#include "prom_kernel/include/japet_connect.h"
#include "prom_kernel/include/reseau.h"


#define JAPET_PORT 1235
#define BROADCAST_IP "127.255.255.255"


//------------------------------------------------LIMITES---------------------------------------------------------------

//Si ces limites se révèlent trop restrictives (ou trop larges), éditer les valeurs de cette rubrique
#define NB_WINDOWS_MAX 30 //Nombre maximal de petites fenêtres affichables dans le bandeau du bas
//#define NB_SCRIPTS_MAX 30      NB_SCRIPTS_MAX est défini dans japet_connect.h
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

#define BUS_ID_MAX 128

//-------------------------------------------------CONSTANTES----------------------------------------------------------

#define MESSAGE_MAX 256 //Utilisé par la fonction fatal_error dans japet.c
#define SHOWCOLOR_SIZE 20
#define GRID_WIDTH 0.2 //Épaisseur des lignes de la grille

#define RECEPTION_DELAY 2 //Délai (en secondes) pendant lequel Japet attend les scripts qu'il a demandé

//Dimension du rectangle représentant un groupe
#define LARGEUR_GROUPE 50
#define HAUTEUR_GROUPE 30

//Échelles par défaut
#define REFRESHSCALE_DEFAULT 5
#define XSCALE_DEFAULT 64
#define YSCALE_DEFAULT 32
#define XGAP_DEFAULT 30
#define YGAP_DEFAULT 20
#define DIGITS_DEFAULT 4 //Nombre de caractères pour afficher les valeurs d'un neurone

//-----------------------------------------------ENUMERATIONS--------------------------------------------------------

enum
{
		NET_LINK_SIMPLE = 0,
		NET_LINK_BLOCK ,
		NET_LINK_ACK,
		NET_LINK_BLOCK_ACK,
};

//------------------------------------------------STRUCTURES---------------------------------------------------------

typedef struct coordonnees
{
	int x;
	int y;
} coordonnees;

typedef struct neuron 
{
    struct group *myGroup;
    float s[4]; //Valeurs du neurone (0 : s, 1 : s1, 2 : s2, 3 : pic)
    int x;
    int y;
    float buffer[4][NB_BUFFERED_MAX];
} neuron;

typedef struct group
{
    struct script *myScript;
    char *name;
    char *function;
    float learningSpeed;
    int nbNeurons;
    int rows;
    int columns;
    neuron *neurons; //Tableau des neurones du groupe
    int x;
    int y;
    gboolean knownX; //TRUE si la coordonnée x est connue
    gboolean knownY;
    int nbLinksTo;
    struct group **previous;	//Adresses des groupes ayant des liaisons vers celui-ci
    int sWindow; //Numéro (entre 0 et NB_WINDOWS_MAX - 1) de la fenêtre où s'affichent les valeurs des neurones de ce groupe (NB_WINDOWS_MAX si pas encore de fenêtre). Il y a 4 valeurs affichables par neurone
    gboolean justRefreshed; //TRUE si le dernier message envoyé par Prométhé a réactualisé ce groupe
    int firstNeuron; ///Numéro du premier neurone de ce groupe dans le grand tableau de tous les neurones du script
    int allocatedNeurons; ///Nombre de neurones déjà rangés
    int nb_update_since_next;
} group;

typedef struct script
{
    char *name;//[SCRIPT_NAME_MAX];
    char *machine;//[IP_LENGTH_MAX];
    int autorize_neurons_update;
    int color, y_offset, height, z;
    int nbGroups;
    group *groups; //Tableau des groupes du script
    gboolean displayed; //TRUE s'il faut afficher le script
    ENetPeer *peer;
} script;

typedef struct script_link
{
	char name[TAILLE_CHAINE];
	group *previous;
	group *next;
	int type;
} type_script_link;

typedef struct ivyServer
{
    char ip[18];
    char appName[30];
} ivyServer;


//---------------------------------------------VARIABLES GLOBALES----------------------------------------------------
 
extern GtkWidget *pWindow; //La fenêtre de l'application Japet
extern GtkWidget *pVBoxScripts; //Panneau des scripts
extern GtkWidget *zone3D; //La grande zone de dessin des liaisons entre groupes
extern GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale, *digitsScale; //Échelles

//Indiquent quel est le mode d'affichage en cours (Off-line, Sampled ou Snapshot)
extern GtkWidget *modeLabel;
extern int currentSnapshot;
extern int nbSnapshots;

extern int Index[NB_SCRIPTS_MAX]; //Tableau des indices : Index[0] = 0, Index[1] = 1, ..., Index[NB_SCRIPTS_MAX-1] = NB_SCRIPTS_MAX-1;
//Ce tableau permet à une fonction signal de retenir la valeur qu'avait i au moment où on a connecté le signal

extern int autorize_neuron_update;

extern GtkWidget **openScripts; //Lignes du panneau des scripts
extern GtkWidget **scriptCheck; //Cases à cocher/décocher pour afficher les scripts ou les masquer
extern GtkWidget **scriptLabel; //Label affichant le nom d'un script dans la bonne couleur
extern GtkWidget **zChooser; //Spin_button pour choisir dans quel plan afficher ce script

extern int nbScripts ; //Nombre de scripts à afficher
char scriptsNames[NB_SCRIPTS_MAX][SCRIPT_NAME_MAX]; //Tableau des noms des scripts
extern script scr[NB_SCRIPTS_MAX]; //Tableau des scripts à afficher
extern int newScriptNumber; //Numéro donné à un script quand on l'ouvre
extern int zMax; //la plus grande valeur de z parmi les scripts ouverts
extern int buffered ; //Nombre d'instantanés actuellement en mémoire

extern int usedWindows ;
extern group *selectedGroup; //Pointeur sur le groupe actuellement sélectionné
extern int selectedWindow ; //Numéro de la fenêtre sélectionnée (entre 0 et NB_WINDOWS_MAX-1). NB_WINDOWS_MAX indique qu'aucune fenêtre n'est sélectionnée

extern GtkWidget *pHBox2; //Panneau des neurones
extern GtkWidget *pFrameNeurones[NB_WINDOWS_MAX];//Tableau de NB_WINDOWS_MAX adresses de petites fenêtres pour les neurones
extern GtkWidget *zoneNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de zones où dessiner des neurones
extern group *windowGroup[NB_WINDOWS_MAX]; //Adresse des groupe affiché dans la zoneNeurones de même indice
extern int windowValue[NB_WINDOWS_MAX]; //Numéro disant quelle valeur des neurones du groupe il faut afficher dans la fenêtre de même indice (0 : s, 1 : s1, 2 : s2, 3 : pic)

extern int nbColonnesTotal; //Nombre total de colonnes de neurones dans les fenêtres du bandeau du bas
extern int nbLignesMax; //Nombre maximal de lignes de neurones à afficher dans l'une des fenêtres du bandeau du bas

//Pour japet_receive_from_prom.c
extern int ivyServerNb;

extern ENetHost *enet_server;

extern type_script_link net_link[SCRIPT_LINKS_MAX];
extern int nb_net_link;
//------------------------------------------------PROTOTYPES--------------------------------------------------------

void init_japet(int argc, char** argv);
void prom_bus_init(const char *ip);
void update_positions(int i);
void update_script_display(int script_id);
void update_neurons_display(int script_id, int neuronGroupId);


//Signaux
void drag_drop_neuron_frame(GtkWidget *zone_neurons, GdkEventButton *event, gpointer data);
void expose_event_zone_neuron(GtkWidget *zone_neurons, gpointer pData);
void cocheDecoche(GtkWidget *pWidget, gpointer pData);
void Close(GtkWidget *pWidget, gpointer pData);
void changePlan(GtkWidget *pWidget, gpointer pData); //Un script change de plan
void changeValue(GtkWidget *pWidget, gpointer pData);
void on_search_group_button_active(GtkWidget *pWidget, script *script);
void on_hide_see_scales_button_active(GtkWidget *hide_see_scales_button, gpointer pData);
void on_check_button_draw_active(GtkWidget *check_button, gpointer data);

void button_press_event (GtkWidget *pWidget, GdkEventButton *event);
void key_press_event (GtkWidget *pWidget, GdkEventKey *event);
void expose_event(GtkWidget *zone3D, gpointer pData);

void japet_save_preferences(GtkWidget *pWidget, gpointer pData);
void japet_load_preferences(GtkWidget *pWidget, gpointer pData);
void defaultScale(GtkWidget *pWidget, gpointer pData);
void askForScripts(GtkWidget *pWidget, gpointer pData);

void expose_neurons(GtkWidget *zone2D, gpointer pData);

//"Constructeurs"
void newScript(script *s, char *name, char *machine, int z, int nbGroups, int id);
void destroyScript(int script_id);
void destroyAllScripts();
void newGroup(group *g, script *myScript, char *name, char *function, float learningSpeed, int nbNeurons, int rows, int columns, int y, int nbLinksTo, int firstNeuron);
void newNeuron(neuron *n, group *mygroup, float s, float s1, float s2, float pic, int x, int y);
//Mise un jour d'un neurone quand Prométhé envoie de nouvelles données
void updateNeuron(neuron *n, float s, float s1, float s2, float pic);
//Mise un jour d'un groupe quand Prométhé envoie de nouvelles données
void updateGroup(group *g, float learningSpeed, float execTime);

//Autres
gboolean refresh_display();
const char* tcolor(script S);
void color(cairo_t *cr, group g);
void clearColor(cairo_t *cr, group g);
void newWindow(group *g, float pos_x, float pos_y);
gfloat niveauDeGris(float val, float valMin, float valMax);
void resizeNeurons();
void saveJapetConfigToFile(char *filename);
void loadJapetConfigToFile(char *filename);

void fatal_error(const char *name_of_file, const char *name_of_function,  int numero_of_line, const char *message, ...);
void japet_bus_send_message(char *id, const char *format, ...);
void server_for_promethes();
#endif
