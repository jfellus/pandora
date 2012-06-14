#ifndef _RESEAU
#define _RESEAU
/*---------------------------------------------------------------*/
/*      type d'un reseau general pour le robot simule            */
/*      version sept 1992 : rajout du temps de memorisation      */
/*      precedente version dans dir_system_a                     */
/*---------------------------------------------------------------*/

/**
\defgroup reseau reseau.h
\ingroup generic_tools

\brief types pour reseau general

\details

inclut les differents types utilises dans promethe

definit la plupart des constantes


\file
\ingroup reseau

*/




#include <semaphore.h>
#include <math.h>
#include "bend.h"

#define RESEAU_VERSION  9

#define nil 0 /* TODO: le supprimer et remplacer partout par NULL */

#define modulo 2147483646

#define No_Hebb              0
#define No_Winner            1
#define No_Kohonen           2
#define No_Widrow            3
#define No_Special           4
#define No_Ou                5
#define No_Et                6
#define No_Entree            7
#define No_Sortie            8
#define No_PTM               9
#define No_Hebb_Seuil       10
#define No_Winner_Special   11
#define No_Hopfield         12
#define No_Winner_Colonne   13
#define No_Fonction_Algo    14
#define No_But              15

/*-------------------------*/

#define No_Pyramidal        16  /* can be a good model of granular cell     */
#define No_Inter_neurone    17  /* no STM, usefull for inhibition mechanism */
/*-------------------------*/
#define No_Fonction_Algo_mvt 18
/*-------------------------*/
#define No_Granular         19
#define No_Micro_Pyramid    20
/*-------------------------*/
#define No_SV_cor 21            /* to correlate (difference) 2 forms */
#define No_Dyn 22
/*-------------------------*/
#define No_SAW              23
#define No_Sigma_PI	     24
#define No_Winner_Selectif	25
#define No_Macro_Colonne 26     /* defined to allow self organization beween neurons of a given macro column */
#define No_BREAK	27
#define No_LMS		  28        /* Group added by O.Ledoux */
#define No_RTTOKEN	  29        /* Group added by A.Patard: Groupe d'execution temps reel. */
#define No_KO_Discret     30    /* ajout M. Quoy 1/03/2005 */
#define No_KO_Continu     31    /* ajout M. Quoy 1/03/2005 */
#define No_Hebb_Seuil_binaire       32
#define No_Pyramidal_plan 33
#define No_LMS_2	34          /* ajoute par B. Siri le 07/07/05 */
#define No_BCM		35          /* ajoute par B. Siri le 20/05/05 */
#define No_PLG 36               /*ajoute par M.M. mais le groupe est pas encore vraiment fonctionnel... */
#define No_Mean_Mvt 37
#define No_Asso_Cond 38
#define No_Sub_Network 39       /* pour gerer l'inclusion s'un script */
#define No_LMS_delayed 40
#define No_Winner_Macro 41
#define No_NLMS 42
#define No_Sutton_Barto 43
#define No_PCR 44
/*#define No_selective_winner_modulated 45
 #define No_PTM_2_old 46*/
#define No_Selverston 45
#define No_RCO 46
#define nbre_type_groupes 47
/*****************************/

#define No_l_1_1_modif     0
#define No_l_1_a           1
#define No_l_1_v           2
#define No_l_1_1_non_modif 3
#define No_l_1_t           4
#define No_l_algorithmique 5
#define No_l_1_1_non_modif_bloqueur 6
#define No_l_1_v_non_modif 7
#define No_l_neuro_mod     8

#define nbre_type_links 9

/*****************************/

#define E0  -0.1
#define E2   0.1
#define EPS 0.0001
#define epsilon 0.00001

#ifndef pi
#define pi 3.14159
#endif

#define Max_voie 5              /* nbre max de voies de liaisons pour 1 neurone */
#define PAS_RT_TOKEN -100

/******************************/
#include "mode_link_def.h"


/*---------------------------------------------------------------*/
/*                  TYPES DE DONNEES UTILISEES                   */
/*---------------------------------------------------------------*/

/* description d'un neurone                                      */

