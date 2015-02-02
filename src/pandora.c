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
/** pandora.c

 *
 *
 * Auteurs : Brice Errandonea, Manuel De Palma
 * Modifié par Arnaud Blanchard et Nils Beaussé. Voir Nils pour le "service après-vente"
 * Pour compiler : make
 *
 *
 **/
#include "float.h"
#include "pandora.h"
#include "pandora_ivy.h"
#include "pandora_graphic.h"
#include "pandora_save.h"
#include "pandora_prompt.h"
#include "pandora_file_save.h"
#include "pandora_architecture.h"
#include "pandora_receive_from_prom.h"

/* Variables globales */
/* TODO : Beaucoups de ces variables globales peuvent etre mise en local en utilisant les astuces adequates, certaine au moins en statiques.
 * D'autre peuvent etre encapsule dans des structures et theme, ce qui simplifierais la lecture et l'utilisation des mutex.
 */
char label_text[LABEL_MAX];

GtkWidget *window = NULL; //La fenetre de l'application Pandora
GtkWidget *selected_group_dialog, *selected_image_combo_box;
GtkWidget *vpaned, *scrollbars;
GtkWidget *hide_see_scales_button = NULL; //Boutton permettant de cacher le menu de droite
GtkWidget *check_button_draw_connections, *check_button_draw_net_connections;
GtkWidget *pVBoxScripts = NULL; //Panneau des scripts
GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale; //echelles
GtkBuilder *builder = NULL;
GtkWidget *compress_button;
GtkWidget *big_graph_frame = NULL, *image_hbox, *format_combobox;

/*Indiquent quel est le mode d'affichage en cours (Off-line, Sampled ou Snapshots)*/
const char *displayMode = NULL;
double start_time;

GtkWidget *modeLabel = NULL;
int currentSnapshot = 0;
int nbSnapshots = 0;

int Index[NB_SCRIPTS_MAX]; /*Tableau des indices : Index[0] = 0, Index[1] = 1, ..., Index[NB_SCRIPTS_MAX-1] = NB_SCRIPTS_MAX-1;
 Ce tableau permet à une fonction signal de retenir la valeur qu'avait i au moment oe on a connecte le signal*/

int number_of_scripts = 0; //Nombre de scripts à afficher
type_script *scripts[NB_SCRIPTS_MAX]; //Tableau des scripts à afficher
int zMax; //la plus grande valeur de z parmi les scripts ouverts
int buffered = 0; //Nombre d'instantanes actuellement en memoire
int period = 0;

type_group *selected_group = NULL; //Pointeur sur le groupe actuellement selectionne

GtkWidget *neurons_frame, *zone_neurons; //Panneau des neurones

int nbColonnesTotal = 0; //Nombre total de colonnes de neurones dans les fenetres du bandeau du bas
int nbLignesMax = 0; //Nombre maximal de lignes de neurones à afficher dans l'une des fenetres du bandeau du bas

gdouble value_hz = 50;
type_group *open_group = NULL;

guint refresh_timer_id = 0; //id du timer actualement utiliser pour le rafraichissement des frame_neurons ouvertes

type_script_link net_links[SCRIPT_LINKS_MAX];
int number_of_net_links = 0;
type_group *groups_to_display[NB_WINDOWS_MAX];
int number_of_groups_to_display = 0;

//type_group *groups_to_survey[NB_SCRIPTS_MAX*NB_GROUPS_MAX];

char bus_id[BUS_ID_MAX];
char bus_ip[HOST_NAME_MAX];

gboolean calculate_executions_times = FALSE;

gboolean load_temporary_save;
char preferences_filename[PATH_MAX]; //fichier de preferences (*.jap)
int stop; // continue l'enregistrement pour le graphe ou non.

//Pour la sauvegarde
gboolean saving_press = 0;
char path_named[MAX_LENGHT_PATHNAME] = "";
char python_path[MAX_LENGHT_PATHNAME] = "~/bin_leto_prom/simulator/japet/save/default_script.py";
char matlab_path[MAX_LENGHT_PATHNAME] = "~/bin_leto_prom/simulator/japet/save/convert_matlab.py";
GtkListStore* currently_saving_list = NULL;

pthread_t new_window_thread;
pthread_mutex_t mutex_loading = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_loading = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_script_caracteristics = PTHREAD_MUTEX_INITIALIZER;
gboolean draw_links_info = FALSE;
sem_t enet_pandora_lock;
gint format_mode = 0;

extern GtkTextBuffer * p_buf;
extern prompt_lign prompt_buf[NB_LIGN_MAX];
int refresh_mode = 0;
char const * const liste_controle_associee[] =
  { "f_checkbox", "f_vue_metres" };

/** Fonctions diverses **/
//TODO : Travaux à continuer : Certaines de ces fonctions peuvent etre deplace dans des fichiers deja cree et plus adaptés.
float**** createTab4(int nbRows, int nbColumns)
{
  float **** tab = malloc(nbRows * sizeof(float ***));
  int i, j, k, l;
  for (i = 0; i < nbRows; i++)
  {
    tab[i] = malloc(nbColumns * sizeof(float **));
    for (j = 0; j < nbColumns; j++)
    {
      tab[i][j] = malloc(3 * sizeof(float *));
      for (k = 0; k < 3; k++)
      {
        tab[i][j][k] = malloc(NB_Max_VALEURS_ENREGISTREES * sizeof(float));
        for (l = 0; l < NB_Max_VALEURS_ENREGISTREES; l++)
          tab[i][j][k][l] = 0.0; // initialisation du tableau multi-dimensionnel.
      }
    }
  }
  return tab;
}

void destroy_tab_4(float **** tab, int nbColumns, int nbRows)
{
  int i, j, k;
  for (i = 0; i < nbRows; i++)
  {
    for (j = 0; j < nbColumns; j++)
    {
      for (k = 0; k < 3; k++)
        free(tab[i][j][k]);

      free(tab[i][j]);
    }
    free(tab[i]);
  }
  free(tab);
}

int** createTab2(int nbRows, int nbColumns, int init_value)
{
  int **tab = malloc(nbRows * sizeof(int *));
  int i, j;
  for (i = 0; i < nbRows; i++)
  {
    tab[i] = malloc(nbColumns * sizeof(int));
    for (j = 0; j < nbColumns; j++)
      tab[i][j] = init_value;
  }
  return tab;
}

void destroy_tab_2(int **tab, int nbRows)
{
  int i;
  for (i = 0; i < nbRows; i++)
  {
    free(tab[i]);
  }
  free(tab);
}

void pandora_quit()
{
  printf("Bye Bye !\n");
  pandora_file_save("./pandora.pandora"); //enregistrement de l'etat actuel. cet etat sera applique au prochain demarrage de pandora.
  pandora_bus_send_message(bus_id, "pandora(%d,0)", PANDORA_STOP);
  enet_deinitialize();
  destroy_saving_ref(scripts); //par securite pour la cloture des fichiers de sauvegarde si on quitte durant la sauvegarde.
  if (currently_saving_list != NULL) g_free(currently_saving_list);
  gtk_widget_destroy(window);
  pthread_cancel(enet_thread);
  gtk_main_quit();
}

void on_signal_interupt(int signal)
{
  switch (signal)
  {
  case SIGINT:
    printf("Bye ! \n");
    exit(EXIT_SUCCESS);
    break;
  case SIGSEGV:
    printf("SEGFAULT ;-) \n");
    exit(EXIT_FAILURE);
    break;
  default:
    printf("signal %d capture par le gestionnaire mais non traite... revoir le gestinnaire ou le remplissage de sigaction\n", signal);
    break;
  }
}

// retour la largeur (resp. la hauteur) de la zone d'affichage d'un groupe en fonction de son nombre de colonnes (resp. de lignes).
int get_width_height(int nb_row)
{

  /*int max;

   if (nb_row >= nb_column)
   {
   max=nb_row;
   }
   else
   {
   max=nb_column;
   }
   */
  if (nb_row == 0) return 1;
  if (nb_row == 1) return 100;
  else if (nb_row <= 16) return 300;
  else if (nb_row <= 128) return 400;
  else if (nb_row <= 256) return 700;
  else return 1000;
}

/**
 *
 * Calcule la coordonnee x d'un groupe en fonction de ses predecesseurs
 *
 */
void findX(type_group *group)
{
  int i;

  if (group->calculate_x == TRUE)
  {
    printf("\n Warning     une boucle !!!\n");
    return;
  }
  group->calculate_x = TRUE;
  if (group->previous == NULL)
  {
    group->x = 1;
  }
  else
  {
    for (i = 0; i < group->number_of_links; i++)
    {
      if (group->previous[i]->knownX == FALSE) findX(group->previous[i]);
      if (group->previous[i]->x >= group->x) group->x = group->previous[i]->x + 1;
    }

    for (i = 0; i < number_of_net_links; i++)
    {
      if (net_links[i].next == group && net_links[i].previous != NULL)
      {
        if (net_links[i].type & NET_LINK_BLOCK)
        {
          if (!net_links[i].previous->knownX) findX(net_links[i].previous);
          if (net_links[i].next->x < net_links[i].previous->x) net_links[i].next->x = net_links[i].previous->x + 1;
        }
      }
    }
  }
  group->calculate_x = FALSE;

  group->knownX = TRUE;
}

/**
 *
 * Calcule la coordonnee y d'un groupe
 *
 */
void findY(type_group *group)
{
  int y_free;
  int group_id;
  type_script *script;
  type_group *group_other;

  y_free = 0;

  script = group->script;
  for (group_id = 0; group_id < script->number_of_groups; group_id++)
  {
    group_other = &script->groups[group_id];
    if (group_other->knownY && group_other->x == group->x)
    {
      y_free++;
    }
  }

  group->y = y_free;

  if (script->height < group->y) script->height = group->y;

  group->knownY = TRUE;
}

/**
 * Initialise les bibliotheques GTK et ENet, ainsi que quelques tableaux
 * @param argv, argc
 */

/****************************************** SIGNAUX ***********************************************/

void on_toggled_saving_button(GtkWidget *save_button, gpointer pData)
{
  (void) pData;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(save_button)))
  {
    saving_press = 1;
    gtk_widget_queue_draw(architecture_display);
  }
  else //Si le bouton est desactive
  {
    saving_press = 0;
    destroy_saving_ref(scripts); //on ferme tout les fichiers et on remet les on_saving à 0
    gtk_widget_queue_draw(architecture_display); // pour remettre à jour l'affichage quand tout les on_saving sont à 0
  }
}

void on_toggled_compress_button(GtkWidget *compress_button, gpointer pData)
{
  (void) pData;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compress_button)))
  {
    gtk_button_set_label(GTK_BUTTON(compress_button), "Deactivate compression (need restart)");
  }
  else //Si le bouton est desactive
  {
    gtk_button_set_label(GTK_BUTTON(compress_button), "Activate compression (need restart)");
  }
}

void on_destroy_control_window(GtkWidget *pWidget, gpointer pdata) //Fonction de fermeture d'une fenetre de controle
{
  type_script* script_actu = (type_script*) pdata;
  int j;

  (void) pWidget;

  for (j = 0; j < NUMBER_OF_CONTROL_TYPE; j++)
  {
    free(script_actu->control_group[j]);
  }
  free(script_actu->control_group);
  free(script_actu->number_of_control);

  script_actu->number_of_control = NULL;
  script_actu->control_group = NULL;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script_actu->control_button), FALSE);
  script_actu->pWindow = NULL;
}

void search_control_in_script_and_allocate_control(type_script* script_actu)
{
  type_group* liste_des_groupes = script_actu->groups;
  type_control** control_group = NULL;
  int i, j;
  int* number_of_control;
  int* count;

  number_of_control = MANY_ALLOCATIONS(NUMBER_OF_CONTROL_TYPE, int);
  count = MANY_ALLOCATIONS(NUMBER_OF_CONTROL_TYPE, int);

  for (j = 0; j < NUMBER_OF_CONTROL_TYPE; j++)
  {
    count[j] = 0;
    number_of_control[j] = 0;
  }

  for (i = 0; i < script_actu->number_of_groups; i++)
  {
    for (j = 0; j < NUMBER_OF_CONTROL_TYPE; j++)
    {
      if (!strcmp(liste_des_groupes[i].function, liste_controle_associee[j]))
      {
        number_of_control[j]++;
        liste_des_groupes[i].type_control = j;
      }
    }
  }

  control_group = MANY_ALLOCATIONS(NUMBER_OF_CONTROL_TYPE, type_control*);

  for (i = 0; i < NUMBER_OF_CONTROL_TYPE; i++)
  {
    control_group[i] = MANY_ALLOCATIONS(number_of_control[i], type_control);
  }

  for (i = 0; i < script_actu->number_of_groups; i++)
  {
    for (j = 0; j < NUMBER_OF_CONTROL_TYPE; j++)
    {
      if (liste_des_groupes[i].type_control == j)
      {
        control_group[j][count[j]].associated_control_widget = NULL;
        control_group[j][count[j]].associated_group = &(liste_des_groupes[i]);
        control_group[j][count[j]].type_de_controle = j;
        count[j]++;
      }
    }
  }

  script_actu->control_group = control_group;
  script_actu->number_of_control = number_of_control;

  free(count);

}

