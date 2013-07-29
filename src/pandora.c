/** pandora.c
 *
 *
 * Auteurs : Brice Errandonea et Manuel De Palma
 *
 * Pour compiler : make
 *
 *
 **/
#include "pandora.h"
#include "pandora_ivy.h"
#include "pandora_graphic.h"
#include "pandora_save.h"
#include "pandora_prompt.h"

/** Variables globales (C'est mal !) ***/

char label_text[LABEL_MAX];

GtkWidget *window = NULL; //La fenêtre de l'application Pandora
GtkWidget *selected_group_dialog, *selected_image_combo_box;
GtkWidget *vpaned, *scrollbars;
GtkWidget *hide_see_scales_button = NULL; //Boutton permettant de cacher le menu de droite
GtkWidget *check_button_draw_connections, *check_button_draw_net_connections;
GtkWidget *pVBoxScripts = NULL; //Panneau des scripts
GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale; //Échelles
GtkBuilder *builder = NULL;
GtkWidget *architecture_display = NULL; //La grande zone de dessin des liaisons entre groupes

/*Indiquent quel est le mode d'affichage en cours (Off-line, Sampled ou Snapshots)*/
const char *displayMode = NULL;
GtkWidget *modeLabel = NULL;
int currentSnapshot = 0;
int nbSnapshots = 0;

int Index[NB_SCRIPTS_MAX]; /*Tableau des indices : Index[0] = 0, Index[1] = 1, ..., Index[NB_SCRIPTS_MAX-1] = NB_SCRIPTS_MAX-1;
 Ce tableau permet à une fonction signal de retenir la valeur qu'avait i au moment où on a connecté le signal*/

int number_of_scripts = 0; //Nombre de scripts à afficher
type_script *scripts[NB_SCRIPTS_MAX]; //Tableau des scripts à afficher
int zMax; //la plus grande valeur de z parmi les scripts ouverts
int buffered = 0; //Nombre d'instantanés actuellement en mémoire
int period = 0;

type_group *selected_group = NULL; //Pointeur sur le groupe actuellement sélectionné

GtkWidget *neurons_frame, *zone_neurons; //Panneau des neurones

int nbColonnesTotal = 0; //Nombre total de colonnes de neurones dans les fenêtres du bandeau du bas
int nbLignesMax = 0; //Nombre maximal de lignes de neurones à afficher dans l'une des fenêtres du bandeau du bas

gdouble move_neurons_old_x, move_neurons_old_y, value_hz = 50;
gboolean move_neurons_start = FALSE;
gboolean open_neurons_start = FALSE; //Pour ouvrir un frame_neuron
type_group *move_neurons_group = NULL; //Pour bouger un frame_neuron
type_group *open_group = NULL;

guint refresh_timer_id = 0; //id du timer actualement utiliser pour le rafraichissement des frame_neurons ouvertes
guint id_semi_automatic = 0;

type_script_link net_links[SCRIPT_LINKS_MAX];
int number_of_net_links = 0;
type_group *groups_to_display[NB_WINDOWS_MAX];
int number_of_groups_to_display = 0;

char bus_id[BUS_ID_MAX];
char bus_ip[HOST_NAME_MAX];

gboolean calculate_executions_times = FALSE;

gboolean load_temporary_save;
char preferences_filename[PATH_MAX]; //fichier de préférences (*.jap)
int stop; // continue l'enregistrement pour le graphe ou non.

//Pour la sauvegarde
gboolean saving_press = 0;
char path_named[MAX_LENGHT_PATHNAME] = "save/";
char python_path[MAX_LENGHT_PATHNAME] = "$HOME/simulateur/japet/save/default_script.py";
char matlab_path[MAX_LENGHT_PATHNAME] = "$HOME/simulateur/japet/save/convert_matlab.py";
GtkListStore* currently_saving_list;

pthread_t new_window_thread;
new_group_argument arguments;
pthread_mutex_t mutex_loading = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond_loading=PTHREAD_COND_INITIALIZER;
    pthread_cond_t cond_copy_arg_top=PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex_copy_arg_top=PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond_copy_arg_group_display=PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex_copy_arg_group_display=PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_t mutex_script_caracteristics=PTHREAD_MUTEX_INITIALIZER;
    gboolean architecture_display_dragging_currently=0;
    gdouble architecture_display_cursor_x=0;
    gdouble architecture_display_cursor_y=0;
    gdouble new_x,
new_y, old_x, old_y;

extern GtkTextBuffer * p_buf;
extern prompt_lign prompt_buf[NB_LIGN_MAX];
int refresh_mode = 0;

/** Fonctions diverses **/

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
  pandora_file_save("./pandora.pandora"); //enregistrement de l'état actuel. cet état sera appliqué au prochain démarrage de pandora.
  pandora_bus_send_message(bus_id, "pandora(%d,0)", PANDORA_STOP);
  enet_deinitialize();
  destroy_saving_ref(scripts); //par securité pour la cloture des fichiers de sauvegarde si on quitte durant la sauvegarde.
  gtk_widget_destroy(window);
  pthread_cancel(enet_thread);
  gtk_main_quit();
}

void on_signal_interupt(int signal)
{
  switch (signal)
  {
  case SIGINT:
    exit(EXIT_SUCCESS);
    break;
  case SIGSEGV:
    printf("SEGFAULT\n");
    exit(EXIT_FAILURE);
    break;
  default:
    printf("signal %d capture par le gestionnaire mais non traite... revoir le gestinnaire ou le remplissage de sigaction\n", signal);
    break;
  }
}

// retour la largeur (resp. la hauteur) de la zone d'affichage d'un groupe en fonction de son nombre de colonnes (resp. de lignes).
int get_width_height(int nb_row_column)
{
  if (nb_row_column == 1) return 100;
  else if (nb_row_column <= 16) return 300;
  else if (nb_row_column <= 128) return 400;
  else if (nb_row_column <= 256) return 700;
  else return 1000;
}

/**
 *
 * Calcule la coordonnée x d'un groupe en fonction de ses prédécesseurs
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
 * Calcule la coordonnée y d'un groupe
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
 * Initialise les bibliothèques GTK et ENet, ainsi que quelques tableaux
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
  else //Si le bouton est désactivé
  {
    saving_press = 0;
    destroy_saving_ref(scripts); //on ferme tout les fichiers et on remet les on_saving à 0
    gtk_widget_queue_draw(architecture_display); // pour remettre à jour l'affichage quand tout les on_saving sont à 0
  }
}

void on_click_save_path_button(GtkWidget *save_button, gpointer pData) //TODO mettre dans le repertoire de l'utilisateur (ce serait cool que le bureau soir également dans ce répertoire)
{

  GtkWidget *selection;

  (void) pData;
  (void) save_button;
  selection = gtk_file_chooser_dialog_new("selectionner un repertoire",
  NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
  NULL);
  gtk_widget_show(selection);

  //On interdit l'utilisation des autres fenêtres.
  //gtk_window_set_modal(GTK_WINDOW(selection), TRUE);

  if (gtk_dialog_run(GTK_DIALOG (selection)) == GTK_RESPONSE_OK)
  {
    char *filename;
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (selection));
    strcpy(path_named, filename);
    strcat(path_named, "/");
  }
  gtk_widget_destroy(selection);

}

void on_script_displayed_toggled(GtkWidget *pWidget, gpointer user_data) //TODO : creer un architecture display uniquement pour un script particulier?.
{
  type_script *script = user_data;

  //Identification du script à afficher ou à masquer
  script->displayed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));

  gtk_widget_queue_draw(architecture_display);
  //architecture_display_update(architecture_display, NULL);
}

void window_title_update()
{
  /* Titre de la fenêtre */
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

  gtk_widget_reparent((GtkWidget*) pdata, vpaned);
  gtk_widget_destroy(pWidget);
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

/**
 *
 * Modification d'une échelle
 *
 */
gboolean refresh_scale_change_value(GtkWidget *pWidget, GtkScrollType scroll, gdouble value, gpointer pData) //TODO : ici ! incohérent avec l'autre fonction ! Modification d'une échelle
{
  (void) pWidget;
  (void) scroll;
  (void) pData;

  if (refresh_mode == REFRESH_MODE_AUTO)
  {
    if (refresh_timer_id != 0)
    {
      g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
      refresh_timer_id = 0;
    }
    value_hz = value;

    if (value > 0.1)
    {
      refresh_timer_id = g_timeout_add((guint) (1000.0 / value), neurons_refresh_display, NULL);
    }
    else
    {
    //  g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
      refresh_timer_id = 0;
    }
  }
  return FALSE;
}

void validate_gtk_entry_text(GtkEntry *pWidget, gpointer data)
{
  (void) pWidget;
  gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_OK);
}

void show_or_mask_default_text(GtkEntry *pWidget, gpointer data)
{
  (void) data;
  gtk_entry_set_text(pWidget, "");
}