typedef struct type_coeff {
/** valeur du coefficient               */
  float val;
/** probabilite du poids                */
  float proba;
  /** pour credit assignement temporel    */
  float Nbre_ES;
/** pour Arnaud nombre de fois ou une sortie a ete active */
  float Nbre_S;
/** nb de fois ou un entree a ete activee  */
  float Nbre_E;
/** numero de l'entree associee         */
  int entree;
/** modifiable ou non
  - 2n   :  ei*wij    non modifiable
  - 2n+1 : |ei - wij |    modifiable
  - n numero de la voie                 */
  int type;
/** si le poids peut evoluer ou non
  si non modif alors evolution=0      */
  int evolution;
/** valeur moyenne de l'entree          */
  float moy;
/** valeur integree de la sortie        */
  float smoy;
  int gpe_liaison;
/** pointe vers le coefficient suivant  */
  struct type_coeff *s;
} type_coeff;

typedef struct type_neurone {
/** seuil du neurone                    */
  float seuil;
/** sortie non seuillee                 */
  float s;
/** sortie temps t1                     */
	float s1;
/** sortie temps t2                     */
	float s2;
/** utilise pour noter les neurones a
 modifier avec PTM
 utilise par les neurones bloqueurs
 pour donner l'adresse du macro N    */
	int flag;
/** retropagation de l'erreur
 ou tolerance pour le seuil
 ou activite de la voie inconditionnelle */
	float d;
/** instant de la derniere activation du neurone */
	float last_activation;
/** coeff correspondant au seuil        */
	float cste;
/** numero du groupe d'appartenance     */
	int groupe;
/** nbre de voies de liaisons ratachees */
	int nbre_voie;
/** nombre de coeffs rataches           */
	int nbre_coeff;
/** indique si le neurone est le max de
 son groupe/ supporter max <1
 pour Stephane                       */
	char max;

/** position du neurone dans l'espace   */
	float posx;
	float posy;
	float posz;
/** pointeur vers liste des coefficients */
	type_coeff *coeff;
/** liste des neurones de sortie        */
	type_coeff *nsor;
} type_neurone;

/** description de la structure du reseau                         */

typedef struct type_noeud {
/** numero du neurone associe           */
	int numero;
/** pointe  neurone suivant du groupe   */
	struct type_noeud *s;
} type_noeud;

#define TAILLE_CHAINE 256

typedef struct type_noeud_comment {
	char chaine[TAILLE_CHAINE];
	struct type_noeud_comment *suiv;
} type_noeud_comment;

typedef struct type_prom_data {
	long type;
	long size;
	struct type_prom_data *next;
	char prom_data_element[1];
} type_prom_data;

typedef struct arg_thread_grp {
	int gpe;
	int i;
	int gestion_STM;
	int learn;
	int no_thread;
	int retour;
	int mvt;
/** semaphore compteur s'assurant que les threads sont bien crees avant de poursuivre*/
	sem_t * p_is_launched;
} arg_thread_grp;

#ifndef _LIMBIC_NEUROMOD_structs_noeudmod_listemod_H
#define _LIMBIC_NEUROMOD_structs_noeudmod_listemod_H

/** Liste des modulations du meme type arrivant sur le groupe */

typedef struct st_noeud_modulation {
/** Poids du lien de neuromodulation */
	float poids;
/** Numero du neurone d'entree effectuant la neuromodulation */
	int num_neurone;
	struct st_noeud_modulation *suivant;
} noeud_modulation;

/** Liste de toutes les neuromodulations affectees a un groupe */

typedef struct st_liste_neuromodulations {
/** Type de la neuromodulation en chaine de caracteres           */
	char *type;
/** Liste des modulations du meme type arrivant sur le groupe    */
	noeud_modulation *liste_modulation;
	struct st_liste_neuromodulations *suivant;
} liste_types_neuromodulations;

#endif

#define MIN_FLOAT_EPS 1e-30
/** pour les tests d'equalite entre 2  flottants */
static inline int isequal(float x, float y)
{
	return (fabs(x - y) < 1e-30);
}

/** pour les tests de difference entre 2  flottants */
static inline int isdiff(float x, float y)
{
	return (fabs(x - y) > 1e-30);
}

/*--------------------------------------------------------------*/
/*         structure des donnees pour le script                 */
/* pour la creation de reseaux avec cluster                     */
/* systeme interactif de creation de reseaux                    */
/*--------------------------------------------------------------*/