gboolean on_vue_metre_change(GtkWidget *gtk_range, type_group *group)
{
  maj_neuro_enet struct_maj;
  ENetPacket *packet = NULL;
  int retour;
  int j;
  j = (int) g_object_get_data(G_OBJECT(gtk_range), "neurone_asso");

  struct_maj.no_group = group->id;
  struct_maj.no_neuro = j + group->firstNeuron;
  struct_maj.s = struct_maj.s1 = struct_maj.s2 = (float) gtk_range_get_value(GTK_RANGE(gtk_range));

  packet = enet_packet_create((void*) (&struct_maj), sizeof(maj_neuro_enet), ENET_PACKET_FLAG_RELIABLE);
  sem_wait(&(enet_pandora_lock));
  if (packet == NULL)
  {
    PRINT_WARNING("The neurons packet (update) has not been created.");
  }
  else if ((retour = enet_peer_send(group->script->peer, ENET_MAJ_NEURONE, packet)) != 0)
  {
    PRINT_WARNING("The neurons packet has not been sent %d.", retour);
  }
  sem_post(&(enet_pandora_lock));

  return FALSE;
}

gboolean on_toggled_check_bouton(GtkWidget *check_bouton, type_group *group)
{
  maj_neuro_enet struct_maj;
  ENetPacket *packet = NULL;
  int retour;
  int j;
  j = (int) g_object_get_data(G_OBJECT(check_bouton), "neurone_asso");

  struct_maj.no_group = group->id;
  struct_maj.no_neuro = j + group->firstNeuron;
  struct_maj.s = struct_maj.s1 = struct_maj.s2 = (float) (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check_bouton)));

  packet = enet_packet_create((void*) (&struct_maj), sizeof(maj_neuro_enet), ENET_PACKET_FLAG_RELIABLE);
  sem_wait(&(enet_pandora_lock));
  if (packet == NULL)
  {
    PRINT_WARNING("The neurons packet (update) has not been created.");
  }
  else if ((retour = enet_peer_send(group->script->peer, ENET_MAJ_NEURONE, packet)) != 0)
  {
    PRINT_WARNING("The neurons packet has not been sent %d.", retour);
  }
  sem_post(&(enet_pandora_lock));

  return FALSE;
}

void create_range_controls(type_script* script_actu, GtkWidget *box1)
{
  int i, j;
  GtkWidget *box2, *grid;
  GtkWidget *label, *separator1, *separator2,*check_button;
  GtkRange *gtk_range;
  int* no_neuro;
  int k = 0;
  /* Standard window-creating stuff */

  /*----------------------------------------------------------------*/
  box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_set_homogeneous(GTK_BOX(box2), TRUE);
  label = gtk_label_new("Controles\n");
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);

  if (script_actu->number_of_control[VUE_METRE] > 0)
  {
    box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(box2), TRUE);
    label = gtk_label_new("VU-mètres : ");
    gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);

    grid = gtk_grid_new();

    /* on peut avoir plus de 2 VuMetre */
    for (i = 0; i < script_actu->number_of_control[VUE_METRE]; i++)
    {
      for (j = 0; j < ((script_actu->control_group[VUE_METRE][i]).associated_group)->number_of_neurons; j++)
      {

        /* creation de la boite pour le VuMetre dans la macro boite box1 */
        /* On inscrit d'abord le nom du VuMetre                          */
        //box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        //gtk_box_set_homogeneous (GTK_BOX(box2),FALSE);
        separator1 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
        separator2 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
        //gtk_container_set_border_width(GTK_CONTAINER(box2), 0);
        label = gtk_label_new(script_actu->control_group[VUE_METRE][i].associated_group->name_n);

        gtk_grid_attach(GTK_GRID(grid), label, 0, k, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), separator1, 1, k, 1, 1);

        // gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);
        //gtk_box_pack_start(GTK_BOX(box2), separator1, FALSE, FALSE, 0);

        gtk_range =
            (GtkRange*) gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,(((script_actu->control_group[VUE_METRE][i]).associated_group)->borne_min), script_actu->control_group[VUE_METRE][i].associated_group->borne_max, script_actu->control_group[VUE_METRE][i].associated_group->step);

        gtk_range_set_value(gtk_range, script_actu->control_group[VUE_METRE][i].associated_group->init);
        g_object_set_data(G_OBJECT(gtk_range), "neurone_asso", (gpointer) j);
        g_signal_connect(G_OBJECT(gtk_range), "value-changed", G_CALLBACK(on_vue_metre_change), script_actu->control_group[VUE_METRE][i].associated_group);

        gtk_widget_set_hexpand(GTK_WIDGET(gtk_range), TRUE);

        gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(gtk_range), 2, k, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), separator2, 3, k, 1, 1);

        //  gtk_box_pack_start(GTK_BOX(box2), (GtkWidget*) gtk_range, TRUE, TRUE, 0);
        // gtk_box_pack_start(GTK_BOX(box2), separator2, FALSE, FALSE, 0);
        // gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);
        k++;
      }
    }
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_pack_start(GTK_BOX(box1), grid, FALSE, FALSE, 0);
  }

  if (script_actu->number_of_control[CHECKBOX] != 0)
  {
    k = 0;
    box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(box2), TRUE);
    label = gtk_label_new("CheckBoxs : ");
    gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);
    grid = gtk_grid_new();

    for (i = 0; i < script_actu->number_of_control[CHECKBOX]; i++)
    {
      for (j = 0; j < script_actu->control_group[CHECKBOX][i].associated_group->number_of_neurons; j++)
      {

        separator1 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
        separator2 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
        //   label = gtk_label_new(script_actu->control_group[CHECKBOX][i].associated_group->name_n);

        check_button = GTK_WIDGET(gtk_check_button_new_with_label(script_actu->control_group[CHECKBOX][i].associated_group->name_n));

       // gtk_grid_attach(GTK_GRID(grid), label, 0, k, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), separator1, 0, k, 1, 1);
        if(script_actu->control_group[CHECKBOX][i].associated_group->init>0.5) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), TRUE);
        else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), FALSE);

        g_object_set_data(G_OBJECT(check_button), "neurone_asso", (gpointer) j);
        g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(on_toggled_check_bouton), script_actu->control_group[CHECKBOX][i].associated_group);

        gtk_widget_set_hexpand(GTK_WIDGET(check_button), TRUE);

        gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(check_button), 1, k, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), separator2, 2, k, 1, 1);

        k++;
      }

    }


    gtk_grid_set_column_homogeneous(GTK_GRID(grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_pack_start(GTK_BOX(box1), grid, FALSE, FALSE, 0);

  }

  gtk_widget_show_all(box1);

}

void on_toggled_affiche_control_button(GtkWidget *affiche_control_button, gpointer pData)
{
  GtkWidget* pWindow = NULL;
  GtkWidget *scrolled_window, *vbox;

  type_script* script_actu = (type_script*) pData;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(affiche_control_button)))
  {
    // chercher f_machin, mettre dans liste chainee

    //creation fenetre
    pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_double_buffered(pWindow, TRUE);
    script_actu->pWindow = pWindow;
    g_signal_connect(G_OBJECT(pWindow), "destroy", G_CALLBACK(on_destroy_control_window), pData);

    search_control_in_script_and_allocate_control(script_actu);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);

    gtk_container_add(GTK_CONTAINER(scrolled_window), vbox);
    gtk_container_add(GTK_CONTAINER(pWindow), scrolled_window);

    create_range_controls(script_actu, vbox);

    gtk_window_set_default_size(GTK_WINDOW(pWindow), 200, 400);
    gtk_window_set_title(GTK_WINDOW(pWindow), script_actu->name);
    gtk_widget_show_all(pWindow);

    // pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, group->script->name+strlen(bus_id)+1);
  }
  else //Si le bouton est desactive
  {
    //detruire fenetre
    gtk_widget_destroy(script_actu->pWindow);

    script_actu->pWindow = NULL;

    //lire liste chainee et detruire(ou pas)
    // arrete l'envoie de neurone
  }
}

void on_click_save_path_button(GtkWidget *save_button, gpointer pData) //TODO mettre dans le repertoire de l'utilisateur (ce serait bien que le bureau soir egalement dans ce repertoire)
{

  GtkWidget *selection = NULL;

  (void) pData;
  (void) save_button;
  selection = gtk_file_chooser_dialog_new("selectionner un repertoire", NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
  gtk_widget_show(selection);

  //On interdit l'utilisation des autres fenetres.
  //gtk_window_set_modal(GTK_WINDOW(selection), TRUE);

  if (gtk_dialog_run(GTK_DIALOG(selection)) == GTK_RESPONSE_OK)
  {
    char *filename = NULL;
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(selection));
    strcpy(path_named, filename);
    strcat(path_named, "/");
  }
  gtk_widget_destroy(selection);

}

void on_script_displayed_toggled(GtkWidget *pWidget, gpointer user_data)
{
  type_script *script = user_data;

  //Identification du script à afficher ou à masquer
  script->displayed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));

  gtk_widget_queue_draw(architecture_display);
  //architecture_display_update(architecture_display, NULL);
}

void window_title_update()
{
  /* Titre de la fenetre */
  snprintf(label_text, LABEL_MAX, "Pandora - [%s]", preferences_filename);
  gtk_window_set_title(GTK_WINDOW(window), label_text);
}

/**
 *
 * Fonction de fermeture de Pandora
 *
 * @return EXIT_SUCCESS
 */
gboolean window_close(GtkWidget *pWidget, GdkEvent *event, gpointer pData) //Fonction de fermeture de Pandora
{
  (void) pWidget;
  (void) event;
  (void) pData;

  exit(0);
  //gtk_main_quit();
  return FALSE;
}

void on_destroy_new_window(GtkWidget *pWidget, gpointer pdata) //Fonction de fermeture d'une fenetre secondaire d'affichage neuronnes
{
  GtkWidget *button;
  gpointer pbutton = g_object_get_data(G_OBJECT(pWidget), "toggle_button");
  //(void)pdata;

  button = (GtkWidget*) pbutton;

  printf("hop\n");
  gtk_widget_reparent((GtkWidget*) pdata, vpaned);
  //gtk_widget_destroy(pWidget);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);

}

void on_destroy(GtkWidget *pWidget, gpointer pdata)
{
  (void) pdata;
  gtk_widget_destroy(pWidget);
}

/**
 *
 * Un script change de plan
 *
 */

void change_plan(GtkWidget *pWidget, gpointer data) //Un script change de plan
{
  int i;
  type_script *script = data;
  script->z = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(pWidget));

  for (i = 0; i < number_of_scripts; i++)
    script_update_positions(scripts[i]);

  gtk_widget_queue_draw(architecture_display);
  //architecture_display_update(architecture_display, NULL); //On redessine avec la nouvelle valeur de z
}

gboolean on_scale_change_value(GtkWidget *gtk_range, GtkScrollType scroll, gdouble value, float* value_pointer)
{
  (void) gtk_range;
  (void) scroll;
  *value_pointer = (float) value;

  gtk_widget_queue_draw(architecture_display);
  //architecture_display_update(architecture_display, NULL);
  return FALSE;
}

void validate_gtk_entry_text(GtkEntry *pWidget, gpointer data)
{
  (void) pWidget;
  gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_OK);
}

/*void show_or_mask_default_text(GtkEntry *pWidget, gpointer data)
 {
 (void) data;
 gtk_entry_set_text(pWidget, "");
 }
 */

// ouvre la fenetre de recherche en selectionnant par defaut le script passe en parametre.
void on_search_group_button_active(GtkWidget *pWidget, type_script *script)
{
  int i, index = 0;
  (void) pWidget;

  for (i = 0; i < number_of_scripts && scripts[i] != script; i++)
    ;

  index = (i == number_of_scripts ? 0 : i);

  on_search_group(index);
}

void on_search_group(int index)
{
  static type_script *script = NULL;
  static int index_script = 0;

  float x = 0.0, y = 0.0;
  GtkWidget *content_area, *search_dialog, *search_entry, *h_box, *scripts_h_box, *name_radio_button_search, *function_radio_button_search, *scripts_combo_box, *scripts_label, *search_label, *search_h_box, *vbox;
  int i, last_occurence = 0, stop = 0, previous_index = -1;
  type_script *selected_script = NULL;
  char previous_research[TAILLE_CHAINE];

  if (number_of_scripts <= 0) return;

  if (index >= 0 && index < number_of_scripts) index_script = index;
  else for (i = 0; i < number_of_scripts; i++)
    if (scripts[i] == script) index_script = i;

  previous_research[0] = '\0';

//On cree une fenetre de dialogue
  search_dialog = gtk_dialog_new_with_buttons("Recherche d'un groupe", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  gtk_window_set_default_size(GTK_WINDOW(search_dialog), 300, 75);

  name_radio_button_search = gtk_radio_button_new_with_label_from_widget(NULL, "by name");
  function_radio_button_search = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(name_radio_button_search), "by function");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(name_radio_button_search), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(function_radio_button_search), FALSE);

  h_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(h_box), TRUE);
  gtk_box_pack_start(GTK_BOX(h_box), name_radio_button_search, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h_box), function_radio_button_search, TRUE, FALSE, 0);

  scripts_label = gtk_label_new("script : ");

  scripts_combo_box = gtk_combo_box_text_new();
  for (i = 0; i < number_of_scripts; i++)
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scripts_combo_box), scripts[i]->name);
  gtk_combo_box_set_active(GTK_COMBO_BOX(scripts_combo_box), index_script);

  scripts_h_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(scripts_h_box), scripts_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(scripts_h_box), scripts_combo_box, TRUE, TRUE, 0);

  search_label = gtk_label_new("");