// zoom avant sur la zone "architecture_display".
void zoom_in(GdkDevice *pointer)
{
  int i, j, x_rel, y_rel;
  float x, y, previous_xscale = graphic.x_scale, previous_yscale = graphic.y_scale;

  //TODO : ici!
  gdk_window_get_device_position(gtk_widget_get_window(scrollbars), pointer, &x_rel, &y_rel, NULL);
  //gtk_widget_get_pointer(scrollbars, &x_rel, &y_rel); // permet de savoir les coordonnées du pointeur de la souris.

  x = (float) gtk_adjustment_get_value(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars))) + x_rel; // scrollbars->allocation.width/2;
  y = (float) gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars))) + y_rel; //scrollbars->allocation.height/2;

  // calcul des coordonnées du nouveau point pointé par la souris avec les nouvelles échelles.

  for (i = 0; (i + 1) * (LARGEUR_GROUPE + graphic.x_scale) < x; i++)
    ;
  for (j = 0; (j + 1) * (HAUTEUR_GROUPE + graphic.y_scale) < y; j++)
    ;
  x -= i * (LARGEUR_GROUPE + previous_xscale);
  y -= j * (HAUTEUR_GROUPE + previous_yscale);

  graphic.x_scale += ZOOM_GAP;
  if (graphic.x_scale > XSCALE_MAX) graphic.x_scale = XSCALE_MAX;

  graphic.y_scale += ZOOM_GAP;
  if (graphic.y_scale > YSCALE_MAX) graphic.y_scale = YSCALE_MAX;

  x = i * (LARGEUR_GROUPE + graphic.x_scale) + x * graphic.x_scale / previous_xscale - x_rel;
  y = j * (HAUTEUR_GROUPE + graphic.y_scale) + y * graphic.y_scale / previous_yscale - y_rel;

  // application des nouvelles coordonnées du point en haut à gauche de la zone à afficher et propagation du changement de ces valeurs.
  gtk_range_set_value(GTK_RANGE(xScale), graphic.x_scale);
  gtk_range_set_value(GTK_RANGE(yScale), graphic.y_scale);
  architecture_set_view_point(scrollbars, x, y);
  //architecture_display_update(architecture_display, NULL); //On redessine la grille avec la nouvelle échelle.
  gtk_widget_queue_draw(architecture_display);
}

// zoom arrière sur la zone "architecture_display".
void zoom_out(GdkDevice *pointer)
{
  int i, j, x_rel, y_rel;
  float x, y, previous_xscale = graphic.x_scale, previous_yscale = graphic.y_scale;

//TODO : ici!
  //gtk_widget_get_pointer(scrollbars, &x_rel, &y_rel);
  gdk_window_get_device_position(gtk_widget_get_window(scrollbars), pointer, &x_rel, &y_rel, NULL);

  x = (float) gtk_adjustment_get_value(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars))) + x_rel; // scrollbars->allocation.width/2;
  y = (float) gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars))) + y_rel; //scrollbars->allocation.height/2;

  for (i = 0; (i + 1) * (LARGEUR_GROUPE + graphic.x_scale) < x; i++)
    ;
  for (j = 0; (j + 1) * (HAUTEUR_GROUPE + graphic.y_scale) < y; j++)
    ;
  x -= i * (LARGEUR_GROUPE + previous_xscale);
  y -= j * (HAUTEUR_GROUPE + previous_yscale);

  graphic.x_scale -= ZOOM_GAP;
  if (graphic.x_scale < XSCALE_MIN) graphic.x_scale = XSCALE_MIN;

  graphic.y_scale -= ZOOM_GAP;
  if (graphic.y_scale < YSCALE_MIN) graphic.y_scale = YSCALE_MIN;

  x = i * (LARGEUR_GROUPE + graphic.x_scale) + x * graphic.x_scale / previous_xscale - x_rel; // scrollbars->allocation.width/2;
  y = j * (HAUTEUR_GROUPE + graphic.y_scale) + y * graphic.y_scale / previous_yscale - y_rel; //scrollbars->allocation.height/2;scrollbars->allocation.height/2;

  gtk_range_set_value(GTK_RANGE(xScale), graphic.x_scale);
  gtk_range_set_value(GTK_RANGE(yScale), graphic.y_scale);
  architecture_set_view_point(scrollbars, x, y);
  //architecture_display_update(architecture_display, NULL); //On redessine la grille avec la nouvelle échelle.
  gtk_widget_queue_draw(architecture_display);
}

/**
 *
 * Clic souris
 *
 */
void architecture_display_button_pressed(GtkWidget *pWidget, GdkEventButton *event, gpointer pdata)
{
  cairo_t *cr;
  (void) pWidget;
  (void) pdata;

  selected_group = NULL;

  // architecture_display_update(architecture_display, NULL);

  cr = gdk_cairo_create(gtk_widget_get_window(architecture_display)); //Crée un contexte Cairo associé à la drawing_area "zone"
  gdk_cairo_set_source_window(cr, gtk_widget_get_window(architecture_display), 0, 0);
  architecture_display_update(architecture_display, cr, event);
  cairo_destroy(cr);
  // gtk_widget_queue_draw (architecture_display);

  switch (event->button)
  {
  case 1:
    architecture_display_dragging_currently = TRUE;
    architecture_display_cursor_x = event->x;
    architecture_display_cursor_y = event->y;
    old_x = gtk_adjustment_get_value(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars)));
    old_y = gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars)));
    move_neurons_start = FALSE;
    open_neurons_start = FALSE;
    break;
  case 3:
    open_neurons_start = TRUE;
    move_neurons_start = FALSE;
    open_group = selected_group;
    if (open_group != NULL) group_display_new(open_group, 25, 25, zone_neurons);
    break;
  default:
    move_neurons_start = FALSE;
    open_neurons_start = FALSE;
    break;
  }

}

void architecture_display_button_released(GtkWidget *pWidget, GdkEventButton *event, gpointer pdata)
{
  (void) pWidget;
  (void) pdata;

  if (event->button == 1) // boutton gauche
  architecture_display_dragging_currently = FALSE;

  // architecture_display_cursor_x = (float) event->x;
  // architecture_display_cursor_y = (float) event->y;
}

// permet de déplacer la zone à afficher s'active en appuyant sur la molette.
void architecture_display_drag_motion(GtkWidget *pWidget, GdkEventMotion *event, gpointer pdata)
{
  (void) pWidget;
  (void) pdata;

  if (architecture_display_dragging_currently == TRUE)
  {
    new_x = old_x - event->x + architecture_display_cursor_x;
    //if(new_x < 0.0) new_x = 0.0;
    //else if(new_x > (gdouble)(10000 - scrollbars->allocation.width)) new_x = (gdouble)(10000 - scrollbars->allocation.width);

    new_y = old_y - event->y + architecture_display_cursor_y;
    //if(new_y < 0.0) new_y = 0.0;
    //else if(new_y > (gdouble)(10000 - scrollbars->allocation.height)) new_y = (gdouble)(10000 - scrollbars->allocation.height);

    architecture_set_view_point(scrollbars, new_x, new_y);
    //architecture_display_cursor_x = event->x;
    //architecture_display_cursor_y = event->y;
  }
}

// ouvre la fenetre de recherche en sélectionnant par défaut le script passé en paramètre.
void on_search_group_button_active(GtkWidget *pWidget, type_script *script)
{
  int i, index = 0;
  (void) pWidget;

  for (i = 0; i < number_of_scripts && scripts[i] != script; i++)
    ;

  index = (i == number_of_scripts ? 0 : i);

  on_search_group(index);
}

//
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

//On crée une fenêtre de dialogue
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
//On lui ajoute une entrée de saisie de texte
  search_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(search_entry), "");
  g_signal_connect(G_OBJECT(search_entry), "activate", G_CALLBACK(validate_gtk_entry_text), search_dialog);
  //g_signal_connect(G_OBJECT(search_entry), "clicked", G_CALLBACK(show_or_mask_default_text), search_dialog);
  search_h_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(search_h_box), search_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(search_h_box), search_entry, TRUE, TRUE, 0);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  ;

  gtk_box_pack_start(GTK_BOX(vbox), h_box, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), scripts_h_box, TRUE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), search_h_box, TRUE, FALSE, 0);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG (search_dialog));
  gtk_container_add(GTK_CONTAINER (content_area), vbox);

  gtk_widget_show_all(search_dialog);

//Aucun groupe n'est séléctionné
  selected_group = NULL;

  do
  {
    switch (gtk_dialog_run(GTK_DIALOG(search_dialog)))
    {
    //Si la réponse est OK
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
            //Le groupe correspondant est séléctionné
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
            //Le groupe correspondant est séléctionné
            selected_group = &selected_script->groups[i];
            // les coordonnées de ce groupe sont récupérées et le groupe est placé au centre de l'affichage.
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

//Si le bouton est activé
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_see_legend_button)))
  {
    //On cache le menu de droite
    gtk_widget_hide(pData);
    //On actualise l'image du boutton
    gtk_image_set_from_stock(GTK_IMAGE(arrow_im_2), GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU);

  }
//Si le bouton est désactivé
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

//Si le bouton est activé
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_see_scales_button)))
  {
    //On cache le menu de gauche
    gtk_widget_hide(pData);
    //On actualise l'image du boutton
    gtk_image_set_from_stock(GTK_IMAGE(arrow_im), GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);

  }
//Si le bouton est désactivé
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

// permet la gestion du zoom avec CTRL + molette. Molette vers le haut pour le zoom avant et vers le bas pour le zoom arrière.
gboolean architecture_display_scroll_event(GtkWidget *pWidget, GdkEventScroll *event, gpointer pdata)
{
  (void) pWidget;
  (void) pdata;

  if ((gdk_device_get_source(event->device) != GDK_SOURCE_MOUSE) || architecture_display_dragging_currently)
  {
    return FALSE;
  }
  else
  {
    if ((event->direction == GDK_SCROLL_UP) && (event->state & GDK_CONTROL_MASK))
    {
      zoom_in(event->device);
      return TRUE;
    }

    if ((event->direction == GDK_SCROLL_DOWN) && (event->state & GDK_CONTROL_MASK))
    {
      zoom_out(event->device);
      return TRUE;
    }
  }
  return FALSE;
}