/** defini dans le parser outils_script.c */
extern const char *stop_if_not_int(const char *chaine);
/** defini dans le parser outils_script.c */
extern const char *stop_if_not_float(const char *chaine);

/* MY_FloatAffect(x,y) : affecte un flottant y a la variable x (float ou string) */
/* MY_IntAffect(x,y):  affecte un entier y a la variable x (float ou string) */
/* MY_Str2Float(x,y): convertit la string y en float x (ou string x) */

#ifndef SYMBOLIQUE_VERSION
#define my_int(x)   int x
#define my_d       %d
#define my_float(x) float x
#define my_f       %f
#define my_adr     &
#define MY_Int2Int(x) x
#define MY_Float2Float(x) x
/** attention fonction dangereuse , pas d'allocation memoire*/
#define MY_Data_Copy(x,y) x=y
#define MY_Float2Str(x) Float2Str(x)
#define MY_Int2Str(x) Int2Str(x)
/*----------*/
#define Str2MY_Float(x,y) x=atof(stop_if_not_float(y))
#define Str2MY_Int(x,y) x=atoi(stop_if_not_int(y))
/** affecte un float y au float x */
#define MY_FloatAffect(x,y) x=y
#define MY_IntAffect(x,y) x=y
#else
#define my_int(x)   char x[TAILLE_CHAINE]
#define my_d       #%s
#define my_float(x) char x[TAILLE_CHAINE]
#define my_f       #%s
#define my_adr                  /* rien */
#define MY_Int2Int(x) atoi(x)
#define MY_Float2Float(x) atof(x)
#define MY_Data_Copy(x,y) strcpy(x,y)
#define MY_Float2Str(x) x
#define MY_Int2Str(x) x
#define Str2MY_Float(x,y) strcpy(x,y)
#define Str2MY_Int(x,y) strcpy(x,y)
#define MY_FloatAffect(x,y) strcpy(x,Float2Str(y))
#define MY_IntAffect(x,y) strcpy(x,Int2Str(y))
#endif

#define AFF1(chaine,type)  chaine #type
#define AFF(chaine,type) AFF1(chaine,type)

#define taille_max_tableau_diff_gaussienne 500 /* 30 */

#define SIZE_NO_NAME 1024

/* penser a faire le lien symbolique vers ce fichier aussi */
/*ln -s ../../prom_kernel/prom_kernel/include/type_groupe_common_struct . */
typedef struct type_com_groupe {
#include "type_groupe_common_struct.h"
} type_com_groupe;

typedef struct type_groupe {

#include "type_groupe_common_struct.h"

/**  ces champs ont le meme objectif que ext mais ils sont specialises dans le traitement de */
	type_prom_data *video_ext;
/** donnees particulieres. Nous avons identifie des donnees de type video (image et sequence d'image)    */
	type_prom_data *tactil_ext;
/**  sonores et tactiles. Type_prom_data est un liste chainee ou chaque maillon contient une donnee particuliere,
   son type, sa taille, et le maillon suivant. */
	type_prom_data *audio_ext;

/** pour permettre de rajouter des trucs particuliers a un groupe */
  /** ce pointeur est initialiser a NULL au debut de l'execution de promethe */
	void *ext;
/** pour stocker les donnees que l'on ne veut pas demander plusieurs fois ... */
  /** ce pointeur est initialiser a NULL au debut de l'execution de promethe */
	void *data;

/** pointeur vers la liste des neuromodulation associees au groupe. */
  /** Ce pointer est initialise au lancement de promethe lors de
      l'initialisation des groupes.                                   */
	liste_types_neuromodulations *liste_types_neuromodulations;
/** liste des commentaires associee a un groupe      */
	type_noeud_comment *comment;
	void (*appel_apprend)(int gpe);
	void (*appel_activite)(int no_neurone, int local_gestion_STM, int local_learn);
	void (*appel_gestion)(int gpe);
	void (*appel_algo)(int gpe);

/** pour la creation et la destruction de structures dediees a un groupe au debut et a la fin d'une simulation */
	void (*init)(void);
	void (*free)(void);

/** gestion heritage de fonctionnalites */

	void (*inherit)(struct type_groupe * this, struct type_groupe * parent);

/* pour la gestion du thread associe a un groupe */

/** pas encore utilise ... */
	void *(*create_and_manage)(void);
/** fonctions appelee a la fin de create_and_manage() */
	void (*new)(int gpe);
	void (*destroy)(int gpe);
/** fonction utilisee lors de la sauvegarde du reseau */
	void (*save)(int gpe);

	pthread_t pthread;

/** semaphore pour eviter de modifier les champs du groupes tant que tout n'est pas fini*/
  /** important pour les rt_tokens */
	sem_t sem_lock_fields;

/** semaphore utilise pour communiquer avec le groupe */
	sem_t sem_wake_up;
/** quand le groupe a fini le traitement lie a wake_up */
	sem_t sem_ack;
/** zone utilisee pour passer un message */
	char message[TAILLE_CHAINE];
	/*int return_value;  *//* valeur de retour lors de l'utilisation du gpe: */
	/*  0 si le  groupe n'a pu s'executer, 1 si OK et 2 si un break intervient  */

	struct type_groupe *s;

	void (*appel_maj_STM_entrees)(int gpe);
	void (*appel_maj_STM_sorties)(int gpe);

	void *data_of_profiling;
} type_groupe;