//On lui ajoute une entree de saisie de texte
  search_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(search_entry), "");
  g_signal_connect(G_OBJECT(search_entry), "activate", G_CALLBACK(validate_gtk_entry_text), search_dialog);
  //g_signal_connect(G_OBJECT(search_entry), "clicked", G_CALLBACK(show_or_mask_default_text), search_dialog);
  search_h_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(search_h_box), search_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(search_h_box), search_entry, TRUE, TRUE, 0);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start(GTK_BOX(vbox), h_box, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), scripts_h_box, TRUE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), search_h_box, TRUE, FALSE, 0);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(search_dialog));
  gtk_container_add(GTK_CONTAINER(content_area), vbox);

  gtk_widget_show_all(search_dialog);

//Aucun groupe n'est selectionne
  selected_group = NULL;

  do
  {
    switch (gtk_dialog_run(GTK_DIALOG(search_dialog)))
    {
    //Si la reponse est OK
    case GTK_RESPONSE_OK:
      index_script = gtk_combo_box_get_active(GTK_COMBO_BOX(scripts_combo_box));
      selected_script = scripts[gtk_combo_box_get_active(GTK_COMBO_BOX(scripts_combo_box))];
      if (previous_index != gtk_combo_box_get_active(GTK_COMBO_BOX(scripts_combo_box)))
      {
        last_occurence = 0;
        previous_index = gtk_combo_box_get_active(GTK_COMBO_BOX(scripts_combo_box));
      }

      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(name_radio_button_search)))
      {
        last_occurence = 0;
        for (i = 0; i < selected_script->number_of_groups; i++)
        {
          if (strcmp(gtk_entry_get_text(GTK_ENTRY(search_entry)), selected_script->groups[i].name) == 0)
          {
            //Le groupe correspondant est selectionne
            selected_group = &selected_script->groups[i];
            architecture_get_group_position(selected_group, &x, &y);
            x -= (gtk_widget_get_allocated_width(scrollbars) - LARGEUR_GROUPE) / 2;
            if (x < 0.0) x = 0.0;
            y -= (gtk_widget_get_allocated_height(scrollbars) - HAUTEUR_GROUPE) / 2;
            if (y < 0.0) y = 0.0;
            architecture_set_view_point(scrollbars, x, y);
            break;
          }
        }
        stop = 1;
      }
      else
      {
        if (strcmp(previous_research, gtk_entry_get_text(GTK_ENTRY(search_entry))))
        {
          last_occurence = 0;
          strcpy(previous_research, gtk_entry_get_text(GTK_ENTRY(search_entry)));
        }

        for (i = last_occurence; i < selected_script->number_of_groups; i++)
        {
          if (strcmp(gtk_entry_get_text(GTK_ENTRY(search_entry)), selected_script->groups[i].function) == 0)
          {
            //Le groupe correspondant est selectionne
            selected_group = &selected_script->groups[i];
            // les coordonnees de ce groupe sont recuperees et le groupe est place au centre de l'affichage.
            architecture_get_group_position(selected_group, &x, &y);
            x -= (gtk_widget_get_allocated_width(scrollbars) - LARGEUR_GROUPE) / 2;
            if (x < 0.0) x = 0.0;
            y -= (gtk_widget_get_allocated_height(scrollbars) - HAUTEUR_GROUPE) / 2;
            if (y < 0.0) y = 0.0;

            architecture_set_view_point(scrollbars, x, y);
            last_occurence = i + 1;
            break;
          }
        }
      }
      break;
    case GTK_RESPONSE_CANCEL:
      stop = 1;
      break;
    case GTK_RESPONSE_NONE:
      stop = 1;
      break;
    default:
      stop = 1;
      break;
    }
    gtk_widget_queue_draw(architecture_display);
    //architecture_display_update(architecture_display, NULL);
  } while (stop != 1);

  index_script = gtk_combo_box_get_active(GTK_COMBO_BOX(scripts_combo_box));
  gtk_widget_destroy(search_dialog);
}

void on_hide_see_legend_button_active(GtkWidget *hide_see_legend_button, gpointer pData)
{
  gpointer arrow_im_2 = g_object_get_data(G_OBJECT(hide_see_legend_button), "arrow_im_2");
  (void) pData;

//Si le bouton est active
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_see_legend_button)))
  {
    //On cache le menu de droite
    gtk_widget_hide(pData);
    //On actualise l'image du boutton
    gtk_image_set_from_stock(GTK_IMAGE(arrow_im_2), GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU);

  }
//Si le bouton est desactive
  else
  {
    //On montre le menu de droite
    gtk_widget_show(pData);
    //On actualise l'image du boutton;
    gtk_image_set_from_stock(GTK_IMAGE(arrow_im_2), GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
  }

}

void on_hide_see_scales_button_active(GtkWidget *hide_see_scales_button, gpointer pData)
{
  gpointer arrow_im = g_object_get_data(G_OBJECT(hide_see_scales_button), "arrow_im");

  (void) pData;

//Si le bouton est active
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_see_scales_button)))
  {
    //On cache le menu de gauche
    gtk_widget_hide(pData);
    //On actualise l'image du boutton
    gtk_image_set_from_stock(GTK_IMAGE(arrow_im), GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);

  }
//Si le bouton est desactive
  else
  {
    //On montre le menu de gauche
    gtk_widget_show(pData);
    //On actualise l'image du boutton;
    gtk_image_set_from_stock(GTK_IMAGE(arrow_im), GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
  }
}

void on_check_button_draw_active(GtkWidget *check_button, gpointer data)
{
  (void) check_button;
  (void) data;

  graphic.draw_links = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_connections));
  graphic.draw_net_links = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_net_connections));
  gtk_widget_queue_draw(architecture_display);
}

void defaultScale(GtkWidget *pWidget, gpointer pData)
{
  (void) pWidget;
  (void) pData;

  gtk_range_set_value(GTK_RANGE(refreshScale), REFRESHSCALE_DEFAULT);
  gtk_range_set_value(GTK_RANGE(xScale), XSCALE_DEFAULT);
  gtk_range_set_value(GTK_RANGE(yScale), YSCALE_DEFAULT);
  gtk_range_set_value(GTK_RANGE(zxScale), XGAP_DEFAULT);
  gtk_range_set_value(GTK_RANGE(zyScale), YGAP_DEFAULT);

  graphic.x_scale = XSCALE_DEFAULT;
  graphic.y_scale = YSCALE_DEFAULT;
  graphic.zx_scale = XGAP_DEFAULT;
  graphic.zy_scale = YGAP_DEFAULT;

  gtk_widget_queue_draw(architecture_display);
}

void on_group_display_output_combobox_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = (type_group*) data;
  int new_output = gtk_combo_box_get_active(combo_box), i;
  prom_images_struct *images;
  int height = 0;
  char legende[64];
  // GtkWidget *big_graph_frame = GTK_WIDGET(gtk_builder_get_object(builder, "list_graph"));
  int nb;

  resize_group(group);
  if (new_output == 3 && group->output_display != 3)
  {
    if (refresh_mode != REFRESH_MODE_MANUAL)
    {
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, group->script->name + strlen(bus_id) + 1);
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_START, group->id, group->script->name + strlen(bus_id) + 1);
    }
    if (gtk_widget_get_visible(big_graph_frame) == TRUE)
    {
      gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "list_graph")));
      height -= gtk_widget_get_allocated_height(big_graph_frame);
    }
    if (group->ext != NULL && ((prom_images_struct *) group->ext)->image_number > 0)
    {
      nb = gtk_tree_model_iter_n_children(gtk_combo_box_get_model(GTK_COMBO_BOX(selected_image_combo_box)), NULL);
      for (i = 0; i < nb; i++)
        gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(selected_image_combo_box), 0);
      images = group->ext;
      printf("\n    image number : %d", (int) images->image_number);
      for (i = 0; i < (int) images->image_number; i++)
      {
        sprintf(legende, "image %d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(selected_image_combo_box), legende);
      }
      gtk_combo_box_set_active(GTK_COMBO_BOX(selected_image_combo_box), group->image_selected_index);
      if (gtk_widget_get_visible(image_hbox) == FALSE)
      {
        gtk_widget_show_all(image_hbox);
        height += gtk_widget_get_allocated_height(image_hbox);
      }
    }
    gtk_window_resize(GTK_WINDOW(selected_group_dialog), gtk_widget_get_allocated_width(selected_group_dialog), gtk_widget_get_allocated_height(selected_group_dialog) + height);
  }
  else if (new_output != 3 && group->output_display == 3)
  {
    if (refresh_mode != REFRESH_MODE_MANUAL)
    {
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_STOP, group->id, group->script->name);
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, group->script->name);
    }
    if (group->display_mode == DISPLAY_MODE_BIG_GRAPH && gtk_widget_get_visible(big_graph_frame) == FALSE)
    {
      height += gtk_widget_get_allocated_height(big_graph_frame);
      gtk_widget_show_all(big_graph_frame);
    }
    if (gtk_widget_get_visible(image_hbox) == TRUE)
    {
      gtk_widget_hide(image_hbox);
      height -= gtk_widget_get_allocated_height(image_hbox);
    }
    gtk_window_resize(GTK_WINDOW(selected_group_dialog), gtk_widget_get_allocated_width(selected_group_dialog), gtk_widget_get_allocated_height(selected_group_dialog) + height);
  }

  group->previous_output_display = group->output_display;
  group->output_display = gtk_combo_box_get_active(combo_box);
  resize_group(group);

}

void on_group_display_mode_combobox_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = (type_group*) data;

  group->previous_display_mode = group->display_mode;
  group->display_mode = gtk_combo_box_get_active(combo_box);

  if (group->display_mode == DISPLAY_MODE_BIG_GRAPH && gtk_widget_get_visible(big_graph_frame) == FALSE && group->output_display != 3)
  {
    /*  gtk_window_resize(GTK_WINDOW(selected_group_dialog), gtk_widget_get_allocated_width(selected_group_dialog), gtk_widget_get_allocated_height(selected_group_dialog) + gtk_widget_get_allocated_height(big_graph_frame));*/
    gtk_widget_show_all(big_graph_frame);
  }
  else if (group->display_mode != DISPLAY_MODE_BIG_GRAPH && gtk_widget_get_visible(big_graph_frame) == TRUE)
  {
    gtk_widget_hide(big_graph_frame);
    //gtk_window_resize(GTK_WINDOW(selected_group_dialog), selected_group_dialog->allocation.width, selected_group_dialog->allocation.height - frame->allocation.height);
  }
  resize_group(group);
}

void resize_group(type_group *group)
{
  int nb_col, nb_lin;
  float hauteur, largeur;

  if (group->output_display == 3)
  {
    gtk_widget_hide(GTK_WIDGET(group->button_vbox));
  }
  else if (group->display_mode == DISPLAY_MODE_BIG_GRAPH)
  {
    gtk_widget_show_all(GTK_WIDGET(group->button_vbox));
    //gtk_widget_set_size_request(group->widget, GRAPH_WIDTH, GRAPH_HEIGHT);
    gtk_widget_set_size_request(group->drawing_area, GRAPH_WIDTH - BUTTON_WIDTH, GRAPH_HEIGHT);
  }
  else
  {
    gtk_widget_hide(GTK_WIDGET(group->button_vbox));
    // gtk_widget_set_size_request(group->widget, get_width_height(group->columns), get_width_height(group->rows));
    //largeur_neur=get_width_height(group->columns,group->rows);
    //gtk_widget_set_size_request(group->drawing_area, group->columns*largeur_neur, group->rows*largeur_neur);

    if (group->columns > 0) nb_col = group->columns;
    else nb_col = 1;
    if (group->rows > 0) nb_lin = group->rows;
    else nb_lin = 1;

    largeur = (float) nb_col * group->neurons_width;
    hauteur = (float) nb_lin * group->neurons_height;

    //gtk_widget_set_size_request(group->drawing_area, get_width_height(group->columns), get_width_height(group->rows));
    gtk_widget_set_size_request(group->drawing_area, (gint) largeur, (gint) hauteur);
    gtk_widget_queue_draw(group->drawing_area);

  }
}

void on_group_display_min_spin_button_value_changed(GtkSpinButton *spin_button, gpointer data)
{
  type_group *group = data;
  group->val_min = gtk_spin_button_get_value(spin_button);
  resize_group(group);
}

void on_group_display_max_spin_button_value_changed(GtkSpinButton *spin_button, gpointer data)
{
  type_group *group = data;
  group->val_max = gtk_spin_button_get_value(spin_button);
  resize_group(group);
}

void on_group_display_auto_checkbox_toggled(GtkToggleButton *button, gpointer data)
{
  type_group *group = data;
  group->normalized = gtk_toggle_button_get_active(button);
  resize_group(group);
}

void on_group_display_line_spin_value_changed(GtkSpinButton *spin_button, gpointer data)
{
  data_courbe *courbe = data;
  int new_value = gtk_spin_button_get_value_as_int(spin_button);
  float r, g, b;
  char legende[126];

  //pthread_mutex_lock(&mutex_script_caracteristics);
  if (courbe->line != new_value)
  {
    courbe->last_index = -1;
    courbe->line = new_value;
    graph_get_line_color(courbe->indice, &r, &g, &b);
    sprintf(legende, "<b><span foreground=\"#%02X%02X%02X\"> %dx%d</span></b>\n", (int) (r * 255), (int) (g * 255), (int) (b * 255), courbe->column, courbe->line);
    gtk_label_set_markup(GTK_LABEL(courbe->check_box_label), legende);
  }
  //pthread_mutex_unlock(&mutex_script_caracteristics);
}