/**
 *
 * Appui sur une touche
 *
 */
void key_press_event(GtkWidget *pWidget, GdkEventKey *event)
{
  int i, j;
  int libre = 0;
  int sauts = 1;
  GdkDeviceManager *device_manager;
  GdkDevice *pointer;

  switch (event->keyval)
  {
  case GDK_KEY_Up: //Touche "Page up", on réduit le y du groupe sélectionné pour le faire monter
    if (selected_group != NULL)
    {
      do
      {
        libre = 1;
        for (i = 0; i < number_of_scripts; i++)
          if (scripts[i]->z == selected_group->script->z && scripts[i]->displayed == TRUE) //Pour chaque script dans le même plan que le groupe sélectionné
          for (j = 0; j < scripts[i]->number_of_groups; j++)
            if (scripts[i]->groups[j].x == selected_group->x && scripts[i]->groups[j].y == (selected_group->y - sauts))
            {
              libre = 0; //La case n'est pas libre
              sauts++; //Le groupe doit faire un saut de plus
            }
      } while (libre == 0); //Tant qu'on n'a pas trouvé une case libre
      selected_group->y -= sauts;
      gtk_widget_queue_draw(architecture_display);
      // architecture_display_update(architecture_display, NULL);
    }
    break;
  case GDK_KEY_Down: //Touche "Page down", on augmente le y du groupe sélectionné pour le faire descendre
    if (selected_group != NULL)
    {
      do
      {
        libre = 1;
        for (i = 0; i < number_of_scripts; i++)
          if (scripts[i]->z == selected_group->script->z && scripts[i]->displayed == TRUE) for (j = 0; j < scripts[i]->number_of_groups; j++)
            if (scripts[i]->groups[j].x == selected_group->x && scripts[i]->groups[j].y == (selected_group->y + sauts))
            {
              libre = 0; //La case n'est pas libre
              sauts++; //Le groupe doit faire un saut de plus
            }
      } while (libre == 0); //Tant qu'on n'a pas trouvé une case libre
      selected_group->y += sauts;
      //architecture_display_update(architecture_display, NULL);
      gtk_widget_queue_draw(architecture_display);
    }
    break;

  case GDK_KEY_f:
    if (event->state & GDK_CONTROL_MASK) on_search_group(-1);
    break;
  case GDK_KEY_s:
    if (event->state & GDK_CONTROL_MASK) save_preferences(NULL, NULL);
    break;
  case GDK_KEY_S:
    if (event->state & GDK_CONTROL_MASK) save_preferences_as(NULL, NULL);
    break;
  case GDK_KEY_KP_Add:
  case GDK_KEY_plus:

    if (event->state & GDK_CONTROL_MASK)
    {
      device_manager = gdk_display_get_device_manager(gtk_widget_get_display(pWidget));
      pointer = gdk_device_manager_get_client_pointer(device_manager);
      zoom_in(pointer);
    }

    break;
  case GDK_KEY_minus:
  case GDK_KEY_KP_Subtract:

    if (event->state & GDK_CONTROL_MASK)
    {
      device_manager = gdk_display_get_device_manager(gtk_widget_get_display(pWidget));
      pointer = gdk_device_manager_get_client_pointer(device_manager);
      zoom_out(pointer);
    }

    break;
  default:
    break; //Appui sur une autre touche. On ne fait rien.
  }
}

/**
 *
 * Enregistrer les préférences dans un fichier
 *
 */
void save_preferences(GtkWidget *pWidget, gpointer pData)
{
  char* filename;
  GtkFileFilter *file_filter, *generic_file_filter;

  GtkWidget *dialog;
  (void) pWidget;
  (void) pData;

  if (preferences_filename[0] == 0)
  {
    dialog = gtk_file_chooser_dialog_new("Save perspective", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    file_filter = gtk_file_filter_new();
    generic_file_filter = gtk_file_filter_new();

    gtk_file_filter_add_pattern(file_filter, "*.pandora");
    gtk_file_filter_add_pattern(generic_file_filter, "*");

    gtk_file_filter_set_name(file_filter, "pandora (.pandora)");
    gtk_file_filter_set_name(generic_file_filter, "all types");

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), generic_file_filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {

      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      strcpy(preferences_filename, filename);
      g_free(filename);
      gtk_widget_destroy(dialog);
    }
    else
    {
      gtk_widget_destroy(dialog);
      return;
    }
  }
  else
  {
    GtkWidget *confirmation = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "File \"%s\" already exists.\nDo you really want to save?", preferences_filename);
    switch (gtk_dialog_run(GTK_DIALOG(confirmation)))
    {
    case GTK_RESPONSE_YES:
      gtk_widget_destroy(confirmation);
      break;
    default:
      gtk_widget_destroy(confirmation);
      return;
      break;
    }
  }
  pandora_file_save(preferences_filename);
}

void save_preferences_as(GtkWidget *pWidget, gpointer pData)
{
  char* filename;
  GtkFileFilter *file_filter, *generic_file_filter;

  GtkWidget *dialog;
  (void) pWidget;
  (void) pData;

  dialog = gtk_file_chooser_dialog_new("Save perspective", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

  file_filter = gtk_file_filter_new();
  generic_file_filter = gtk_file_filter_new();

  gtk_file_filter_add_pattern(file_filter, "*.pandora");
  gtk_file_filter_add_pattern(generic_file_filter, "*");

  gtk_file_filter_set_name(file_filter, "pandora (.pandora)");
  gtk_file_filter_set_name(generic_file_filter, "all types");

  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), generic_file_filter);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    strcpy(preferences_filename, filename);
    g_free(filename);
    gtk_widget_destroy(dialog);
  }
  else
  {
    gtk_widget_destroy(dialog);
    return;
  }

  pandora_file_save(preferences_filename);
}

/**
 *
 * Ouvrir un fichier de préférences
 *
 */
void pandora_load_preferences(GtkWidget *pWidget, gpointer pData)
{
  char* filename;
  GtkFileFilter *file_filter, *generic_file_filter;
  GtkWidget *dialog;

  (void) pWidget;
  (void) pData;

  dialog = gtk_file_chooser_dialog_new("Load perspective", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

  file_filter = gtk_file_filter_new();
  generic_file_filter = gtk_file_filter_new();

  gtk_file_filter_add_pattern(file_filter, "*.pandora");
  gtk_file_filter_add_pattern(generic_file_filter, "*");

  gtk_file_filter_set_name(file_filter, "pandora (.pandora)");
  gtk_file_filter_set_name(generic_file_filter, "all types");

  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), generic_file_filter);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    strncpy(preferences_filename, filename, PATH_MAX);
    window_title_update();
    pandora_file_load(filename);
    g_free(filename);
  }
  gtk_widget_destroy(dialog);

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
  //architecture_display_update(architecture_display, NULL); //On redessine la grille avec la nouvelle échelle.
  refresh_scale_change_value(NULL, GTK_SCROLL_NONE, (gdouble) REFRESHSCALE_DEFAULT, NULL); // modifie la fréquence de rafraichissement.
}

void on_group_display_output_combobox_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = data;
  int new_output = gtk_combo_box_get_active(combo_box), i;
  prom_images_struct *images;
  int height = 0;
  char legende[64];
  GtkWidget *frame = GTK_WIDGET(gtk_builder_get_object(builder, "list_graph"));
  GtkWidget *image_box = GTK_WIDGET(gtk_builder_get_object(builder, "image_hbox"));
  int nb;

  if (new_output == 3 && group->output_display != 3)
  {
    if (refresh_mode != REFRESH_MODE_MANUAL)
    {
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, group->script->name);
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_START, group->id, group->script->name);
    }
    if (gtk_widget_get_visible(frame) == TRUE)
    {
      gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "list_graph")));
      height -= gtk_widget_get_allocated_height(frame);
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
      if (gtk_widget_get_visible(image_box) == FALSE)
      {
        gtk_widget_show_all(image_box);
        height += gtk_widget_get_allocated_height(image_box);
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
    if (group->display_mode == DISPLAY_MODE_BIG_GRAPH && gtk_widget_get_visible(frame) == FALSE)
    {
      height += gtk_widget_get_allocated_height(frame);
      gtk_widget_show_all(frame);
    }
    if (gtk_widget_get_visible(image_box) == TRUE)
    {
      gtk_widget_hide(image_box);
      height -= gtk_widget_get_allocated_height(image_box);
    }
    gtk_window_resize(GTK_WINDOW(selected_group_dialog), gtk_widget_get_allocated_width(selected_group_dialog), gtk_widget_get_allocated_height(selected_group_dialog) + height);
  }
  group->previous_output_display = group->output_display;
  group->output_display = gtk_combo_box_get_active(combo_box);

  if (group->previous_output_display != group->output_display) resize_group(group);
}

