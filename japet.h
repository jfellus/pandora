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
#include <time.h>
#include <semaphore.h>

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>
#include "../enet/include/enet/enet.h"

#include "colors.h"


#ifdef ETIS
  #include "../prom_kernel/include/japet_connect.h"
  #include "../prom_kernel/include/reseau.h"
#endif

#define JAPET_PORT 1235
#define BROADCAST_IP "127.255.255.255"


//------------------------------------------------LIMITES---------------------------------------------------------------

//Si ces limites se révèlent trop restrictives (ou trop larges), éditer les valeurs de cette rubrique
#define NB_WINDOWS_MAX 30 //Nombre maximal de petites fenêtres affichables dans le bandeau du bas
#define NB_SCRIPTS_MAX 20 //Nombre maximal de scripts qu'on peut détecter (et donc afficher) simultanément
#define NB_ROWS_MAX 4 //Nombre maximal de lignes de petites fenêtres affichant des neurones
#define NB_BUFFERED_MAX 100 //Nombre maximal d'instantanés stockés


#define NB_PLANES_MAX 7
//Le nombre maximal de plans affichables simultanément dans la zone 3D est limité à 7. Cette limite là est un peu plus
//compliquée à modifier car il faudrait définir de nouvelles couleurs pour chaque valeur de z > 6. 
//De toute façon, 8 plans superposés seraient difficilement lisibles (7, déjà...).




//-------------------------------------------------CONSTANTES----------------------------------------------------------


#define MESSAGE_MAX 256 //Utilisé par la fonction fatal_error dans japet.c
#define SHOWCOLOR_SIZE 20
#define GRID_WIDTH 0.3 //Épaisseur des lignes de la grille

#define RECEPTION_DELAY 5 //Délai (en secondes) pendant lequel Japet attend les scripts qu'il a demandé

//Dimension du rectangle représentant un groupe
#define LARGEUR_GROUPE 50
#define HAUTEUR_GROUPE 30

//Échelles par défaut
#define REFRESHSCALE_DEFAULT 5
#define XSCALE_DEFAULT 150
#define YSCALE_DEFAULT 150
#define XGAP_DEFAULT 30
#define YGAP_DEFAULT 20
#define NEURON_HEIGHT_MIN_DEFAULT 30 //Hauteur minimale d'un neurone dans les fenêtres du bas
#define DIGITS_DEFAULT 8 //Nombre de caractères pour afficher les valeurs d'un neurone


//------------------------------------------------STRUCTURES---------------------------------------------------------


typedef struct neuron 
{
    struct group* myGroup;
    gfloat s[4]; //Valeurs du neurone (0 : s, 1 : s1, 2 : s2, 3 : pic)    
    gint x;
    gint y;
    gfloat buffer[4][NB_BUFFERED_MAX];    
} neuron;

typedef struct group
{
    struct script* myScript;
    gchar* name;
    gint function;
    gfloat learningSpeed;    
    gint nbNeurons;
    gint rows;
    gint columns;
    neuron* neurons; //Tableau des neurones du groupe
    gint x;
    gint y;
    gboolean knownX; //TRUE si la coordonnée x est connue
    gboolean knownY;
    gint nbLinksTo;    
    struct group** previous;	//Adresses des groupes ayant des liaisons vers celui-ci
    gint sWindow[4]; //Numéro (entre 0 et NB_WINDOWS_MAX - 1) de la fenêtre où s'affichent les valeurs des neurones de ce groupe (NB_WINDOWS_MAX si pas encore de fenêtre). Il y a 4 valeurs affichables par neurone  
    gboolean justRefreshed; //TRUE si le dernier message envoyé par Prométhé a réactualisé ce groupe
} group;

typedef struct script
{
    gchar* name;
    gchar* depiction;
    gchar* machine;
    gint z;
    gint nbGroups;
    group* groups; //Tableau des groupes du script
    gboolean displayed; //TRUE s'il faut afficher le script
} script;


typedef struct ivyServer
{
    char ip[18];
    char appName[30];
} ivyServer;


//---------------------------------------------VARIABLES GLOBALES----------------------------------------------------
 
extern GtkWidget* pWindow; //La fenêtre de l'application Japet
extern GtkWidget* pVBoxScripts; //Panneau des scripts
extern GtkWidget* zone3D; //La grande zone de dessin des liaisons entre groupes
extern GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale, *neuronHeightScale, *digitsScale; //Échelles

//Indiquent quel est le mode d'affichage en cours (Off-line, Sampled ou Snapshot)
extern gchar* displayMode;
extern GtkWidget* modeLabel; 

extern gint Index[NB_SCRIPTS_MAX]; //Tableau des indices : Index[0] = 0, Index[1] = 1, ..., Index[NB_SCRIPTS_MAX-1] = NB_SCRIPTS_MAX-1;
//Ce tableau permet à une fonction signal de retenir la valeur qu'avait i au moment où on a connecté le signal