void on_group_display_column_spin_value_changed(GtkSpinButton *spin_button, gpointer data)
{
  data_courbe *courbe = data;
  int new_value = gtk_spin_button_get_value_as_int(spin_button);
  float r, g, b;
  char legende[126];

  //pthread_mutex_lock(&mutex_script_caracteristics);
  if (courbe->column != new_value)
  {
    courbe->last_index = -1;
    courbe->column = new_value;
    graph_get_line_color(courbe->indice, &r, &g, &b);
    sprintf(legende, "<b><span foreground=\"#%02X%02X%02X\"> %dx%d</span></b>\n", (int) (r * 255), (int) (g * 255), (int) (b * 255), courbe->column, courbe->line);
    gtk_label_set_markup(GTK_LABEL(courbe->check_box_label), legende);
  }
  //pthread_mutex_unlock(&mutex_script_caracteristics);
}

void on_group_display_selected_image_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = data;
  int temp = 0;
  temp = gtk_combo_box_get_active(combo_box);
  if (group != NULL)
  {
    if (group->ext != NULL)
    {
      if (temp >= 0 && temp < ((prom_images_struct*) (group->ext))->image_number)
      {
        group->image_selected_index = temp;
      }
    }
  }
  resize_group(group);
}

void graph_stop_or_not(GtkWidget *pWidget, gpointer pData)
{
  (void) pData;
  stop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));
  if (stop == TRUE) gtk_button_set_label(GTK_BUTTON(pWidget), "Record graphs");
  else gtk_button_set_label(GTK_BUTTON(pWidget), "Stop graphs");
}

void phases_info_start_or_stop(GtkToggleButton *pWidget, gpointer pData)
{
  int i, j;
  (void) pData;

  calculate_executions_times = gtk_toggle_button_get_active(pWidget);
  //printf("on rentre dans le bouton\n");
  if (calculate_executions_times == TRUE)
  {
    // printf("on rentre dans le bouton TRUE\n");

    for (i = 0; i < number_of_scripts; i++)
    {
      for (j = 0; j < scripts[i]->number_of_groups; j++)
      {
        scripts[i]->groups[j].stats.somme_temps_executions = 0;
        scripts[i]->groups[j].stats.nb_executions_tot = 0;
        scripts[i]->groups[j].stats.initialiser = TRUE;
        scripts[i]->groups[j].stats.somme_tot = 0;
      }
    }

    init_top(prompt_buf, p_buf);
    gtk_button_set_label(GTK_BUTTON(pWidget), "Stop calculate execution times");
    for (i = 0; i < number_of_scripts; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_PHASES_INFO_START, 0, scripts[i]->name + strlen(bus_id) + 1);
    //printf("ordre d'envoie bien lance\n");
  }
  else
  {
    gtk_button_set_label(GTK_BUTTON(pWidget), "Start calculate execution times");
    for (i = 0; i < number_of_scripts; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_PHASES_INFO_STOP, 0, scripts[i]->name + strlen(bus_id) + 1);

    for (i = 0; i < number_of_scripts; i++)
    {
      for (j = 0; j < scripts[i]->number_of_groups; j++)
      {
        scripts[i]->groups[j].stats.somme_temps_executions = 0;
        scripts[i]->groups[j].stats.nb_executions_tot = 0;
      }
    }
  }
}

void debug_grp_mem_info_start_or_stop(GtkToggleButton *pWidget, gpointer pData)
{
  int i;
  gboolean lancement_debug;

  (void) pData;

  lancement_debug = gtk_toggle_button_get_active(pWidget);
  //printf("on rentre dans le bouton\n");
  if (lancement_debug == TRUE)
  {
    for (i = 0; i < number_of_scripts; i++)
    {
      gtk_button_set_label(GTK_BUTTON(pWidget), "Stop Debug Group Functions");
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_OK_DEBUG_GRP_MEM, 0, scripts[i]->name + strlen(bus_id) + 1);
      //printf("ordre d'envoie bien lancé\n");
    }
  }

  else
  {
    for (i = 0; i < number_of_scripts; i++)
    {
      gtk_button_set_label(GTK_BUTTON(pWidget), "Start Debug Group Functions");
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_STOP_DEBUG_GRP_MEM, 0, scripts[i]->name + strlen(bus_id) + 1);
    }
  }

}

void on_button_draw_links_info_pressed(GtkToggleButton *pWidget, gpointer pData)
{
  (void) pData;

  draw_links_info = gtk_toggle_button_get_active(pWidget);
  //printf("on rentre dans le bouton\n");
  if (draw_links_info == TRUE)
  {
    gtk_button_set_label(GTK_BUTTON(pWidget), "Stop drawing links info");
  }
  else
  {
    gtk_button_set_label(GTK_BUTTON(pWidget), "Draw links info");
  }
}

void on_group_display_show_or_mask_neuron(GtkWidget *pWidget, gpointer pData)
{
  int *data = pData;
  *data = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));
}

void on_group_display_clicked(GtkButton *button, type_group *group)
{
  GtkWidget *vbox, *hbox, *label, *left_spinner, *right_spinner, *dialog_box;
  GtkWidget *group_display_output_combobox, *group_display_mode_combobox, *group_display_min_spin_button, *group_display_max_spin_button, *group_display_auto_checkbox, *grid;
  int height = 0;
  float r, g, b;
  char markup[126];
  int i;
  prom_images_struct *images;
  (void) button;

  selected_group_dialog = gtk_dialog_new();
  dialog_box = gtk_dialog_get_content_area(GTK_DIALOG(selected_group_dialog));
  gtk_window_set_title(GTK_WINDOW(selected_group_dialog), group->name);

  grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(dialog_box), grid);

  group_display_output_combobox = gtk_combo_box_text_new();
  group_display_mode_combobox = gtk_combo_box_text_new();
  group_display_min_spin_button = gtk_spin_button_new_with_range(-1000000000, 1000000000, 0.01);
  group_display_max_spin_button = gtk_spin_button_new_with_range(-1000000000, 1000000000, 0.01);

  group_display_auto_checkbox = gtk_toggle_button_new_with_label("auto");

  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_output_combobox), "s");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_output_combobox), "s1");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_output_combobox), "s2");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_output_combobox), "ext");

  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_mode_combobox), "squares");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_mode_combobox), "circles");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_mode_combobox), "grey levels");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_mode_combobox), "grey levels fast");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_mode_combobox), "bar graphs");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_mode_combobox), "texts");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_mode_combobox), "graphs");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(group_display_mode_combobox), "big graph");

  gtk_grid_attach(GTK_GRID(grid), group_display_output_combobox, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), group_display_mode_combobox, 1, 0, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), group_display_min_spin_button, 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), group_display_max_spin_button, 1, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), group_display_auto_checkbox, 2, 1, 1, 1);

  /// partie de la fenetre permettant de pre-selectionner les neurones à afficher dans le graphe
  big_graph_frame = gtk_frame_new("Neurons to display (column x row)");
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(vbox), TRUE);
  gtk_container_add(GTK_CONTAINER(big_graph_frame), vbox);
  gtk_grid_attach(GTK_GRID(grid), big_graph_frame, 0, 2, 3, 1);

  for (i = 0; i < group->number_of_courbes; i++) // signaux à creer et ajouter aux spinners.
  {
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    label = gtk_label_new(NULL);
    graph_get_line_color(i, &r, &g, &b);
    sprintf(markup, "<b><span foreground=\"#%02X%02X%02X\">Courbe %d</span></b>\n", (int) (r * 255), (int) (g * 255), (int) (b * 255), i);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);

    left_spinner = gtk_spin_button_new_with_range(0, group->columns - 1, 1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(left_spinner), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(left_spinner), group->courbes[i].column);
    gtk_box_pack_start(GTK_BOX(hbox), left_spinner, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(left_spinner), "value_changed", G_CALLBACK(on_group_display_column_spin_value_changed), &(group->courbes[i]));

    right_spinner = gtk_spin_button_new_with_range(0, group->rows - 1, 1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(right_spinner), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(right_spinner), group->courbes[i].line);
    gtk_box_pack_start(GTK_BOX(hbox), right_spinner, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(right_spinner), "value_changed", G_CALLBACK(on_group_display_line_spin_value_changed), &(group->courbes[i]));
    gtk_container_add(GTK_CONTAINER(vbox), hbox);

  }
  image_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  label = gtk_label_new("Selected image : ");
  gtk_box_pack_start(GTK_BOX(image_hbox), label, TRUE, FALSE, 0);
  selected_image_combo_box = gtk_combo_box_text_new();
  gtk_box_pack_start(GTK_BOX(image_hbox), selected_image_combo_box, TRUE, FALSE, 0);
  g_signal_connect(G_OBJECT(selected_image_combo_box), "changed", G_CALLBACK(on_group_display_selected_image_changed), group);
  gtk_grid_attach(GTK_GRID(grid), image_hbox, 0, 3, 1, 1);

  gtk_widget_show_all(selected_group_dialog);

  if (group->display_mode != DISPLAY_MODE_BIG_GRAPH || group->output_display == 3)
  {
    gtk_widget_hide(big_graph_frame);
    height = gtk_widget_get_allocated_height(big_graph_frame);
  }
  else height = 0;

  if (group->output_display == 3 && group->ext != NULL && ((prom_images_struct *) group->ext)->image_number > 0)
  {
    images = group->ext;
    for (i = 0; i < images->image_number; i++)
    {
      sprintf(markup, "image %d", i);
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(selected_image_combo_box), markup);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(selected_image_combo_box), group->image_selected_index);
  }
  else
  {
    gtk_widget_hide(image_hbox);
    /*height += gtk_widget_get_allocated_height(image_hbox);*/
  }
  gtk_window_resize(GTK_WINDOW(selected_group_dialog), gtk_widget_get_allocated_width(selected_group_dialog), gtk_widget_get_allocated_height(selected_group_dialog) - height);

  gtk_combo_box_set_active(GTK_COMBO_BOX(group_display_output_combobox), group->output_display);
  gtk_combo_box_set_active(GTK_COMBO_BOX(group_display_mode_combobox), group->display_mode);

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(group_display_min_spin_button), group->val_min);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(group_display_max_spin_button), group->val_max);

  g_signal_connect(G_OBJECT(group_display_output_combobox), "changed", G_CALLBACK(on_group_display_output_combobox_changed), group);
  g_signal_connect(G_OBJECT(group_display_mode_combobox), "changed", G_CALLBACK(on_group_display_mode_combobox_changed), group);

  g_signal_connect(G_OBJECT(group_display_min_spin_button), "value_changed", G_CALLBACK(on_group_display_min_spin_button_value_changed), group);
  g_signal_connect(G_OBJECT(group_display_max_spin_button), "value_changed", G_CALLBACK(on_group_display_max_spin_button_value_changed), group);
  g_signal_connect(G_OBJECT(group_display_auto_checkbox), "toggled", G_CALLBACK(on_group_display_auto_checkbox_toggled), group);

}

void script_widget_update(type_script *script)
{
  char label_text[LABEL_MAX];
  //script->label=NULL;
  //script->label=gtk_label_new("");
  sprintf(label_text, "<span><b>%s</b></span>", script->name);
  gtk_label_set_markup(GTK_LABEL(script->label), label_text);
  gtk_widget_override_color(script->label, GTK_STATE_FLAG_NORMAL, &colors[script->color]);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(script->z_spinnner), script->z);
}

void script_update_positions(type_script *script)
{
  int i, y_min = 0;
  type_script *other_script;

  /**Calcul du x de chaque groupe
   */
  for (i = 0; i < script->number_of_groups; i++)
  {
    if (script->groups[i].knownX == FALSE) findX(&script->groups[i]);
  }

  /**Calcul du y de chaque groupe
   */
  for (i = 0; i < script->number_of_groups; i++)
  {
    if (script->groups[i].knownY == FALSE) findY(&script->groups[i]);
  }

  for (i = 0; i < number_of_scripts; i++)
  {
    other_script = scripts[i];
    if (script != other_script)
    {
      if (script->z == other_script->z)
      {
        if (y_min < other_script->y_offset + other_script->height) y_min = other_script->y_offset + other_script->height;
      }
    }
    else break;
  }

  script->y_offset = y_min + 1;

}

void script_update_display(type_script *script)
{
  GtkWidget *search_icon;
  GtkWidget *control_icon;
  /**On veut determiner zMax, la plus grande valeur de z parmi les scripts ouverts
   */
  zMax = number_of_scripts;

  /**On remplit le panneau des scripts
   */
  gdk_threads_enter();

  script->widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxScripts), script->widget, FALSE, TRUE, 0);

//Cases à cocher pour afficher les scripts ou pas
  script->checkbox = gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script->checkbox), FALSE); //Par defaut, les scripts ne sont pas coches, donc pas affiches
  g_signal_connect(G_OBJECT(script->checkbox), "toggled", G_CALLBACK(on_script_displayed_toggled), script);
  gtk_box_pack_start(GTK_BOX(script->widget), script->checkbox, FALSE, TRUE, 0);

//Labels des scripts : le nom du script ecrit en gras, de la couleur du script
  script->label = gtk_label_new("");
  gtk_label_set_use_markup(GTK_LABEL(script->label), TRUE);
  gtk_box_pack_start(GTK_BOX(script->widget), script->label, TRUE, TRUE, 0);

  script->z_spinnner = gtk_spin_button_new_with_range(0, NB_SCRIPTS_MAX, 1); //Choix du plan dans lequel on affiche le script. On n'a pas besoin de plus de plans qu'il n'y a de scripts
  gtk_box_pack_start(GTK_BOX(script->widget), script->z_spinnner, FALSE, TRUE, 0);
  g_signal_connect(G_OBJECT(script->z_spinnner), "value-changed", G_CALLBACK(change_plan), script);

  script->search_button = gtk_toggle_button_new();
  search_icon = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
  gtk_button_set_image(GTK_BUTTON(script->search_button), search_icon);
  gtk_box_pack_start(GTK_BOX(script->widget), script->search_button, FALSE, TRUE, 0);
  g_signal_connect(G_OBJECT(script->search_button), "toggled", G_CALLBACK(on_search_group_button_active), script);

  script->control_button = gtk_toggle_button_new();
  control_icon = gtk_image_new_from_icon_name("preferences-system", GTK_ICON_SIZE_MENU);
  gtk_button_set_image(GTK_BUTTON(script->control_button), control_icon);
  gtk_box_pack_start(GTK_BOX(script->widget), script->control_button, FALSE, TRUE, 0);

  g_signal_connect(G_OBJECT(script->control_button), "toggled", G_CALLBACK(on_toggled_affiche_control_button), script);