void on_group_display_mode_combobox_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = data;
  GtkWidget *frame = GTK_WIDGET(gtk_builder_get_object(builder, "list_graph"));

  group->previous_display_mode = group->display_mode;
  group->display_mode = gtk_combo_box_get_active(combo_box);

  if (group->display_mode == DISPLAY_MODE_BIG_GRAPH && gtk_widget_get_visible(frame) == FALSE && group->output_display != 3)
  {
    gtk_window_resize(GTK_WINDOW(selected_group_dialog), gtk_widget_get_allocated_width(selected_group_dialog), gtk_widget_get_allocated_height(selected_group_dialog) + gtk_widget_get_allocated_height(frame));
    gtk_widget_show_all(frame);
  }
  else if (group->display_mode != DISPLAY_MODE_BIG_GRAPH && gtk_widget_get_visible(frame) == TRUE)
  {
    gtk_widget_hide(frame);
    //gtk_window_resize(GTK_WINDOW(selected_group_dialog), selected_group_dialog->allocation.width, selected_group_dialog->allocation.height - frame->allocation.height); //TODO : corriger et comprendre
  }
  if (group->previous_display_mode != group->display_mode) resize_group(group);
}

void resize_group(type_group *group)
{
  if (group->output_display == 3)
  {
    gtk_widget_hide(GTK_WIDGET(group->button_vbox));
  }
  else if (group->display_mode == DISPLAY_MODE_BIG_GRAPH)
  {
    gtk_widget_show_all(GTK_WIDGET(group->button_vbox));
    gtk_widget_set_size_request(group->widget, GRAPH_WIDTH, GRAPH_HEIGHT);
    gtk_widget_set_size_request(group->drawing_area, GRAPH_WIDTH - BUTTON_WIDTH, GRAPH_HEIGHT);
  }
  else
  {
    gtk_widget_hide(GTK_WIDGET(group->button_vbox));
    gtk_widget_set_size_request(group->widget, get_width_height(group->columns), get_width_height(group->rows));
  }
}

void on_group_display_min_spin_button_value_changed(GtkSpinButton *spin_button, gpointer data)
{
  type_group *group = data;
  group->val_min = gtk_spin_button_get_value(spin_button);
}

void on_group_display_max_spin_button_value_changed(GtkSpinButton *spin_button, gpointer data)
{
  type_group *group = data;
  group->val_max = gtk_spin_button_get_value(spin_button);
}

void on_group_display_auto_checkbox_toggled(GtkToggleButton *button, gpointer data)
{
  type_group *group = data;
  group->normalized = gtk_toggle_button_get_active(button);
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
  group->image_selected_index = gtk_combo_box_get_active(combo_box);
}

void on_group_display_clicked(GtkButton *button, type_group *group)
{
  char builder_file_name[PATH_MAX];
  GError *g_error = NULL;
  GtkWidget *frame = NULL, *vbox, *hbox, *label, *left_spinner, *right_spinner, *image_hbox;
  int height = 0;
  float r, g, b;
  char markup[126];
  int i;
  prom_images_struct *images;
  (void) button;

  if (selected_group_dialog != NULL) gtk_widget_destroy(selected_group_dialog);

  builder = gtk_builder_new();
  snprintf(builder_file_name, PATH_MAX, "%s/bin_leto_prom/resources/pandora_group_display_properties.glade", getenv("HOME"));
  gtk_builder_add_from_file(builder, builder_file_name, &g_error);
  if (g_error != NULL) EXIT_ON_ERROR("%s", g_error->message);
  gtk_builder_connect_signals(builder, group);

  selected_group_dialog = GTK_WIDGET(gtk_builder_get_object(builder, "group_display_dialog"));
  gtk_window_set_title(GTK_WINDOW(selected_group_dialog), group->name);
  gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "group_display_output_combobox")), group->output_display);
  gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "group_display_mode_combobox")), group->display_mode);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "group_display_min_spin_button")), group->val_min);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "group_display_max_spin_button")), group->val_max);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "group_display_auto_checkbox")), group->normalized);

  /// partie de la fenêtre permettant de pré-sélectionner les neurones à afficher dans le graphe
  frame = GTK_WIDGET(gtk_builder_get_object(builder, "list_graph"));
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(vbox), TRUE);
  gtk_container_remove(GTK_CONTAINER(frame), GTK_WIDGET(gtk_builder_get_object(builder, "box1")));
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  for (i = 0; i < group->number_of_courbes; i++) // signaux à créer et ajouter aux spinners.
  {
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    ;

    label = gtk_label_new("");
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

    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
  }

  image_hbox = GTK_WIDGET(gtk_builder_get_object(builder, "image_hbox"));
  label = gtk_label_new("Selected image : ");
  gtk_box_pack_start(GTK_BOX(image_hbox), label, TRUE, FALSE, 0);
  selected_image_combo_box = gtk_combo_box_text_new();
  gtk_box_pack_start(GTK_BOX(image_hbox), selected_image_combo_box, TRUE, FALSE, 0);
  g_signal_connect(G_OBJECT(selected_image_combo_box), "changed", G_CALLBACK(on_group_display_selected_image_changed), group);

  gtk_widget_show_all(selected_group_dialog);

  if (group->display_mode != DISPLAY_MODE_BIG_GRAPH || group->output_display == 3)
  {
    gtk_widget_hide(frame);
    height += gtk_widget_get_allocated_height(frame);
  }
  if (group->output_display == 3 && group->ext != NULL && ((prom_images_struct *) group->ext)->image_number > 0)
  {
    images = group->ext;
    for (i = 0; i < (int) images->image_number; i++)
    {
      sprintf(markup, "image %d", i);
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(selected_image_combo_box), markup);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(selected_image_combo_box), group->image_selected_index);
  }
  else
  {
    gtk_widget_hide(image_hbox);
    height += gtk_widget_get_allocated_height(image_hbox);
  }
  gtk_window_resize(GTK_WINDOW(selected_group_dialog), gtk_widget_get_allocated_width(selected_group_dialog), gtk_widget_get_allocated_height(selected_group_dialog) - height);
}

gboolean button_press_neurons(GtkStatusIcon *status_icon, GdkEvent *event, type_group *group)
{
  GdkEventButton *event_button;
  event_button = (GdkEventButton*) event;
  (void) status_icon;
  switch (event_button->button)
  {
  case 1:
    move_neurons_old_x = event_button->x;
    move_neurons_old_y = event_button->y;
    move_neurons_start = TRUE;
    move_neurons_group = group;
    selected_group = group; //Le groupe associé à la fenêtre dans laquelle on a cliqué est également sélectionné
    if (event_button->type == GDK_2BUTTON_PRESS) on_group_display_clicked(NULL, group); //Affichage de la petite fenetre etc.. (?)
    break;

  case 2:
    group_display_destroy(group);
    break;

  case 3:
    break;

  default:
    break;
  }

//On actualise l'affichage
  gtk_widget_queue_draw(architecture_display);
  //architecture_display_update(architecture_display, NULL);

  return TRUE;
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
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_PHASES_INFO_START, 0, scripts[i]->name);
    //printf("ordre d'envoie bien lancé\n");
  }
  else
  {
    gtk_button_set_label(GTK_BUTTON(pWidget), "Start calculate execution times");
    for (i = 0; i < number_of_scripts; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_PHASES_INFO_STOP, 0, scripts[i]->name);

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

void on_group_display_show_or_mask_neuron(GtkWidget *pWidget, gpointer pData)
{
  int *data = pData;
  *data = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));
}

void script_widget_update(type_script *script)
{
  char label_text[LABEL_MAX];
  //script->label=NULL;
  //script->label=gtk_label_new("");
  sprintf(label_text, "<span foreground=\"%s\"><b>%s</b></span>", tcolor(script), script->name);
  gtk_label_set_markup(GTK_LABEL(script->label), label_text);
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

  /**On veut déterminer zMax, la plus grande valeur de z parmi les scripts ouverts
   */
  zMax = number_of_scripts;

  /**On remplit le panneau des scripts
   */
  gdk_threads_enter();

  script->widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxScripts), script->widget, FALSE, TRUE, 0);

//Cases à cocher pour afficher les scripts ou pas
  script->checkbox = gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script->checkbox), FALSE); //Par défaut, les scripts ne sont pas cochés, donc pas affichés
  g_signal_connect(G_OBJECT(script->checkbox), "toggled", G_CALLBACK(on_script_displayed_toggled), script);
  gtk_box_pack_start(GTK_BOX(script->widget), script->checkbox, FALSE, TRUE, 0);

//Labels des scripts : le nom du script écrit en gras, de la couleur du script
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