extern GtkWidget** openScripts; //Lignes du panneau des scripts
extern GtkWidget** scriptCheck; //Cases à cocher/décocher pour afficher les scripts ou les masquer
extern GtkWidget** scriptLabel; //Label affichant le nom d'un script dans la bonne couleur
extern GtkWidget** zChooser; //Spin_button pour choisir dans quel plan afficher ce script

extern gint nbScripts ; //Nombre de scripts à afficher
extern script* scr; //Tableau des scripts à afficher
extern gint newScriptNumber; //Numéro donné à un script quand on l'ouvre
extern gint zMax; //la plus grande valeur de z parmi les scripts ouverts
extern gint buffered ; //Nombre d'instantanés actuellement en mémoire

extern gint usedWindows ;
extern group* selectedGroup; //Pointeur sur le groupe actuellement sélectionné
extern gint selectedWindow ; //Numéro de la fenêtre sélectionnée (entre 0 et NB_WINDOWS_MAX-1). NB_WINDOWS_MAX indique qu'aucune fenêtre n'est sélectionnée

extern GtkWidget* pHBox2; //Panneau des neurones
extern GtkWidget* pFrameNeurones[NB_WINDOWS_MAX];//Tableau de NB_WINDOWS_MAX adresses de petites fenêtres pour les neurones
extern GtkWidget* zoneNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de zones où dessiner des neurones
extern group* windowGroup[NB_WINDOWS_MAX]; //Adresse des groupe affiché dans la zoneNeurones de même indice
extern gint windowValue[NB_WINDOWS_MAX]; //Numéro disant quelle valeur des neurones du groupe il faut afficher dans la fenêtre de même indice (0 : s, 1 : s1, 2 : s2, 3 : pic)
extern gint windowPosition[NB_WINDOWS_MAX]; //Position de la fenêtre de même indice dans le bandeau du bas (0 : la plus à gauche)

extern gint nbColonnesTotal; //Nombre total de colonnes de neurones dans les fenêtres du bandeau du bas
extern gint nbLignesMax; //Nombre maximal de lignes de neurones à afficher dans l'une des fenêtres du bandeau du bas

//Pour japet_receive_from_prom.c
extern int ivyServerNb;

//int nbre_groupe;
extern ENetHost* enet_server;

//Sémaphores
extern sem_t sem_script;



//------------------------------------------------PROTOTYPES--------------------------------------------------------

void init_japet(int argc, char** argv);
void prom_bus_init( char *ip); 


//Signaux
void cocheDecoche(GtkWidget* pWidget, gpointer pData);
void Close(GtkWidget* pWidget, gpointer pData); 
void changePlan(GtkWidget* pWidget, gpointer pData); //Un script change de plan
void changeValue(GtkWidget* pWidget, gpointer pData);

void button_press_event (GtkWidget* pWidget, GdkEventButton* event);
void key_press_event (GtkWidget* pWidget, GdkEventKey *event);
void expose_event(GtkWidget* zone3D, gpointer pData);

void saveScale(GtkWidget* pWidget, gpointer pData);
void loadScale(GtkWidget* pWidget, gpointer pData);
void defaultScale(GtkWidget* pWidget, gpointer pData);
void askForScripts(GtkWidget* pWidget, gpointer pData);

void expose_neurons(GtkWidget* zone2D, gpointer pData);
void button_press_neurons(GtkWidget* pFrame, GdkEventButton* event);

//"Constructeurs"
void newScript(script* s, gchar* name, gchar* machine, gint z, gint nbGroups);
void destroyScript(script* s);
void destroyAllScripts();
void newGroup(group* g, script* myScript, gchar* name, gint function, gfloat learningSpeed, gint nbNeurons, gint rows, gint columns, gint y, gint nbLinksTo);
void newNeuron(neuron* n, group* mygroup, gfloat s, gfloat s1, gfloat s2, gfloat pic, gint x, gint y);
//Mise un jour d'un neurone quand Prométhé envoie de nouvelles données
void updateNeuron(neuron* n, gfloat s, gfloat s1, gfloat s2, gfloat pic);
//Mise un jour d'un groupe quand Prométhé envoie de nouvelles données
void updateGroup(group* g, gfloat learningSpeed, gfloat execTime);

//Autres
gboolean refresh_display();
gchar* tcolor(script S);
void color(cairo_t* cr, group g);
void clearColor(cairo_t* cr, group g);
void dessinGroupe (cairo_t* cr, gint a, gint b, gint c, gint d, group* g, gint z, gint zMax);
void findX(group* g);
void newWindow(group* g);
gfloat niveauDeGris(gfloat val, gfloat valMin, gfloat valMax);
void resizeNeurons();
void saveJapetConfigToFile(char* filename);
void loadJapetConfigToFile(char* filename);

void fatal_error(const char *name_of_file, const char* name_of_function,  int numero_of_line, const char *message, ...);
void japet_bus_send_message(const char *format, ...);
#endif