//Pour que les cases soient cochees par defaut
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script->checkbox), TRUE);
  script->displayed = TRUE;

  gtk_widget_show_all(pVBoxScripts); //Affichage du widget pWindow et de tous ceux qui sont dedans

  script_widget_update(script); //TODO erreur ici probablement
  gtk_widget_queue_draw(architecture_display);
  gdk_threads_leave();
}

void drag_drop_neuron_frame(GtkWidget *pWidget, GdkEventButton *event, gpointer data)
{

  gint x = 0;
  gint y = 0;
  GdkEventMotion* motion_event = (GdkEventMotion*) event;
  (void) data;
  gtk_widget_translate_coordinates(pWidget, zone_neurons, (gint) motion_event->x, (gint) motion_event->y, &x, &y);

  // s ey y sont dans les coordonnees de zone neurons, move_neuron_oldx et y sont dans les coordonnees du widget
  if (move_neurons_group != NULL && move_neurons_start == TRUE)
  {
    if (x - move_neurons_old_x < 0) x = move_neurons_old_x;
    if (y - move_neurons_old_y < 0) y = move_neurons_old_y;
    if (x - move_neurons_old_x + gtk_widget_get_allocated_width(pWidget) > gtk_widget_get_allocated_width(zone_neurons)) x = gtk_widget_get_allocated_width(zone_neurons) + move_neurons_old_x - gtk_widget_get_allocated_width(pWidget);
    if (y - move_neurons_old_y + gtk_widget_get_allocated_height(pWidget) > gtk_widget_get_allocated_height(zone_neurons)) y = gtk_widget_get_allocated_height(zone_neurons) + move_neurons_old_y - gtk_widget_get_allocated_height(pWidget);

    //gtk_layout_move(GTK_LAYOUT(zone_neurons), move_neurons_group->widget, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
    gtk_layout_move(GTK_LAYOUT(zone_neurons), pWidget, ((int) (round((x - move_neurons_old_x) / 25.0) * 25)), ((int) (round((y - move_neurons_old_y) / 25.0) * 25)));

    move_neurons_group = NULL;
    move_neurons_start = FALSE;
  }
  else if (open_group != NULL && open_neurons_start == TRUE)
  {
    if (x - move_neurons_old_x < 0) x = move_neurons_old_x;
    if (y - move_neurons_old_y < 0) y = move_neurons_old_y;
    if (x - move_neurons_old_x + gtk_widget_get_allocated_width(pWidget) > gtk_widget_get_allocated_width(zone_neurons)) x = gtk_widget_get_allocated_width(zone_neurons) + move_neurons_old_x - gtk_widget_get_allocated_width(pWidget);
    if (y - move_neurons_old_y + gtk_widget_get_allocated_height(pWidget) > gtk_widget_get_allocated_height(zone_neurons)) y = gtk_widget_get_allocated_height(zone_neurons) + move_neurons_old_y - gtk_widget_get_allocated_height(pWidget);

    gtk_layout_move(GTK_LAYOUT(zone_neurons), pWidget, ((int) (round((x - move_neurons_old_x) / 25.0) * 25)), ((int) (round((y - move_neurons_old_y) / 25.0) * 25)));
    //group_display_new(open_group, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
    open_neurons_start = FALSE;

  }
}

void on_release(GtkWidget *zone_neurons, GdkEventButton *event, gpointer data)
{
  (void) data;
  (void) event;
  (void) zone_neurons;

  if (move_neurons_group != NULL && move_neurons_start == TRUE)
  {
    move_neurons_group = NULL;
    move_neurons_start = FALSE;
  }
  else if (open_group != NULL && open_neurons_start == TRUE)
  {
    open_neurons_start = FALSE;
  }

}

void neurons_frame_drag_group(GtkWidget *pWidget, GdkEvent *event, gpointer pdata)
{
  gint x = 0;
  gint y = 0;
  GdkEventMotion* motion_event = (GdkEventMotion*) event;

  (void) pdata;

  gtk_widget_translate_coordinates(pWidget, zone_neurons, (gint) motion_event->x, (gint) motion_event->y, &x, &y);

  if (move_neurons_start)
  {

    if (x - move_neurons_old_x < 0) x = move_neurons_old_x;
    if (y - move_neurons_old_y < 0) y = move_neurons_old_y;
    if (x - move_neurons_old_x + gtk_widget_get_allocated_width(pWidget) > gtk_widget_get_allocated_width(zone_neurons)) x = gtk_widget_get_allocated_width(zone_neurons) + move_neurons_old_x - gtk_widget_get_allocated_width(pWidget);
    if (y - move_neurons_old_y + gtk_widget_get_allocated_height(pWidget) > gtk_widget_get_allocated_height(zone_neurons)) y = gtk_widget_get_allocated_height(zone_neurons) + move_neurons_old_y - gtk_widget_get_allocated_height(pWidget);

    gtk_layout_move(GTK_LAYOUT(zone_neurons), pWidget, x - move_neurons_old_x, y - move_neurons_old_y);

    //gtk_layout_move(GTK_LAYOUT(zone_neurons), move_neurons_group->widget, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
  }
  else if (open_group != NULL && open_neurons_start == TRUE)
  {
    if (x - move_neurons_old_x < 0) x = move_neurons_old_x;
    if (y - move_neurons_old_y < 0) y = move_neurons_old_y;
    if (x - move_neurons_old_x + gtk_widget_get_allocated_width(pWidget) > gtk_widget_get_allocated_width(zone_neurons)) x = gtk_widget_get_allocated_width(zone_neurons) + move_neurons_old_x - gtk_widget_get_allocated_width(pWidget);
    if (y - move_neurons_old_y + gtk_widget_get_allocated_height(pWidget) > gtk_widget_get_allocated_height(zone_neurons)) y = gtk_widget_get_allocated_height(zone_neurons) + move_neurons_old_y - gtk_widget_get_allocated_height(pWidget);

    gtk_layout_move(GTK_LAYOUT(zone_neurons), pWidget, x - move_neurons_old_x, y - move_neurons_old_y);
    //gtk_layout_move(GTK_LAYOUT(zone_neurons), open_group->widget, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
  }
}

gboolean script_caracteristics(type_script *script, int action)
{
  static type_script_display_save *saves[NB_SCRIPTS_MAX];
  static int number_of_saves = 0;
  new_group_argument *arguments;
  type_group *group;
  int i, j, save_id = 0, number_of_group_displayed = 0;
  type_script_display_save *save = NULL;
  GtkAllocation allocation;
  // new_group_argument arguments;

  if (script == NULL) return FALSE;

  if (action == APPLY_SCRIPT_GROUPS_CARACTERISTICS)
  {
    for (i = 0; i < number_of_saves; i++)
      if (strcmp(saves[i]->name, script->name) == 0)
      {
        save = saves[i];
        save_id = i;
      }
    if (save == NULL) return FALSE;

    pthread_mutex_lock(&mutex_script_caracteristics);
    group = script->groups;
    number_of_group_displayed = script->number_of_groups;
    for (i = 0; i < number_of_group_displayed; i++)
      for (j = 0; j < save->number_of_groups; j++)
        if (strcmp(group[i].name, save->groups[j].name) == 0)
        {
          group[i].display_mode = save->groups[j].display_mode;
          group[i].normalized = save->groups[j].normalized;
          group[i].output_display = save->groups[j].output_display;
          group[i].val_min = save->groups[j].val_min;
          group[i].val_max = save->groups[j].val_max;
          // gdk_threads_enter();

          arguments = ALLOCATION(new_group_argument);
          arguments->group = &(group[i]);
          arguments->posx = save->groups[j].x;
          arguments->posy = save->groups[j].y;
          arguments->zone_neuron = zone_neurons;
          arguments->blocked = TRUE;
          // pthread_mutex_unlock(&mutex_script_caracteristics);
          // g_idle_add ((GSourceFunc)group_display_new_threaded,(gpointer)&arguments);
          pthread_mutex_unlock(&mutex_script_caracteristics);
          g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc) group_display_new_threaded, (gpointer) arguments, NULL);

          //Il est important d'attendre car sinon on envoie une fonction à executer dans la boucle principale puis on la renvoie sans attendre que la precedente ai finie, ce qui cause des problemes, il est notemment complique de passer des arguments differents car on les modifie lors de la boucle suivante. //TODO : envisager un tableau d'argument aussi grand que la longueur de la boucle for? Aurais l'avantage de ne pas penaliser le script enet.
          pthread_mutex_lock(&mutex_loading);
          pthread_cond_wait(&cond_loading, &mutex_loading);
          pthread_mutex_unlock(&mutex_loading);
          //group_display_new(&(group[i]), save->groups[j].x, save->groups[j].y);
          pthread_mutex_lock(&mutex_script_caracteristics);
          //pthread_mutex_lock(&mutex_script_caracteristics);

          //gdk_threads_leave();
        }
    number_of_saves--;
    free(save->groups);
    free(save);
    if (number_of_saves > 0) saves[save_id] = saves[number_of_saves];
    pthread_mutex_unlock(&mutex_script_caracteristics);

    return TRUE;
  }
  else if (action == SAVE_SCRIPT_GROUPS_CARACTERISTICS && number_of_saves < NB_SCRIPTS_MAX)
  {
    for (i = 0; i < number_of_groups_to_display; i++)
      if (groups_to_display[i]->script == script) number_of_group_displayed++;

    if (number_of_group_displayed == 0) return TRUE;

    for (j = 0; j < number_of_saves; j++)
      if (strcmp(saves[j]->name, script->name) == 0) save = saves[j];

    if (save == NULL)
    {
      save = malloc(sizeof(type_script_display_save));
      saves[number_of_saves] = save;
      number_of_saves++;
    }
    save->groups = malloc(number_of_group_displayed * sizeof(type_group_display_save));
    save->number_of_groups = number_of_group_displayed;
    strcpy(save->name, script->name);

    j = 0;
    for (i = 0; i < number_of_groups_to_display; i++)
    {
      group = groups_to_display[i];
      if (group->script == script)
      {
        strcpy(save->groups[j].name, group->name);
        save->groups[j].display_mode = group->display_mode;
        save->groups[j].output_display = group->output_display;
        save->groups[j].val_min = group->val_min;
        save->groups[j].val_max = group->val_max;
        save->groups[j].normalized = group->normalized;

        gtk_widget_get_allocation(GTK_WIDGET(group->widget), &allocation);

        save->groups[j].x = allocation.x;
        save->groups[j].y = allocation.y;
        j++;
      }
    }
    return TRUE;
  }
  return FALSE;
}

void script_destroy(type_script *script)
{
  int i, j;
  type_group *group;

  //pthread_mutex_lock(&mutex_script_caracteristics);
  script_caracteristics(script, SAVE_SCRIPT_GROUPS_CARACTERISTICS);
  //pthread_mutex_unlock(&mutex_script_caracteristics);

  if (script->pWindow != NULL)
  {
    gtk_widget_destroy(script->pWindow);
  }

  for (i = 0; i < number_of_groups_to_display; i++)
  {
    group = groups_to_display[i];
    if (group->script == script)
    {
      gtk_label_set_text(GTK_LABEL(group->label), "");
      gtk_widget_destroy(groups_to_display[i]->widget);
      if (group->indexAncien != NULL) destroy_tab_2(group->indexAncien, group->rows);
      group->indexAncien = NULL;
      if (group->indexDernier != NULL) destroy_tab_2(group->indexDernier, group->rows);
      group->indexDernier = NULL;
      if (group->tabValues != NULL) destroy_tab_4(group->tabValues, group->columns, group->rows);
      group->tabValues = NULL;

      number_of_groups_to_display--;
      groups_to_display[i] = groups_to_display[number_of_groups_to_display];
      i--;
    }
  }

  for (i = 0; i < number_of_net_links; i++)
  {
    if (net_links[i].previous)
    {
      if (net_links[i].previous->script == script)
      {
        net_links[i].previous = NULL;
      }
    }
    else if (net_links[i].next)
    {
      if (net_links[i].next->script == script)
      {
        net_links[i].next = NULL;
      }
    }

    if (!net_links[i].previous && !net_links[i].next)
    {
      number_of_net_links--;
      net_links[i] = net_links[number_of_net_links];

      i--; /* il faudra tester le lien qui a pris la place */
    }
  }

  for (i = 0; i < script->number_of_groups; i++)
  {

    for (j = 0; j < script->groups[i].number_of_neurons; j++)
    {
      if (script->groups[i].param_neuro_pandora[j].links_ok)
      {
        free(script->groups[i].param_neuro_pandora[j].coeff);
        free(script->groups[i].param_neuro_pandora[j].links_to_draw);
      }
    }

    free(script->groups[i].neurons);
    free(script->groups[i].param_neuro_pandora);
  }
  free(script->groups);

  gtk_widget_destroy(script->widget);

  number_of_scripts--;
  for (i = script->id; i < number_of_scripts; i++)
  {
    scripts[i] = scripts[i + 1];
    scripts[i]->id = i;
    scripts[i]->color = i % COLOR_MAX;
  }
  // scripts[script->id]->z = script->id;

  // mise à jour des coordonnees des scripts

  for (i = script->id; i < number_of_scripts; i++)
  {
    script_update_positions(scripts[i]);
    script_widget_update(scripts[i]);
  }

  gtk_widget_queue_draw(architecture_display);
}