//Pour que les cases soient cochées par défaut
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script->checkbox), TRUE);
  script->displayed = TRUE;

  gtk_widget_show_all(pVBoxScripts); //Affichage du widget pWindow et de tous ceux qui sont dedans

  script_widget_update(script); //TODO erreur ici probablement
  //architecture_display_update(architecture_display, NULL); // TODO : interet ?????????
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

  type_group *group;
  int i, j, save_id = 0, number_of_group_displayed = 0;
  type_script_display_save *save = NULL;
  GtkAllocation allocation;

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

          arguments.group = &(group[i]);
          arguments.posx = save->groups[j].x;
          arguments.posy = save->groups[j].y;
          arguments.zone_neuron=zone_neurons;
          // pthread_mutex_unlock(&mutex_script_caracteristics);
          // g_idle_add ((GSourceFunc)group_display_new_threaded,(gpointer)&arguments);
          pthread_mutex_unlock(&mutex_script_caracteristics);
          g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc) group_display_new_threaded, (gpointer) &arguments, NULL);

          //g_main_context_invoke (NULL,(GSourceFunc)group_display_new_threaded,(gpointer)(&arguments));//TODO : resoudre ce probleme !
          //Il est important d'attendre car sinon on envoie une fonction à executer dans la boucle principale puis on la renvoie sans attendre que la précédente ai finie, ce qui cause des problemes, il est notemment compliqué de passer des arguments différents car on les modifie lors de la boucle suivante. //TODO : envisager un tableau d'argument aussi grand que la longueur de la boucle for? Aurais l'avantage de ne pas pénaliser le script enet.
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
  int i;
  type_group *group;

  //pthread_mutex_lock(&mutex_script_caracteristics);
  script_caracteristics(script, SAVE_SCRIPT_GROUPS_CARACTERISTICS);
  //pthread_mutex_unlock(&mutex_script_caracteristics);

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
    free(script->groups[i].neurons);
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

  // mise à jour des coordonnées des scripts

  for (i = script->id; i < number_of_scripts; i++)
  {
    script_update_positions(scripts[i]);
    script_widget_update(scripts[i]);
  }

  gtk_widget_queue_draw(architecture_display);
}

/**
 * Cette fonction permet de traiter les informations provenant de prométhé pour mettre à jour
 * l'affichage. Elle est appelée automatiquement plusieurs fois par seconde.
 */
gboolean neurons_refresh_display()
{
  int i;
  if (refresh_mode != REFRESH_MODE_AUTO) return FALSE;

  pthread_mutex_lock(&mutex_script_caracteristics);
  for (i = 0; i < number_of_groups_to_display; i++)
  {
    groups_to_display[i]->refresh_freq = TRUE;
    gtk_widget_queue_draw(GTK_WIDGET(groups_to_display[i]->drawing_area));
    //group_expose_neurons(groups_to_display[i], TRUE, TRUE);
  }
  pthread_mutex_unlock(&mutex_script_caracteristics);
  return TRUE;
}

/*
 * Mise en page du xml dans le fichier de sauvegarde des préférences (*.jap)
 */
const char* whitespace_callback_pandora(mxml_node_t *node, int where)
{
  const char *name;

  if (node == NULL)
  {
    fprintf(stderr, "%s \n", __FUNCTION__);
    return NULL;
  }

  name = node->value.element.name;

  if (where == MXML_WS_AFTER_CLOSE || where == MXML_WS_AFTER_OPEN)
  {
    return ("\n");
  }

  if (!strcmp(name, "preferences"))
  {
    if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) return ("  ");
  }
  else if (!strcmp(name, "script"))
  {
    if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) return ("    ");
  }
  else if (!strcmp(name, "group"))
  {
    if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) return ("      ");
  }

  return (NULL);
}

//TODO : a mettre dans un fichier séparé ainsi que tout ce qui touche à la sauvegarde de preference.
void pandora_file_save(const char *filename)
{
  int script_id, i, width, height;
  GtkWidget *widget;
  Node *tree, *script_node, *group_node, *preferences_node;
  type_group *group;
  type_script *script;
  GtkAllocation allocation;

  tree = mxmlNewXML("1.0");

  preferences_node = mxmlNewElement(tree, "preferences");
  xml_set_string(preferences_node, "bus_id", bus_id);
  xml_set_string(preferences_node, "bus_ip", bus_ip);

  gtk_window_get_size(GTK_WINDOW(window), &width, &height);
  xml_set_int(preferences_node, "window_width", width);
  xml_set_int(preferences_node, "window_height", height);
  xml_set_int(preferences_node, "refresh", gtk_range_get_value(GTK_RANGE(refreshScale)));
  xml_set_int(preferences_node, "x_scale", gtk_range_get_value(GTK_RANGE(xScale)));
  xml_set_int(preferences_node, "y_scale", gtk_range_get_value(GTK_RANGE(yScale)));
  xml_set_int(preferences_node, "z_x_scale", gtk_range_get_value(GTK_RANGE(zxScale)));
  xml_set_int(preferences_node, "z_y_scale", gtk_range_get_value(GTK_RANGE(zyScale)));
  xml_set_int(preferences_node, "architecture_window_height", gtk_paned_get_position(GTK_PANED(vpaned)));

  for (script_id = 0; script_id < number_of_scripts; script_id++)
  {
    script = scripts[script_id];
    script_node = mxmlNewElement(preferences_node, "script");
    xml_set_string(script_node, "name", scripts[script_id]->name);

    xml_set_int(script_node, "visibility", script->displayed);

    for (i = 0; i < number_of_groups_to_display; i++)
    {
      group = groups_to_display[i];
      if (group->script == script)
      {
        group_node = NULL;
        widget = group->widget;
        group_node = mxmlNewElement(script_node, "group");
        xml_set_string(group_node, "name", group->name);
        xml_set_int(group_node, "output_display", group->output_display);
        xml_set_int(group_node, "display_mode", group->display_mode);
        xml_set_float(group_node, "min", group->val_min);
        xml_set_float(group_node, "max", group->val_max);
        xml_set_int(group_node, "normalized", group->normalized);

        gtk_widget_get_allocation(widget, &allocation);

        xml_set_int(group_node, "x", allocation.x);
        xml_set_int(group_node, "y", allocation.y);
      }
    }
  }

  xml_save_file((char *) filename, tree, whitespace_callback_pandora);

  mxmlDelete(tree);
}

void pandora_file_load(const char *filename)
{
  int script_id, group_id, value, value2;
  Node *tree, *script_node, *group_node, *preferences_node;
  type_group *group;
  type_script *script;

  while (number_of_groups_to_display != 0)
  {
    group_display_destroy(groups_to_display[0]);
  }

  tree = xml_load_file(filename); // TODO oublie de liberation de memoire

  preferences_node = xml_get_first_child_with_node_name(tree, "preferences");
  /* On change le bus id s'il n'a pas été mis en paramètre */
  if (bus_id[0] == 0) strcpy(bus_id, xml_get_string(preferences_node, "bus_id"));
  if (bus_ip[0] == 0) strcpy(bus_id, xml_get_string(preferences_node, "bus_ip"));

  if (xml_try_to_get_int(preferences_node, "window_width", &value) && xml_try_to_get_int(preferences_node, "window_height", &value2)) gtk_window_resize(GTK_WINDOW(window), value, value2);
  if (xml_try_to_get_int(preferences_node, "refresh", &value)) gtk_range_set_value(GTK_RANGE(refreshScale), value);
  if (xml_try_to_get_int(preferences_node, "x_scale", &value)) gtk_range_set_value(GTK_RANGE(xScale), (double) value);
  if (xml_try_to_get_int(preferences_node, "y_scale", &value)) gtk_range_set_value(GTK_RANGE(yScale), value);
  if (xml_try_to_get_int(preferences_node, "z_x_scale", &value)) gtk_range_set_value(GTK_RANGE(zxScale), value);
  if (xml_try_to_get_int(preferences_node, "z_y_scale", &value)) gtk_range_set_value(GTK_RANGE(zyScale), value);
  if (xml_try_to_get_int(preferences_node, "architecture_window_height", &value)) gtk_paned_set_position(GTK_PANED(vpaned), value);

  for (script_id = 0; script_id < number_of_scripts; script_id++)
  {
    script = scripts[script_id];
    for (script_node = xml_get_first_child_with_node_name(preferences_node, "script"); script_node; script_node = xml_get_next_homonymous_sibling(script_node))
    {
      if (!strcmp(script->name, xml_get_string(script_node, "name")))
      {
        script->displayed = xml_get_int(script_node, "visibility");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script->checkbox), scripts[script_id]->displayed);

        if (xml_get_number_of_childs(script_node) > 0)
        {
          for (group_id = 0; group_id < scripts[script_id]->number_of_groups; group_id++)
          {
            group = &script->groups[group_id];
            for (group_node = xml_get_first_child_with_node_name(script_node, "group"); group_node; group_node = xml_get_next_homonymous_sibling(group_node))
            {
              if (!strcmp(group->name, xml_get_string(group_node, "name")))
              {
                group->output_display = xml_get_int(group_node, "output_display");
                group->display_mode = xml_get_int(group_node, "display_mode");
                group->val_min = xml_get_float(group_node, "min");
                group->val_max = xml_get_float(group_node, "max");
                group->normalized = xml_get_int(group_node, "normalized");
                group_display_new(group, xml_get_float(group_node, "x"), xml_get_float(group_node, "y"), zone_neurons);

              }
            }
          }
        }
      }
    }
  }
}