typedef struct type_liaison {
/** sert a savoir pour leto si le lien a ete lu d'un script insere (=1 si insere) (=0 en direct) */
	int deja_active;
/** secondaire =1 pas necessaire pour jetons circulant */
	/* secondaire =0 indispensable jeton present en entree */
	int secondaire;
/**        type de la liaison                          */
	int type;
/** pour gerer le type de voie                         */
  /** pour gerer le type de voie
   - si mode = 1 --> distance
   - si mode = 0 --> produit
   - si mode = 2 --> distance vers buts
   - si mode = 3 --> produit vers buts
   - (pas a jour: voir neuromod locale) */
	int mode;
/* extremites de la liaison                           */
	int posx1;
	int posy1;
	int posx2;
	int posy2;

/** type des coudes du lien 0: segments, 1: courbe      */
	int style;

	t_polyline_list *polyline_list;

/** valeur initiale ou intervale coeff                 */
	my_float(norme);
/**         groupe de depart                           */
	int depart;
	char depart_name[SIZE_NO_NAME];
/**         groupe d'arrivee                           */
	int arrivee;
	char arrivee_name[SIZE_NO_NAME];
/** temps de mem. pour ces liaisons en entree          */
	my_float(temps);
/** temps de mem. pour ces liaisons en sortie          */
	my_float(stemps);
/**  nombre d'entrees pour un neurone                  */
	my_int(nbre);
/** distances pour liaisons vers voisinage             */
	my_float(dv_x);
/** distances pour liaisons vers voisinage             */
	my_float(dv_y);
/** probabilite de creation de la liaison              */
	my_float(proba);
/** nom de la liaison pour traitements algo            */
	char nom[TAILLE_CHAINE];
/** liste des commentaires associee a une liaison       */
	type_noeud_comment *comment;
	struct type_liaison *s;
} type_liaison;

/** tableau contenant le reseau       */
typedef type_neurone *type_tableau;
/** tableau contenant les voies de liaison       */
typedef type_liaison *type_tableau_voies;
/** tableau contenant les entrees     */
typedef float **type_matrice;
/** pour vecteur d'entree ou sortie   */
typedef float *type_vecteur;
typedef int *type_vecteur_entier;

typedef type_noeud *type_tableau_groupe;
typedef type_noeud *pointeur_type_noeud;

#define EPSILON 1.e-10
#define FABS(x) ((x>0)?x:-x)
#define FLOAT_NEAR(x,y) (FABS(x-y)<EPSILON)

extern void Str2Int(int *x, char *chaine);

extern void Str2Float(float *x, char *chaine);

/**************************************************************/
/* transforme un entier en chaine de caracteres */
/* attention si l'on ne recopie pas le resultat renvoye par un 1er
 appel avant le 2eme appel il sera remplace par le resultat du 2eme appel!
 Une seule zone de memoire allouee... */
/* fonctions definies dans Tx_graphic.c ou cc_leto.c */
extern char *Int2Str(int entier);
extern char *Float2Str(float x);

#endif