/**
 * Cette fonction permet de traiter les informations provenant de promethe pour mettre à jour
 * l'affichage. Elle est appelee automatiquement plusieurs fois par seconde.
 */
gboolean neurons_refresh_display()
{
//int i;
  if (refresh_mode != REFRESH_MODE_AUTO) return FALSE;

  pthread_mutex_lock(&mutex_script_caracteristics);
  gtk_widget_queue_draw(GTK_WIDGET(zone_neurons));
  pthread_mutex_unlock(&mutex_script_caracteristics);
  return TRUE;
}

void fatal_error(const char *name_of_file, const char *name_of_function, int numero_of_line, const char *message, ...)
{
  char total_message[MESSAGE_MAX];
  GtkWidget *dialog;
  va_list arguments;
  va_start(arguments, message);
  vsnprintf(total_message, MESSAGE_MAX, message, arguments);
  va_end(arguments);
  dialog =
      gtk_message_dialog_new(GTK_WINDOW(window), (window == NULL ? 0 : GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s \t %s \t %i :\n \t Error: %s \n", name_of_file, name_of_function, numero_of_line, total_message);
  gtk_dialog_run(GTK_DIALOG(dialog));

  if (window != NULL)
  {
    gtk_widget_destroy(window);
    gtk_main_quit();
  }
  else gtk_widget_destroy(dialog);

  exit(EXIT_FAILURE);
}

void refresh_mode_combo_box_value_changed(GtkComboBox *comboBox, gpointer data)
{
  int i;
  refresh_mode = gtk_combo_box_get_active(comboBox);

  switch (refresh_mode)
  {
  case REFRESH_MODE_MANUAL:
    if (refresh_timer_id != 0)
    {
      g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
      refresh_timer_id = 0;
    }
    /*
     if(id_manual!=0)
     {g_source_destroy(g_main_context_find_source_by_id(NULL, id_manual)); id_manual = 0;}

     if(id_manual==0) id_manual = g_timeout_add((guint) 50, neurons_refresh_display_without_change_values, NULL); //
     */
    for (i = 0; i < number_of_groups_to_display; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_STOP : PANDORA_SEND_NEURONS_STOP), groups_to_display[i]->id, groups_to_display[i]->script->name + strlen(bus_id) + 1);

    gtk_widget_show_all(GTK_WIDGET(data));
    break;

  case REFRESH_MODE_AUTO:
    if (refresh_timer_id != 0)
    {
      g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
      refresh_timer_id = 0;
    }

    //if (refresh_timer_id == 0 && value_hz > 0.1) refresh_timer_id = g_timeout_add((guint) (1000.0 / value_hz), neurons_refresh_display, NULL);
    //else if (refresh_timer_id == 0) refresh_timer_id = 0;
    /*if(id_manual!=0)
     {g_source_destroy(g_main_context_find_source_by_id(NULL, id_manual)); id_manual = 0;}
     */

    for (i = 0; i < number_of_groups_to_display; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_START : PANDORA_SEND_NEURONS_START), groups_to_display[i]->id, groups_to_display[i]->script->name + strlen(bus_id) + 1);

    gtk_widget_hide(GTK_WIDGET(data));
    break;
  default:
    break;
  }

}

void format_combo_box_changed(GtkComboBox *comboBox, gpointer data)
{
  (void) data;
  format_mode = gtk_combo_box_get_active(comboBox);
}

void neurons_manual_refresh(GtkWidget *pWidget, gpointer pdata)
{
  int i;

  (void) pWidget;
  (void) pdata;
  if (refresh_mode == REFRESH_MODE_MANUAL)
  {
    for (i = 0; i < number_of_groups_to_display; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_ONE : PANDORA_SEND_NEURONS_ONE), groups_to_display[i]->id, groups_to_display[i]->script->name + strlen(bus_id) + 1);

  }
}

gboolean neurons_refresh_display_without_change_values()
{
  int i, current_stop = stop;
  if (refresh_mode != REFRESH_MODE_MANUAL) return FALSE;
  pthread_mutex_lock(&mutex_script_caracteristics);
  stop = TRUE;
  for (i = 0; i < number_of_groups_to_display; i++)
  {
    groups_to_display[i]->refresh_freq = FALSE;
    gtk_widget_queue_draw(GTK_WIDGET(groups_to_display[i]->drawing_area));
    //group_expose_neurons(groups_to_display[i], TRUE, FALSE);
  }
  stop = current_stop;
  pthread_mutex_unlock(&mutex_script_caracteristics);
  return TRUE;
}

void call_matlab(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
  gchar *name;
  char exec[MAX_LENGHT_PATHNAME] = "";
  FILE* link = NULL;

  (void) path;
  (void) userdata;

  gtk_tree_model_get(model, iter, 0, &name, -1);
  strcat(exec, "python ");
  strcat(exec, matlab_path);
  strcat(exec, " ");
  strcat(exec, name);
  link = popen(exec, "r");
  (void) link;

}

void call_python(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
  gchar *name;
  char exec[MAX_LENGHT_PATHNAME] = "";
  FILE* link = NULL;

  (void) path;
  (void) userdata;

  gtk_tree_model_get(model, iter, 0, &name, -1);
  strcat(exec, "python ");
  strcat(exec, python_path);
  strcat(exec, " ");
  strcat(exec, name);
  link = popen(exec, "r");
  (void) link;

}

void call_python_inter(GtkWidget *python_window, int arg, gpointer pdata)
{
  GtkWidget* text_entry = NULL;
  GtkTreeIter iter;
  gchar entry[MAX_LENGHT_FILENAME];

  // (void) python_window;
  if (arg == GTK_RESPONSE_OK) gtk_tree_selection_selected_foreach(GTK_TREE_SELECTION(pdata), call_python, NULL);
  if (arg == GTK_RESPONSE_DELETE_EVENT) gtk_widget_destroy(python_window);
  if (arg == GTK_RESPONSE_ACCEPT)
  {
    text_entry = g_object_get_data(G_OBJECT(python_window), "text_entry");
    strcpy(entry, gtk_entry_get_text(GTK_ENTRY(text_entry)));
    if (strcmp(entry, "") != 0)
    {
      gtk_list_store_append(currently_saving_list, &iter);
      gtk_list_store_set(currently_saving_list, &iter, 0, entry, -1);
    }
  }
}
void call_matlab_inter(GtkWidget *python_window, int arg, gpointer pdata)
{
  GtkWidget* text_entry = NULL;
  GtkTreeIter iter;
  gchar entry[MAX_LENGHT_FILENAME];

  (void) python_window;
  if (arg == GTK_RESPONSE_OK) gtk_tree_selection_selected_foreach(GTK_TREE_SELECTION(pdata), call_matlab, NULL);
  if (arg == GTK_RESPONSE_DELETE_EVENT) gtk_widget_destroy(python_window);
  if (arg == GTK_RESPONSE_ACCEPT)
  {
    text_entry = g_object_get_data(G_OBJECT(python_window), "text_entry");
    strcpy(entry, gtk_entry_get_text(GTK_ENTRY(text_entry)));
    if (strcmp(entry, "") != 0)
    {
      gtk_list_store_append(currently_saving_list, &iter);
      gtk_list_store_set(currently_saving_list, &iter, 0, entry, -1);
    }
  }
}

void on_click_call_dialog(GtkWidget *pWidget, gpointer pdata)
{

  GtkWidget *path_list;
  GtkCellRenderer *render_text_column;
  GtkTreeViewColumn *text_column;
  GtkWidget *python_window, *pScrollbar;
  GtkTreeSelection *selection;
  GtkWidget *text_entry, *vbox, *content_area;
  long ID;

  (void) pWidget;

  //Attention, ceci est une astuce pour passer simplement un argument 0 ou 1, à un risque d'echec en cas de changement d'architecture.
  ID = (long) pdata;

  python_window = NULL;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
  path_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(currently_saving_list));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(path_list));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

  render_text_column = gtk_cell_renderer_text_new();
  text_column = gtk_tree_view_column_new_with_attributes("Saved box", render_text_column, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(path_list), text_column);

  if (ID == 0) python_window = gtk_dialog_new_with_buttons("Python Call", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, "Call python script on selected", GTK_RESPONSE_OK, "add manually", GTK_RESPONSE_ACCEPT, NULL);
  if (ID == 1) python_window = gtk_dialog_new_with_buttons("Matlab conversion", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, "Convert selected", GTK_RESPONSE_OK, "add manually", GTK_RESPONSE_ACCEPT, NULL);

  gtk_window_set_default_size(GTK_WINDOW(python_window), 300, 200);

  if (ID == 0) g_signal_connect(python_window, "response", G_CALLBACK(call_python_inter), selection);
  if (ID == 1) g_signal_connect(python_window, "response", G_CALLBACK(call_matlab_inter), selection);

  g_signal_connect(G_OBJECT(python_window), "destroy", G_CALLBACK(on_destroy), NULL);

  pScrollbar = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(pScrollbar), 150);
  gtk_widget_set_size_request(pScrollbar, -1, 100);
  gtk_container_add(GTK_CONTAINER(pScrollbar), path_list);

  gtk_box_pack_start(GTK_BOX(vbox), pScrollbar, TRUE, TRUE, 1);

  text_entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(text_entry), MAX_LENGHT_FILENAME);
  g_object_set_data(G_OBJECT(python_window), "text_entry", (gpointer) text_entry);
  gtk_box_pack_start(GTK_BOX(vbox), text_entry, FALSE, FALSE, 1);
  // gtk_box_pack_start(GTK_BOX(GTK_DIALOG(python_window)->vbox),pScrollbar,TRUE,TRUE,1);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(python_window));
  gtk_container_add(GTK_CONTAINER(content_area), vbox);

  gtk_widget_show_all(python_window);

}

void on_click_config(GtkWidget *button, gpointer pdata)
{
  GtkWidget *selection;
  char home[PATH_MAX];

  (void) pdata;
  (void) button;

  sprintf(home, "%s/bin_leto_prom/simulator/pandora/save", getenv("HOME"));
  printf("home = %s\n", home);
  selection = gtk_file_chooser_dialog_new("selectionner un repertoire", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(selection), (gchar*) home);

  gtk_widget_show(selection);

  if (gtk_dialog_run(GTK_DIALOG(selection)) == GTK_RESPONSE_OK)
  {
    char *path;
    path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(selection));
    strcpy(python_path, path);
    g_free(path);
  }
  gtk_widget_destroy(selection);
}

void on_click_extract_area(GtkWidget *button, gpointer pdata)
{
  static GtkWidget *pWindow;
  static GtkWidget *pframe_extract;
  int i;
  //GtkWidget *v_box_second;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
  {
    pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    pframe_extract = (GtkWidget*) pdata;

    gtk_widget_set_double_buffered(pWindow, TRUE);
    g_object_set_data(G_OBJECT(pWindow), "toggle_button", (gpointer) button);

    g_signal_connect(G_OBJECT(pWindow), "destroy", G_CALLBACK(on_destroy_new_window), pframe_extract);
    gtk_window_set_default_size(GTK_WINDOW(pWindow), 800, 800);

    gtk_widget_reparent(pframe_extract, GTK_WIDGET(pWindow));

    g_signal_connect(G_OBJECT(pWindow), "key_press_event", G_CALLBACK(key_press_event), NULL);

    gtk_widget_show_all(pWindow);

    for (i = 0; i < number_of_groups_to_display; i++)
    {
      resize_group(groups_to_display[i]);
    }

  }
  else
  {
    gtk_widget_destroy(pWindow);
    // on_destroy_new_window(pWindow, pframe_extract);
    pWindow = NULL;
    pframe_extract = NULL;
  }

}