void pandora_file_load_script(const char *filename, type_script *script)
{
  int group_id;
  Node *tree, *script_node, *group_node, *preferences_node;
  type_group *group;
  tree = xml_load_file((char *) filename);

  preferences_node = xml_get_first_child_with_node_name(tree, "preferences");
  /* On change le bus id s'il n'a pas été mis en paramètre */
  if (bus_id[0] == 0) strcpy(bus_id, xml_get_string(preferences_node, "bus_id"));
  if (bus_ip[0] == 0) strcpy(bus_id, xml_get_string(preferences_node, "bus_ip"));

  for (script_node = xml_get_first_child_with_node_name(preferences_node, "script"); script_node; script_node = xml_get_next_homonymous_sibling(script_node))
  {
    if (!strcmp(script->name, xml_get_string(script_node, "name")))
    {
      pthread_mutex_lock(&mutex_script_caracteristics);

      script->displayed = xml_get_int(script_node, "visibility");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script->checkbox), script->displayed);
      pthread_mutex_unlock(&mutex_script_caracteristics);

      if (xml_get_number_of_childs(script_node) > 0)
      {
        for (group_id = 0; group_id < script->number_of_groups; group_id++)
        {
          group = &(script->groups[group_id]);
          for (group_node = xml_get_first_child_with_node_name(script_node, "group"); group_node; group_node = xml_get_next_homonymous_sibling(group_node))
          {
            if (!strcmp(group->name, xml_get_string(group_node, "name")))
            {
              group->output_display = xml_get_int(group_node, "output_display");
              group->display_mode = xml_get_int(group_node, "display_mode");
              group->val_min = xml_get_float(group_node, "min");
              group->val_max = xml_get_float(group_node, "max");
              group->normalized = xml_get_int(group_node, "normalized");

              arguments.group = group;
              arguments.posx = xml_get_float(group_node, "x");
              arguments.posy = xml_get_float(group_node, "y");
              arguments.zone_neuron=zone_neurons;

              g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc) group_display_new_threaded, (gpointer) &arguments, NULL);
              //g_main_context_invoke (NULL,(GSourceFunc)group_display_new_threaded,(gpointer)(&arguments));//TODO : resoudre ce probleme !
              pthread_mutex_lock(&mutex_loading);
              pthread_cond_wait(&cond_loading, &mutex_loading);
              pthread_mutex_unlock(&mutex_loading);
              //TODO : enlever les variable mutex et arguments des variables globales.
              // group_display_new(group, xml_get_float(group_node, "x"), xml_get_float(group_node, "y"));

            }
          }
        }
      }
    }
  }

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
    if (id_semi_automatic != 0)
    {
      g_source_destroy(g_main_context_find_source_by_id(NULL, id_semi_automatic));
      id_semi_automatic = 0;
    }
    /*
     if(id_manual!=0)
     {g_source_destroy(g_main_context_find_source_by_id(NULL, id_manual)); id_manual = 0;}

     if(id_manual==0) id_manual = g_timeout_add((guint) 50, neurons_refresh_display_without_change_values, NULL); //TODO comprendre interet d'ajouter un idle pour les click manuel ??????
     */
    for (i = 0; i < number_of_groups_to_display; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_STOP : PANDORA_SEND_NEURONS_STOP), groups_to_display[i]->id, groups_to_display[i]->script->name);

    gtk_widget_show_all(GTK_WIDGET(data));
    break;

  case REFRESH_MODE_AUTO:
    if (id_semi_automatic != 0)
    {
      g_source_destroy(g_main_context_find_source_by_id(NULL, id_semi_automatic));
      id_semi_automatic = 0;
    }
    if (refresh_timer_id != 0)
    {
      g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
      refresh_timer_id = 0;
    }

    if (refresh_timer_id == 0 && value_hz > 0.1) refresh_timer_id = g_timeout_add((guint) (1000.0 / value_hz), neurons_refresh_display, NULL);
    else if (refresh_timer_id == 0) refresh_timer_id = 0;
    /*if(id_manual!=0)
     {g_source_destroy(g_main_context_find_source_by_id(NULL, id_manual)); id_manual = 0;}
     */

    for (i = 0; i < number_of_groups_to_display; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_START : PANDORA_SEND_NEURONS_START), groups_to_display[i]->id, groups_to_display[i]->script->name);

    gtk_widget_hide(GTK_WIDGET(data));
    break;
  case REFRESH_MODE_SEMI_AUTO:

    /*if(id_manual!=0)
     {g_source_destroy(g_main_context_find_source_by_id(NULL, id_manual)); id_manual = 0;}*/
    if (refresh_timer_id != 0)
    {
      g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
      refresh_timer_id = 0;
    }
    if (id_semi_automatic != 0)
    {
      g_source_destroy(g_main_context_find_source_by_id(NULL, id_semi_automatic));
      id_semi_automatic = 0;
    }

    if (id_semi_automatic == 0) id_semi_automatic = g_timeout_add((guint) 150, neurons_display_refresh_when_semi_automatic, NULL);

    for (i = 0; i < number_of_groups_to_display; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_START : PANDORA_SEND_NEURONS_START), groups_to_display[i]->id, groups_to_display[i]->script->name);

    gtk_widget_hide(GTK_WIDGET(data));
    break;
  default:
    break;
  }

}

void neurons_manual_refresh(GtkWidget *pWidget, gpointer pdata)
{
  int i;

  (void) pWidget;
  (void) pdata;
  if (refresh_mode == REFRESH_MODE_MANUAL)
  {
    for (i = 0; i < number_of_groups_to_display; i++)
      pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_ONE : PANDORA_SEND_NEURONS_ONE), groups_to_display[i]->id, groups_to_display[i]->script->name);

  }
}

gboolean neurons_display_refresh_when_semi_automatic() //TODO : meilleur façon de faire?
{
  int i, j, current_stop = stop;

  if (refresh_mode != REFRESH_MODE_SEMI_AUTO) return FALSE;
  pthread_mutex_lock(&mutex_script_caracteristics);
  stop = TRUE;
  for (i = 0; i < number_of_groups_to_display; i++)
  {
    if (g_timer_elapsed(groups_to_display[i]->timer, NULL) > 2)
    {
      groups_to_display[i]->frequence_index_last = -1;
      for (j = 0; j < FREQUENCE_MAX_VALUES_NUMBER; j++)
        groups_to_display[i]->frequence_values[j] = -1;
      //group_expose_neurons(groups_to_display[i], TRUE, FALSE);
      //groups_to_display[i]->refresh_freq=FALSE;
      //gtk_widget_queue_draw (GTK_WIDGET(groups_to_display[i]->drawing_area));
    }
  }
  stop = current_stop;
  pthread_mutex_unlock(&mutex_script_caracteristics);
  return TRUE;
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

  (void) python_window;
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

void on_click_call_dialog(GtkWidget *pWidget, gpointer pdata) //TODO : ramener au cas 1 et copier coller? ou resoudre ce shtruc
{

  GtkWidget *path_list;
  GtkCellRenderer *render_text_column;
  GtkTreeViewColumn *text_column;
  GtkWidget *python_window, *pScrollbar;
  GtkTreeSelection *selection;
  GtkWidget *text_entry, *vbox, *content_area;
  long ID;

  (void) pWidget;

  ID = (long) pdata;

  python_window = NULL;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
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
  gtk_container_add(GTK_CONTAINER(pScrollbar), path_list);

  gtk_box_pack_start(GTK_BOX(vbox), pScrollbar, TRUE, TRUE, 1);

  text_entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(text_entry), MAX_LENGHT_FILENAME);
  g_object_set_data(G_OBJECT(python_window), "text_entry", (gpointer) text_entry);
  gtk_box_pack_start(GTK_BOX(vbox), text_entry, FALSE, TRUE, 1);
  // gtk_box_pack_start(GTK_BOX(GTK_DIALOG(python_window)->vbox),pScrollbar,TRUE,TRUE,1);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG (python_window));
  gtk_container_add(GTK_CONTAINER (content_area), vbox);

  gtk_widget_show_all(python_window);

}

void on_click_config(GtkWidget *button, gpointer pdata)
{
  GtkWidget *selection;

  (void) pdata;
  (void) button;
  selection = gtk_file_chooser_dialog_new("selectionner un repertoire",
  NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
  NULL);
  gtk_widget_show(selection);

  if (gtk_dialog_run(GTK_DIALOG (selection)) == GTK_RESPONSE_OK)
  {
    char *path;
    path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (selection));
    strcpy(python_path, path);
  }
  gtk_widget_destroy(selection);

}

//TODO: A voir si on peut utiliser la fonction de GTK3 pour l'ouverture d'une fenetre.
void on_click_extract_area(GtkWidget *button, gpointer pdata)
{
  static GtkWidget *pWindow;
  static GtkWidget *pframe_extract;
  //GtkWidget *v_box_second;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
  {

    pframe_extract = (GtkWidget*) pdata;
    pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_double_buffered(pWindow, TRUE);
    g_object_set_data(G_OBJECT(pWindow), "toggle_button", (gpointer) button);
    g_signal_connect(G_OBJECT(pWindow), "destroy", G_CALLBACK(on_destroy_new_window), pframe_extract);
    gtk_window_set_default_size(GTK_WINDOW(pWindow), 800, 800);

    gtk_widget_reparent(pframe_extract, GTK_WIDGET(pWindow));

    g_signal_connect(G_OBJECT(pWindow), "key_press_event", G_CALLBACK(key_press_event), NULL);

    gtk_widget_show_all(pWindow);

  }
  else
  {
    on_destroy_new_window(pWindow, pframe_extract);
    pWindow = NULL;
    pframe_extract = NULL;
  }

}