gboolean on_resize_neuron_frame(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
  GtkWidget *separator = (GtkWidget*) user_data;
  (void) widget;
  (void) allocation;
  gtk_widget_set_size_request(separator, gtk_widget_get_allocated_width(neurons_frame) - 20, -1);
  gtk_widget_set_hexpand(separator, TRUE);
  return FALSE;
}
/** Fonction creant la fenetre GTK principale **/
void pandora_window_new()
{
  char path[PATH_MAX];
  char path_css[PATH_MAX];
  char path_fleche[PATH_MAX];
  char path_separateur[PATH_MAX];
  GtkWidget *h_box_main, *v_box_main, *v_box_inter, *pFrameEchelles, *pVBoxEchelles, *hbox_buttons, *hbox_barre, *refreshModeHBox, *refreshModeComboBox, *refreshModeLabel, *refreshManualButton, *refreshSetting, *xSetting, *xLabel, *menuBar, *fileMenu,
      *legend, *com;
  GtkWidget *h_box_save, *h_box_global, *h_box_network;
  GtkWidget *ySetting, *yLabel, *zxSetting, *zxLabel, *pFrameGroupes, *zySetting, *zyLabel, *saveLabel, *globalLabel, *networkLabel;
  GtkWidget *boutonDefault, *boutonPause, *save_button, *boutonDebug, *boutonTempsPhases, *save_path_button, *hide_see_legend_button, *call_python_button, *convert_matlab_button, *config_button;
  GtkWidget *pPane, *lPane; //Panneaux lateraux
  GtkWidget *pFrameScripts, *scrollbars2, *textScrollbar;
  GtkWidget *load, *save, *saveAs, *quit, *itemFile;
  GtkWidget *arrow_im, *arrow_im_2;
  GtkWidget *notebook;
  GtkWidget *com_zone;
  GtkWidget *button_label;
  GtkWidget *neuron_label_widget, *neuron_label, *dedou_im, *button_draw_links_info;
  gchar *s = NULL;
  GtkWidget *image_fleche_froite1, *image_fleche_froite2, *image_fleche_froite3, *image_separateur;
  GtkCssProvider *css_provider = NULL;
  GdkScreen * screen;
  GdkDisplay *display;
  gboolean test = 0;

  //Methode tres particuliere, afin de ne pas se compliquer la tache lors du passage d'argument utilisant les deux expressions ci dessous, on utilise le fait de pouvoir passer une adresse pour passer un int 0 ou 1 sous le format adresse.
  void* IDdialog_python = (void*) 0;
  void* IDdialog_matlab = (void*) 1;

  (void) p_buf;

  sprintf(path_css, "%s/bin_leto_prom/simulator/pandora/resources/pandora.css", getenv("HOME"));

  currently_saving_list = gtk_list_store_new(1, G_TYPE_STRING);

//La fenetre principale

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_double_buffered(window, TRUE);
  /* Positionne la GTK_WINDOW "pWindow" au centre de l'ecran */
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  /* Taille de la fenetre */
  gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);

  css_provider = gtk_css_provider_new();
  display = gdk_display_get_default();
  screen = gdk_display_get_default_screen(display);

  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  test = gtk_css_provider_load_from_path(css_provider, path_css, NULL);

  //gtk_css_provider_load_from_path(css_provider, "/home/nilsbeau/simulateur_trunk/pandora/resources", &error);

  printf("Etat de chargement du css : %d\n", (int) test);

  window_title_update();
  sprintf(path, "%s/bin_leto_prom/resources/pandora_icon.png", getenv("HOME"));
  sprintf(path_fleche, "%s/bin_leto_prom/simulator/pandora/resources/image_fleche_droite.png", getenv("HOME"));
  sprintf(path_separateur, "%s/bin_leto_prom/simulator/pandora/resources/separateur2.png", getenv("HOME"));

  image_fleche_froite1 = gtk_image_new_from_file((gchar*) path_fleche);
  image_fleche_froite2 = gtk_image_new_from_file((gchar*) path_fleche);
  image_fleche_froite3 = gtk_image_new_from_file((gchar*) path_fleche);
  image_separateur = gtk_image_new_from_file((gchar*) path_separateur);

  gtk_window_set_icon_from_file(GTK_WINDOW(window), path, NULL);

//Le signal de fermeture de la fenetre est connecte à la fenetre (petite croix)
  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(window_close), (GtkWidget* ) window);

// creation de la barre de menus.
  menuBar = gtk_menu_bar_new(); //TODO : verif libe memoire
  fileMenu = gtk_menu_new(); //TODO : verif libe memoire

  neurons_frame = gtk_frame_new("");

  button_label = gtk_toggle_button_new();
  dedou_im = gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU);
  //g_object_set_data(G_OBJECT(button_label), "dedou_im", GTK_WIDGET(dedou_im));
  gtk_container_add(GTK_CONTAINER(button_label), dedou_im);

  neuron_label = gtk_label_new("Neurons' frame");
  //separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

  neuron_label_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(neuron_label_widget), FALSE);

  gtk_box_pack_start(GTK_BOX(neuron_label_widget), neuron_label, FALSE, FALSE, 0);
  // gtk_box_pack_start(GTK_BOX(neuron_label_widget),separator,FALSE,FALSE,0);
  gtk_box_pack_end(GTK_BOX(neuron_label_widget), button_label, FALSE, FALSE, 0);
  gtk_frame_set_label_widget(GTK_FRAME(neurons_frame), neuron_label_widget);
  //gtk_widget_set_hexpand (gtk_frame_get_label_widget(GTK_FRAME(neurons_frame)),TRUE);
  //gtk_widget_set_hexpand (neurons_frame,TRUE);
  //gtk_widget_set_hexpand (separator,TRUE);
  //gtk_widget_set_halign (gtk_frame_get_label_widget(GTK_FRAME(neurons_frame)),GTK_ALIGN_FILL);
  //gtk_widget_set_halign (neurons_frame,GTK_ALIGN_FILL);

  s = g_locale_to_utf8("Load", -1, NULL, NULL, NULL);
  load = gtk_menu_item_new_with_label(s);
  g_free(s);

  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), load);
  g_signal_connect(G_OBJECT(load), "activate", G_CALLBACK(pandora_load_preferences), NULL);

  s = g_locale_to_utf8("Save", -1, NULL, NULL, NULL);
  save = gtk_menu_item_new_with_label(s);
  g_free(s);

  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), save);
  g_signal_connect(G_OBJECT(save), "activate", G_CALLBACK(save_preferences), NULL);

  s = g_locale_to_utf8("Save as", -1, NULL, NULL, NULL);
  saveAs = gtk_menu_item_new_with_label(s);
  g_free(s);

  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveAs);
  g_signal_connect(G_OBJECT(saveAs), "activate", G_CALLBACK(save_preferences_as), NULL);

  s = g_locale_to_utf8("Quit", -1, NULL, NULL, NULL);
  quit = gtk_menu_item_new_with_label(s);
  g_free(s);

  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quit);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(window_close), (GtkWidget* ) window);

  itemFile = gtk_menu_item_new_with_label("File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemFile), fileMenu);

  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemFile);

  /*Creation d'une VBox (boete de widgets disposes verticalement) */
  v_box_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /*ajout de v_box_main dans pWindow, qui est alors vu comme un GTK_CONTAINER*/
  gtk_container_add(GTK_CONTAINER(window), v_box_main);

  gtk_box_pack_start(GTK_BOX(v_box_main), menuBar, FALSE, FALSE, 0); // ajout de la barre de menus.

  // Creation du systemes d'onglets pour la barre d'outils
  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

  h_box_save = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  //gtk_box_set_homogeneous(GTK_BOX(h_box_save), TRUE);
  h_box_global = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(h_box_global), TRUE);
  h_box_network = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(h_box_network), TRUE);

  globalLabel = gtk_label_new("General");
  saveLabel = gtk_label_new("Saving");
  networkLabel = gtk_label_new("Network");

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), h_box_global, globalLabel);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), h_box_save, saveLabel);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), h_box_network, networkLabel);

  gtk_box_pack_start(GTK_BOX(v_box_main), notebook, FALSE, FALSE, 0);

  //Creation du bouton de changement de chemin
  save_path_button = gtk_button_new_with_label("Define save path");
  g_signal_connect(G_OBJECT(save_path_button), "clicked", G_CALLBACK(on_click_save_path_button), NULL);
  gtk_box_pack_start(GTK_BOX(h_box_save), save_path_button, TRUE, FALSE, 0);

  //fleche
  gtk_box_pack_start(GTK_BOX(h_box_save), image_fleche_froite1, FALSE, FALSE, 0);

  //Creation de l'onglet format pandora promethe
  format_combobox = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combobox), "Format Pandora");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combobox), "Format Promethee");
  gtk_combo_box_set_active(GTK_COMBO_BOX(format_combobox), 0);
  gtk_box_pack_start(GTK_BOX(h_box_save), format_combobox, TRUE, FALSE, 0);
  g_signal_connect(G_OBJECT(format_combobox), "changed", (GCallback ) format_combo_box_changed, NULL);

  //fleche
  gtk_box_pack_start(GTK_BOX(h_box_save), image_fleche_froite2, FALSE, FALSE, 0);

  //Creation du bouton d'enregistrement
  save_button = gtk_toggle_button_new_with_label("Launch data saving");
  g_signal_connect(G_OBJECT(save_button), "toggled", G_CALLBACK(on_toggled_saving_button), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_button), FALSE);
  gtk_box_pack_start(GTK_BOX(h_box_save), save_button, TRUE, FALSE, 0);

  //gtk_box_pack_start(GTK_BOX(h_box_save),separator, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h_box_save), image_separateur, FALSE, FALSE, 0);
  //Creation du bouton de conversion matlab
  convert_matlab_button = gtk_button_new_with_label("Convert to Matlab");
  g_signal_connect(G_OBJECT(convert_matlab_button), "clicked", G_CALLBACK(on_click_call_dialog), (gpointer ) IDdialog_matlab);
  gtk_box_pack_start(GTK_BOX(h_box_save), convert_matlab_button, TRUE, FALSE, 0);

  //Creation du bouton de config
  config_button = gtk_button_new_with_label("Define script path");
  g_signal_connect(G_OBJECT(config_button), "clicked", G_CALLBACK(on_click_config), NULL);
  gtk_box_pack_start(GTK_BOX(h_box_save), config_button, TRUE, FALSE, 0);

  //fleche
  gtk_box_pack_start(GTK_BOX(h_box_save), image_fleche_froite3, FALSE, FALSE, 0);

  //Creation du bouton d'appel python
  call_python_button = gtk_button_new_with_label("Call python script");
  g_signal_connect(G_OBJECT(call_python_button), "clicked", G_CALLBACK(on_click_call_dialog), (gpointer ) IDdialog_python);
  gtk_box_pack_start(GTK_BOX(h_box_save), call_python_button, TRUE, FALSE, 0);

  boutonTempsPhases = gtk_toggle_button_new_with_label("start calculate execution times");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(boutonTempsPhases), FALSE);
  gtk_box_pack_start(GTK_BOX(h_box_global), boutonTempsPhases, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(boutonTempsPhases), "toggled", G_CALLBACK(phases_info_start_or_stop), NULL);

  boutonDebug = gtk_toggle_button_new_with_label("Start Debug Group Functions");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(boutonDebug), FALSE);
  gtk_box_pack_start(GTK_BOX(h_box_global), boutonDebug, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(boutonDebug), "toggled", G_CALLBACK(debug_grp_mem_info_start_or_stop), NULL);

  button_draw_links_info = gtk_toggle_button_new_with_label("Draw links info");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_draw_links_info), FALSE);
  gtk_box_pack_start(GTK_BOX(h_box_global), button_draw_links_info, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button_draw_links_info), "toggled", G_CALLBACK(on_button_draw_links_info_pressed), NULL);

  //Creation du bouton de compression
  compress_button = gtk_toggle_button_new_with_label("Activate compression (need restart)");
  g_signal_connect(G_OBJECT(compress_button), "toggled", G_CALLBACK(on_toggled_compress_button), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compress_button), FALSE);
  gtk_box_pack_start(GTK_BOX(h_box_network), compress_button, FALSE, FALSE, 0);

  //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(windowed_area_button), FALSE);
  // gtk_box_pack_start(GTK_BOX(h_box_global), windowed_area_button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button_label), "toggled", G_CALLBACK(on_click_extract_area), (gpointer ) neurons_frame);

  /*Création de deux HBox : une pour le panneau latéral et la zone principale, l'autre pour les 6 petites zones*/
  h_box_main = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_position(GTK_PANED(vpaned), 600);
  gtk_box_pack_start(GTK_BOX(v_box_main), h_box_main, TRUE, TRUE, 0);

  /*Panneau latéral*/
  pPane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  lPane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  gtk_box_pack_start(GTK_BOX(h_box_main), pPane, FALSE, TRUE, 0);

//Les échelles
  pFrameEchelles = gtk_frame_new("Scales");
  gtk_container_add(GTK_CONTAINER(pPane), pFrameEchelles);
  pVBoxEchelles = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  gtk_container_add(GTK_CONTAINER(pFrameEchelles), pVBoxEchelles);

  // Mode de reactualisation des groupes de neurones contenus dans le panneau du bas.
  refreshModeHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshModeHBox, FALSE, TRUE, 0);
  refreshModeLabel = gtk_label_new("Refresh mode : ");
  refreshModeComboBox = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refreshModeComboBox), "Auto");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refreshModeComboBox), "Manual");
  gtk_combo_box_set_active(GTK_COMBO_BOX(refreshModeComboBox), REFRESH_MODE_AUTO);
  gtk_box_pack_start(GTK_BOX(refreshModeHBox), refreshModeLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(refreshModeHBox), refreshModeComboBox, TRUE, TRUE, 0);

  refreshManualButton = gtk_button_new_with_label("Refresh");
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshManualButton, FALSE, TRUE, 0);

  //g_signal_connect(G_OBJECT(refreshManualButton), "realize", (GCallback) gtk_widget_hide, NULL); // faeon un peu orginale de cacher le widget à l'initialisation.

//Frequence de reactualisation de l'affichage, quand on est en mode echantillonne (Sampled)
  refreshSetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshSetting, FALSE, TRUE, 0);

  g_signal_connect(G_OBJECT(refreshModeComboBox), "changed", (GCallback ) refresh_mode_combo_box_value_changed, refreshManualButton);
  g_signal_connect(G_OBJECT(refreshManualButton), "clicked", (GCallback ) neurons_manual_refresh, NULL);

//Echelle de l'axe des x
  xSetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), xSetting, FALSE, TRUE, 0);
  xLabel = gtk_label_new("x scale:");
  xScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, XSCALE_MIN, XSCALE_MAX, 1); //Ce widget est deje declare comme variable globale
  gtk_box_pack_start(GTK_BOX(xSetting), xLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(xSetting), xScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(xScale), graphic.x_scale);
  g_signal_connect(G_OBJECT(xScale), "change-value", (GCallback ) on_scale_change_value, &graphic.x_scale);