void pandora_window_new()
{
  char path[PATH_MAX];
  GtkWidget *h_box_main, *v_box_main, *v_box_inter, *pFrameEchelles, *pVBoxEchelles, *hbox_buttons, *hbox_barre, *refreshModeHBox, *refreshModeComboBox, *refreshModeLabel, *refreshManualButton, *refreshSetting, *refreshLabel, *xSetting, *xLabel, *menuBar,
      *fileMenu, *legend, *com;
  GtkWidget *h_box_save, *h_box_global, *h_box_network;
  GtkWidget *ySetting, *yLabel, *zxSetting, *zxLabel, *pFrameGroupes, *zySetting, *zyLabel, *saveLabel, *globalLabel, *networkLabel;
  GtkWidget *boutonDefault, *boutonPause, *save_button, *boutonTempsPhases, *save_path_button, *hide_see_legend_button, *call_python_button, *convert_matlab_button, *config_button, *windowed_area_button;
  GtkWidget *pPane, *lPane; //Panneaux latéraux
  GtkWidget *pFrameScripts, *scrollbars2, *textScrollbar;
  GtkWidget *load, *save, *saveAs, *quit, *itemFile;
  GtkWidget *arrow_im, *arrow_im_2;
  GtkWidget *notebook;
  GtkWidget *com_zone;

  //Méthode très particuliere, afin de ne pas se compliquer la tache lors du passage d'argument utilisant les deux expréssions ci dessous, on utilise le fait de pouvoir passer une adresse pour passer un int 0 ou 1 sous le format adresse.
  void* IDdialog_python = (void*) 0;
  void* IDdialog_matlab = (void*) 1;

  (void) p_buf;
  currently_saving_list = gtk_list_store_new(1, G_TYPE_STRING);

//La fenêtre principale

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_double_buffered(window, TRUE);
  /* Positionne la GTK_WINDOW "pWindow" au centre de l'écran */
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  /* Taille de la fenêtre */
  gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);

  window_title_update();
  sprintf(path, "%s/bin_leto_prom/resources/pandora_icon.png", getenv("HOME"));
  gtk_window_set_icon_from_file(GTK_WINDOW(window), path, NULL);

//Le signal de fermeture de la fenêtre est connecté à la fenêtre (petite croix)
  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(window_close), (GtkWidget* ) window);

// création de la barre de menus.
  menuBar = gtk_menu_bar_new(); //TODO : verif libé mémoire
  fileMenu = gtk_menu_new(); //TODO : verif libé mémoire

  neurons_frame = gtk_frame_new("Neurons' frame");

  load = gtk_menu_item_new_with_label(g_locale_to_utf8("Load", -1, NULL, NULL, NULL));
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), load);
  g_signal_connect(G_OBJECT(load), "activate", G_CALLBACK(pandora_load_preferences), NULL);

  save = gtk_menu_item_new_with_label(g_locale_to_utf8("Save", -1, NULL, NULL, NULL));
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), save);
  g_signal_connect(G_OBJECT(save), "activate", G_CALLBACK(save_preferences), NULL);

  saveAs = gtk_menu_item_new_with_label(g_locale_to_utf8("Save as", -1, NULL, NULL, NULL));
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveAs);
  g_signal_connect(G_OBJECT(saveAs), "activate", G_CALLBACK(save_preferences_as), NULL);

  quit = gtk_menu_item_new_with_label(g_locale_to_utf8("Quit", -1, NULL, NULL, NULL));
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quit);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(window_close), (GtkWidget* ) window);

  itemFile = gtk_menu_item_new_with_label("File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemFile), fileMenu);

  gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemFile);

  /*Création d'une VBox (boîte de widgets disposés verticalement) */
  v_box_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  ;
  /*ajout de v_box_main dans pWindow, qui est alors vu comme un GTK_CONTAINER*/
  gtk_container_add(GTK_CONTAINER(window), v_box_main);

  gtk_box_pack_start(GTK_BOX(v_box_main), menuBar, FALSE, FALSE, 0); // ajout de la barre de menus.

  // Création du systemes d'onglets pour la barre d'outils
  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

  h_box_save = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(h_box_save), TRUE);
  h_box_global = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(h_box_global), TRUE);
  h_box_network = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(h_box_network), TRUE);

  saveLabel = gtk_label_new("Saving");
  globalLabel = gtk_label_new("General");
  networkLabel = gtk_label_new("Network");

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), h_box_save, saveLabel);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), h_box_global, globalLabel);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), h_box_network, networkLabel);

  gtk_box_pack_start(GTK_BOX(v_box_main), notebook, FALSE, FALSE, 0);

  //Creation du bouton de changement de chemin
  save_path_button = gtk_button_new_with_label("Define save path");
  g_signal_connect(G_OBJECT(save_path_button), "clicked", G_CALLBACK(on_click_save_path_button), NULL);
  gtk_box_pack_start(GTK_BOX(h_box_save), save_path_button, FALSE, FALSE, 0);

  //Creation du bouton d'enregistrement
  save_button = gtk_toggle_button_new_with_label("Launch data saving");
  g_signal_connect(G_OBJECT(save_button), "toggled", G_CALLBACK(on_toggled_saving_button), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_button), FALSE);
  gtk_box_pack_start(GTK_BOX(h_box_save), save_button, FALSE, FALSE, 0);

  //Creation du bouton d'appel python
  call_python_button = gtk_button_new_with_label("Call python script");
  g_signal_connect(G_OBJECT(call_python_button), "clicked", G_CALLBACK(on_click_call_dialog), (gpointer )IDdialog_python);
  gtk_box_pack_start(GTK_BOX(h_box_save), call_python_button, FALSE, FALSE, 0);

  //Creation du bouton de conversion matlab
  convert_matlab_button = gtk_button_new_with_label("Convert to Matlab");
  g_signal_connect(G_OBJECT(convert_matlab_button), "clicked", G_CALLBACK(on_click_call_dialog), (gpointer )IDdialog_matlab);
  gtk_box_pack_start(GTK_BOX(h_box_save), convert_matlab_button, FALSE, FALSE, 0);

  //Creation du bouton de config
  config_button = gtk_button_new_with_label("Define script path");
  g_signal_connect(G_OBJECT(config_button), "clicked", G_CALLBACK(on_click_config), NULL);
  gtk_box_pack_start(GTK_BOX(h_box_save), config_button, FALSE, FALSE, 0);

  boutonTempsPhases = gtk_toggle_button_new_with_label("start calculate execution times");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(boutonTempsPhases), FALSE);
  gtk_box_pack_start(GTK_BOX(h_box_global), boutonTempsPhases, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(boutonTempsPhases), "toggled", G_CALLBACK(phases_info_start_or_stop), NULL);

  windowed_area_button = gtk_toggle_button_new_with_label("windowed draw");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(windowed_area_button), FALSE);
  gtk_box_pack_start(GTK_BOX(h_box_global), windowed_area_button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(windowed_area_button), "toggled", G_CALLBACK(on_click_extract_area), (gpointer )neurons_frame);
  //TODO : modifier la façon dont est créé cette fenetre pour utiliser la fonctionalité de GTK3 -> la fenetre detachable

  /*Création de deux HBox : une pour le panneau latéral et la zone principale, l'autre pour les 6 petites zones*/
  h_box_main = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_position(GTK_PANED(vpaned), 600);
  gtk_box_pack_start(GTK_BOX(v_box_main), h_box_main, TRUE, TRUE, 0);

  /*Panneau latéral*/
  pPane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  ;
  lPane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(h_box_main), pPane, FALSE, TRUE, 0);

//Les échelles
  pFrameEchelles = gtk_frame_new("Scales");
  gtk_container_add(GTK_CONTAINER(pPane), pFrameEchelles);
  pVBoxEchelles = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  ;
  gtk_container_add(GTK_CONTAINER(pFrameEchelles), pVBoxEchelles);

  // Mode de réactualisation des groupes de neurones contenus dans le panneau du bas.
  refreshModeHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshModeHBox, FALSE, TRUE, 0);
  refreshModeLabel = gtk_label_new("Refresh mode : ");
  refreshModeComboBox = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refreshModeComboBox), "Auto");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refreshModeComboBox), "Semi-auto");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refreshModeComboBox), "Manual");
  gtk_combo_box_set_active(GTK_COMBO_BOX(refreshModeComboBox), REFRESH_MODE_AUTO);
  gtk_box_pack_start(GTK_BOX(refreshModeHBox), refreshModeLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(refreshModeHBox), refreshModeComboBox, TRUE, TRUE, 0);

  refreshManualButton = gtk_button_new_with_label("Refresh");
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshManualButton, FALSE, TRUE, 0);

  //g_signal_connect(G_OBJECT(refreshManualButton), "realize", (GCallback) gtk_widget_hide, NULL); // façon un peu orginale de cacher le widget à l'initialisation.

//Fréquence de réactualisation de l'affichage, quand on est en mode échantillonné (Sampled)
  refreshSetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshSetting, FALSE, TRUE, 0);
  refreshLabel = gtk_label_new("Refresh (Hz):");
  refreshScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 50, 1); //Ce widget est déjà déclaré comme variable globale
//On choisit le nombre de réactualisations de l'affichage par seconde, entre 0 et 50
  gtk_box_pack_start(GTK_BOX(refreshSetting), refreshLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(refreshSetting), refreshScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(refreshScale), REFRESHSCALE_DEFAULT);
  g_signal_connect(G_OBJECT(refreshScale), "change-value", G_CALLBACK(refresh_scale_change_value), NULL);

  g_signal_connect(G_OBJECT(refreshModeComboBox), "changed", (GCallback ) refresh_mode_combo_box_value_changed, refreshManualButton);
  g_signal_connect(G_OBJECT(refreshManualButton), "clicked", (GCallback ) neurons_manual_refresh, NULL);

//Echelle de l'axe des x
  xSetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), xSetting, FALSE, TRUE, 0);
  xLabel = gtk_label_new("x scale:");
  xScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, XSCALE_MIN, XSCALE_MAX, 1); //Ce widget est déjà déclaré comme variable globale
  gtk_box_pack_start(GTK_BOX(xSetting), xLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(xSetting), xScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(xScale), graphic.x_scale);
  g_signal_connect(G_OBJECT(xScale), "change-value", (GCallback ) on_scale_change_value, &graphic.x_scale);

//Echelle de l'axe des y
  ySetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), ySetting, FALSE, TRUE, 0);
  yLabel = gtk_label_new("y scale:");
  yScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, YSCALE_MIN, YSCALE_MAX, 1); //Ce widget est déjà déclaré comme variable globale
  gtk_box_pack_start(GTK_BOX(ySetting), yLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(ySetting), yScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(yScale), graphic.y_scale);
  g_signal_connect(G_OBJECT(yScale), "change-value", (GCallback ) on_scale_change_value, &graphic.y_scale);

//Décalage des plans selon x
  zxSetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zxSetting, FALSE, TRUE, 0);
  zxLabel = gtk_label_new("x gap:");
  zxScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 200, 1); //Ce widget est déjà déclaré comme variable globale
  gtk_box_pack_start(GTK_BOX(zxSetting), zxLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(zxSetting), zxScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(zxScale), graphic.zx_scale);
  g_signal_connect(G_OBJECT(zxScale), "change-value", (GCallback ) on_scale_change_value, &graphic.zx_scale);

//Décalage des plans selon y
  zySetting = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  ;
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zySetting, FALSE, TRUE, 0);
  zyLabel = gtk_label_new("y gap:");
  zyScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 2000, 1); //Ce widget est déjà déclaré comme variable globale
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
  g_signal_connect(G_OBJECT(check_button_draw_connections), "toggled", G_CALLBACK(on_check_button_draw_active), NULL); //TODO : heuuuuu? ? ? ?
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

  //ceation d'une Vbox qui contiendra le paned de la zone affichage et les boutons d'ouverture des cotés
  v_box_inter = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
  ;
  //Création d'une Hbox qui contiendra les boutons suivants
  hbox_barre = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
  ;

  // Creation des deux boutons de reduction des panneaux gauche et droits.
  hide_see_scales_button = gtk_toggle_button_new();
  arrow_im = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU);
  g_object_set_data(G_OBJECT(hide_see_scales_button), "arrow_im", GTK_WIDGET(arrow_im));
  gtk_container_add(GTK_CONTAINER (hide_see_scales_button), arrow_im);
  g_signal_connect(G_OBJECT(hide_see_scales_button), "toggled", G_CALLBACK(on_hide_see_scales_button_active), pPane);
  gtk_box_pack_start(GTK_BOX(hbox_barre), hide_see_scales_button, FALSE, TRUE, 0);

  hide_see_legend_button = gtk_toggle_button_new();
  arrow_im_2 = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
  g_object_set_data(G_OBJECT(hide_see_legend_button), "arrow_im_2", arrow_im_2);
  gtk_container_add(GTK_CONTAINER (hide_see_legend_button), arrow_im_2);
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

  gtk_widget_set_events(architecture_display, GDK_SCROLL_MASK | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK); //TODO : remplacer par add , Détecte quand on appuie OU quand on relache un bouton de la souris alors que le curseur est dans la zone3D
  g_signal_connect(G_OBJECT(architecture_display), "scroll-event", G_CALLBACK(architecture_display_scroll_event), NULL);
  g_signal_connect(G_OBJECT(architecture_display), "draw", G_CALLBACK(architecture_display_update), NULL);
  // gtk_widget_set_events(architecture_display, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK); //TODO : remplacer par add , Détecte quand on appuie OU quand on relache un bouton
  //gtk_signal_connect(GTK_OBJECT(architecture_display), "button-press-event", (GtkSignalFunc) button_press_event, NULL);
  g_signal_connect(G_OBJECT(architecture_display), "button-press-event", G_CALLBACK(architecture_display_button_pressed), NULL);
  g_signal_connect(G_OBJECT(architecture_display), "button-release-event", G_CALLBACK(architecture_display_button_released), NULL);
  g_signal_connect(G_OBJECT(architecture_display), "motion-notify-event", G_CALLBACK(architecture_display_drag_motion), NULL);
  g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_event), NULL);

//la zone des groupes de neurones
  gtk_container_add(GTK_CONTAINER(vpaned), neurons_frame);
  scrollbars2 = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(neurons_frame), scrollbars2);

  zone_neurons = gtk_layout_new(NULL, NULL);
  gtk_widget_set_size_request(GTK_WIDGET(zone_neurons), 3000, 3000);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars2), zone_neurons);
  //gtk_widget_set_events(zone_neurons, GDK_BUTTON_RELEASE_MASK);
  //g_signal_connect(G_OBJECT(zone_neurons), "button-release-event", G_CALLBACK(on_release), NULL);

  gtk_box_pack_start(GTK_BOX(v_box_inter), vpaned, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(h_box_main), v_box_inter, TRUE, TRUE, 0);

  /*Panneau latéral droit*/

  legend = gtk_frame_new("Legends");
  com = gtk_frame_new("Prompt");
  com_zone = gtk_text_view_new();
  p_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW (com_zone));
  textScrollbar = gtk_scrolled_window_new(NULL, NULL);

  gtk_widget_set_size_request(GTK_WIDGET(textScrollbar), 240, 600);
  gtk_text_view_set_editable( GTK_TEXT_VIEW(com_zone), FALSE);

  gtk_container_add(GTK_CONTAINER(textScrollbar), com_zone);
  gtk_container_add(GTK_CONTAINER(com), textScrollbar);
  gtk_box_pack_start(GTK_BOX(lPane), legend, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(lPane), com, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h_box_main), lPane, FALSE, TRUE, 0);

  gtk_widget_show_all(window); //Affichage du widget pWindow et de tous ceux qui sont dedans
  gtk_widget_hide(refreshManualButton);
  // On cache la fenetre au départ
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_see_legend_button), TRUE);
  on_hide_see_legend_button_active(hide_see_legend_button, (gpointer*) lPane);
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
  //GtkSettings *default_settings = gtk_settings_get_default();
  // g_object_set(default_settings, "gtk-button-images", TRUE, NULL);

  stop = FALSE;
  load_temporary_save = FALSE;
  architecture_display_dragging_currently = FALSE;
  refresh_mode = REFRESH_MODE_AUTO;
  graphic.x_scale = XSCALE_DEFAULT;
  graphic.y_scale = YSCALE_DEFAULT;
  graphic.zx_scale = XGAP_DEFAULT;
  graphic.zy_scale = YGAP_DEFAULT;

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

  //Préparation pour le lancement de la lecture des arguments
  bus_ip[0] = 0;
  bus_id[0] = 0;
  preferences_filename[0] = 0;

  //On regarde les options passées en ligne de commande
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

  //On regarde les fichier passés en ligne de commande
  if (optind < argc)
  {
    if (strstr(argv[optind], ".pandora"))
    {
      strcpy(preferences_filename, argv[optind]);
    }
  }

  prom_bus_init(bus_ip);

//g_thread_init(NULL); /* useless since glib 2.32 */
  gdk_threads_init();

  pthread_mutex_init(&mutex_script_caracteristics, NULL);
  pthread_mutex_init(&mutex_copy_arg_top, NULL);
  pthread_mutex_init(&mutex_loading, NULL);
  pthread_mutex_init(&mutex_copy_arg_group_display, NULL);

// Initialisation de GTK+
  gtk_init(&argc, &argv);

//Initialisation d'ENet
  if (enet_initialize() != 0)
  {
    printf("An error occurred while initializing ENet.\n");
    exit(EXIT_FAILURE);
  }

//Lancement du thread d'écoute et de gestion des informations reçues du réseau
  server_for_promethes();

//Création de la fenetre GTK principale, disposition des boutons etc...
  pandora_window_new();

//si après chargement il n'y a pas de bus_id
  if ((bus_id[0] == 0) || (bus_ip[0] == 0))
  {
    EXIT_ON_ERROR("You miss bus_ip or bus_id \n\tUsage: %s [-b bus_ip -i bus_id] \n", argv[0]);
  }

  //TODO : erreur ici : on peut recevoir les infos de scripts après le chargement des script par le serveur enet selon la rapidité du thread.
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

//Appelle la fonction de raffraichissement, voir la fonction lié à l'event refresh_mode_combo_box_changed pour plus de details
  refresh_mode = REFRESH_MODE_AUTO;
  if (refresh_timer_id != 0)
  {
    g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
    refresh_timer_id = 0;
  }
  if (id_semi_automatic != 0)
  {
    g_source_destroy(g_main_context_find_source_by_id(NULL, id_semi_automatic));
    id_semi_automatic = 0;
  }
  if (refresh_timer_id == 0 && value_hz > 0.1) refresh_timer_id = g_timeout_add((guint) (1000.0 / value_hz), neurons_refresh_display, NULL);
      else if (refresh_timer_id == 0) refresh_timer_id = 0;

  gdk_threads_enter();
  gtk_main(); //Boucle infinie : attente des événements
  gdk_threads_leave();
  return EXIT_SUCCESS;
}