//Echelle de l'axe des y
  ySetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), ySetting, FALSE, TRUE, 0);
  yLabel = gtk_label_new("y scale:");
  yScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, YSCALE_MIN, YSCALE_MAX, 1); //Ce widget est deje declare comme variable globale
  gtk_box_pack_start(GTK_BOX(ySetting), yLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(ySetting), yScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(yScale), graphic.y_scale);
  g_signal_connect(G_OBJECT(yScale), "change-value", (GCallback ) on_scale_change_value, &graphic.y_scale);

//Decalage des plans selon x
  zxSetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zxSetting, FALSE, TRUE, 0);
  zxLabel = gtk_label_new("x gap:");
  zxScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 200, 1); //Ce widget est deje declare comme variable globale
  gtk_box_pack_start(GTK_BOX(zxSetting), zxLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(zxSetting), zxScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(zxScale), graphic.zx_scale);
  g_signal_connect(G_OBJECT(zxScale), "change-value", (GCallback ) on_scale_change_value, &graphic.zx_scale);

//Decalage des plans selon y
  zySetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zySetting, FALSE, TRUE, 0);
  zyLabel = gtk_label_new("y gap:");
  zyScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 2000, 1); //Ce widget est deje declare comme variable globale
  gtk_box_pack_start(GTK_BOX(zySetting), zyLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(zySetting), zyScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(zyScale), graphic.zy_scale);
  g_signal_connect(G_OBJECT(zyScale), "change-value", (GCallback ) on_scale_change_value, &graphic.zy_scale);

  hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(hbox_buttons), TRUE);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), hbox_buttons, FALSE, TRUE, 12);

  boutonPause = gtk_toggle_button_new_with_label("Stop graphs");
  gtk_box_pack_start(GTK_BOX(hbox_buttons), boutonPause, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonPause), "toggled", G_CALLBACK(graph_stop_or_not), NULL);

  boutonDefault = gtk_button_new_with_label("Default scales");
  gtk_box_pack_start(GTK_BOX(hbox_buttons), boutonDefault, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonDefault), "clicked", G_CALLBACK(defaultScale), NULL);

  check_button_draw_connections = gtk_check_button_new_with_label("draw connections");
  check_button_draw_net_connections = gtk_check_button_new_with_label("draw net connections");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_draw_connections), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_draw_net_connections), TRUE);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), check_button_draw_connections, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), check_button_draw_net_connections, FALSE, TRUE, 0);
  g_signal_connect(G_OBJECT(check_button_draw_connections), "toggled", G_CALLBACK(on_check_button_draw_active), NULL);
  g_signal_connect(G_OBJECT(check_button_draw_net_connections), "toggled", G_CALLBACK(on_check_button_draw_active), NULL);

  graphic.draw_links = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_connections));
  graphic.draw_net_links = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_net_connections));

  displayMode = "Sampled mode";

  modeLabel = gtk_label_new(displayMode);
  gtk_container_add(GTK_CONTAINER(pPane), modeLabel);

//Les scripts
  pFrameScripts = gtk_frame_new("Open scripts");
  gtk_container_add(GTK_CONTAINER(pPane), pFrameScripts);
  pVBoxScripts = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  ;
  gtk_container_add(GTK_CONTAINER(pFrameScripts), pVBoxScripts);

//La zone principale

  //ceation d'une Vbox qui contiendra le paned de la zone affichage et les boutons d'ouverture des cotes
  v_box_inter = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

  //Creation d'une Hbox qui contiendra les boutons suivants
  hbox_barre = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

  // Creation des deux boutons de reduction des panneaux gauche et droits.
  hide_see_scales_button = gtk_toggle_button_new();
  arrow_im = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU);
  g_object_set_data(G_OBJECT(hide_see_scales_button), "arrow_im", GTK_WIDGET(arrow_im));
  gtk_container_add(GTK_CONTAINER(hide_see_scales_button), arrow_im);
  g_signal_connect(G_OBJECT(hide_see_scales_button), "toggled", G_CALLBACK(on_hide_see_scales_button_active), pPane);
  gtk_box_pack_start(GTK_BOX(hbox_barre), hide_see_scales_button, FALSE, TRUE, 0);

  hide_see_legend_button = gtk_toggle_button_new();
  arrow_im_2 = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
  g_object_set_data(G_OBJECT(hide_see_legend_button), "arrow_im_2", arrow_im_2);
  gtk_container_add(GTK_CONTAINER(hide_see_legend_button), arrow_im_2);
  g_signal_connect(G_OBJECT(hide_see_legend_button), "toggled", G_CALLBACK(on_hide_see_legend_button_active), lPane);

  gtk_box_pack_end(GTK_BOX(hbox_barre), hide_see_legend_button, FALSE, TRUE, 0);

  // on insere la Hbox dans la vbox vpaned
  gtk_box_pack_start(GTK_BOX(v_box_inter), hbox_barre, FALSE, FALSE, 0);
  //gtk_container_add(GTK_CONTAINER(vpaned), hbox_barre);

  pFrameGroupes = gtk_frame_new("Neural groups");
  gtk_container_add(GTK_CONTAINER(vpaned), pFrameGroupes);
  scrollbars = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(pFrameGroupes), scrollbars);
  architecture_display = gtk_drawing_area_new();
  gtk_widget_set_size_request(architecture_display, 10000, 10000);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars), architecture_display);

  scrollbars2 = gtk_scrolled_window_new(NULL, NULL);

  gtk_widget_set_events(architecture_display, GDK_SCROLL_MASK | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK); //TODO : remplacer par add , Detecte quand on appuie OU quand on relache un bouton de la souris alors que le curseur est dans la zone3D
  g_signal_connect(G_OBJECT(architecture_display), "scroll-event", G_CALLBACK(architecture_display_scroll_event), NULL);
  g_signal_connect(G_OBJECT(architecture_display), "draw", G_CALLBACK(architecture_display_update), NULL);
  g_signal_connect(G_OBJECT(architecture_display), "button-release-event", G_CALLBACK(architecture_display_button_released), NULL);
  g_signal_connect(G_OBJECT(architecture_display), "motion-notify-event", G_CALLBACK(architecture_display_drag_motion), NULL);
  g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_event), NULL);
  g_signal_connect(G_OBJECT(architecture_display), "button-press-event", G_CALLBACK(architecture_display_button_pressed), (gpointer ) scrollbars2);
//la zone des groupes de neurones
  gtk_container_add(GTK_CONTAINER(vpaned), neurons_frame);
  //g_signal_connect(G_OBJECT(neurons_frame), "configure-event", G_CALLBACK(on_resize_neuron_frame), (gpointer)separator);
  //gtk_widget_add_events(v_box_inter,GDK_STRUCTURE_MASK);
  // g_signal_connect(G_OBJECT(neurons_frame), "size-allocate", G_CALLBACK(on_resize_neuron_frame), (gpointer)separator);
  gtk_container_add(GTK_CONTAINER(neurons_frame), scrollbars2);

  zone_neurons = gtk_layout_new(NULL, NULL);
  gtk_widget_add_events(zone_neurons, GDK_EXPOSURE_MASK);
  gtk_widget_set_size_request(GTK_WIDGET(zone_neurons), 3000, 3000);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars2), zone_neurons);
  g_signal_connect(G_OBJECT(zone_neurons), "draw", G_CALLBACK(draw_all_links), NULL);
  gtk_widget_set_events(zone_neurons, GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(G_OBJECT(zone_neurons), "button-release-event", G_CALLBACK(on_release), NULL);

  gtk_box_pack_start(GTK_BOX(v_box_inter), vpaned, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(h_box_main), v_box_inter, TRUE, TRUE, 0);

  /*Panneau lateral droit*/

  legend = gtk_frame_new("Legends");
  com = gtk_frame_new("Prompt");
  com_zone = gtk_text_view_new();
  p_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(com_zone));
  textScrollbar = gtk_scrolled_window_new(NULL, NULL);

  gtk_widget_set_size_request(GTK_WIDGET(textScrollbar), 240, 600);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(com_zone), FALSE);

  gtk_container_add(GTK_CONTAINER(textScrollbar), com_zone);
  gtk_container_add(GTK_CONTAINER(com), textScrollbar);
  gtk_box_pack_start(GTK_BOX(lPane), legend, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(lPane), com, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h_box_main), lPane, FALSE, TRUE, 0);

  gtk_widget_show_all(window); //Affichage du widget pWindow et de tous ceux qui sont dedans
  gtk_widget_hide(refreshManualButton);
  // On cache la fenetre au depart
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_see_legend_button), TRUE);
  on_hide_see_legend_button_active(hide_see_legend_button, (gpointer*) lPane);
  //gtk_window_set_default_size(GTK_WINDOW(gtk_widget_get_window(separator)),gtk_widget_get_allocated_width(neurons_frame)-10,-1);
  // gtk_widget_set_size_request(separator,gtk_widget_get_allocated_width(neurons_frame)-5,-1);
  //gtk_widget_hide(lPane);
  //gtk_widget_hide(pPane);
}

/**
 *
 * name: Programme Principal (Main)
 *
 * @param argv, argc
 * @see enet_initialize()
 * @see server_for_promethes()
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv)
{
  int option;
  struct sigaction action;
  char chemin[] = CHEMIN;
  struct timeval time_stamp;
  //GtkSettings *default_settings = gtk_settings_get_default();
  // g_object_set(default_settings, "gtk-button-images", TRUE, NULL);

  setlocale(LC_ALL, "C");
  stop = FALSE;
  load_temporary_save = FALSE;
  architecture_display_dragging_currently = FALSE;
  refresh_mode = REFRESH_MODE_AUTO;
  graphic.x_scale = XSCALE_DEFAULT;
  graphic.y_scale = YSCALE_DEFAULT;
  graphic.zx_scale = XGAP_DEFAULT;
  graphic.zy_scale = YGAP_DEFAULT;

  if (sem_init(&(enet_pandora_lock), 1, 1) != 0) PRINT_WARNING("Critical error ! Fail to init semaphore !");

  //definition de pandora_quit comme la fonction de fermeture du programme par defaut.
  atexit(pandora_quit);

  sigfillset(&action.sa_mask);
  action.sa_handler = on_signal_interupt;
  action.sa_flags = 0;
  if (sigaction(SIGINT, &action, NULL) != 0)
  {
    printf("Signal SIGINT not catched.");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGSEGV, &action, NULL) != 0)
  {
    printf("Signal SIGSEGV not catched.");
    exit(EXIT_FAILURE);
  }

  //Preparation pour le lancement de la lecture des arguments
  bus_ip[0] = 0;
  bus_id[0] = 0;
  preferences_filename[0] = 0;

  //On regarde les options passees en ligne de commande
  while ((option = getopt(argc, argv, "i:b:")) != -1)
  {
    switch (option)
    {
    // -i "bus_id"
    case 'i':
      strncpy(bus_id, optarg, BUS_ID_MAX);
      break;

    case 'b':
      strncpy(bus_ip, optarg, HOST_NAME_MAX);
      break;
      // autres options : erreur
    default:
      printf("\tUsage: %s [-b broadcast -i bus_id] \n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  //On regarde les fichier passes en ligne de commande
  if (optind < argc)
  {
    if (strstr(argv[optind], ".pandora"))
    {
      strcpy(preferences_filename, argv[optind]);
    }
  }

//g_thread_init(NULL); /* useless since glib 2.32 */
  gdk_threads_init();

  pthread_mutex_init(&mutex_script_caracteristics, NULL);
  pthread_mutex_init(&mutex_loading, NULL);

// Initialisation de GTK+
  gtk_init(&argc, &argv);

//Initialisation d'ENet
  if (enet_initialize() != 0)
  {
    printf("An error occurred while initializing ENet.\n");
    exit(EXIT_FAILURE);
  }

//Creation de la fenetre GTK principale, disposition des boutons etc...

  pandora_window_new();

//si apres chargement il n'y a pas de bus_id
  if ((bus_id[0] == 0) || (bus_ip[0] == 0))
  {
    EXIT_ON_ERROR("You miss bus_ip or bus_id \n\tUsage: %s [-b bus_ip -i bus_id] \n", argv[0]);
  }

  if (access(preferences_filename, R_OK) == 0)
  {
    pandora_file_load(preferences_filename);
    load_temporary_save = FALSE;
  }
  else if (access(chemin, R_OK) == 0)
  {
    pandora_file_load(chemin);
    load_temporary_save = TRUE;
  }

  prom_bus_init(bus_ip);

  //Lancement du thread d'ecoute et de gestion des informations reeues du reseau
  server_for_promethes();

//Appelle la fonction de raffraichissement, voir la fonction lie à l'event refresh_mode_combo_box_changed pour plus de details
  refresh_mode = REFRESH_MODE_AUTO;
  if (refresh_timer_id != 0)
  {
    g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
    refresh_timer_id = 0;
  }
  if (refresh_timer_id == 0 && value_hz > 0.1) refresh_timer_id = g_timeout_add((guint) (1000.0 / value_hz), neurons_refresh_display, NULL);
  else if (refresh_timer_id == 0) refresh_timer_id = 0;

  gettimeofday(&time_stamp, NULL);

  start_time = time_stamp.tv_sec + (double) time_stamp.tv_usec / 1000000.;
  printf("start time %lf\n", start_time);
  gdk_threads_enter();
  gtk_main(); //Boucle infinie : attente des evenements
  gdk_threads_leave();
  return EXIT_SUCCESS;
}
