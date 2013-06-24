/* pandora.c
 *
 *
 * Auteurs : Brice Errandonea et Manuel De Palma
 *
 * Pour compiler : make
 *
 *
 */
#include "pandora.h"
#include "pandora_ivy.h"
#include "graphic.h"
#include "basic_tools.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char label_text[LABEL_MAX];

GtkWidget *window = NULL; //La fenêtre de l'application Pandora
GtkWidget *selected_group_dialog;
GtkWidget *vpaned, *scrollbars;
GtkWidget *hide_see_scales_button; //Boutton permettant de cacher le menu de droite
GtkWidget *pPane; //Panneau latéral
GtkWidget *pVBoxScripts; //Panneau des scripts
GtkWidget *architecture_display; //La grande zone de dessin des liaisons entre groupes
GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale; //Échelles
    /*Indiquent quel est le mode d'affichage en cours (Off-line, Sampled ou Snapshots)*/
const char *displayMode;
GtkWidget *modeLabel;
int currentSnapshot;
int nbSnapshots;

int Index[NB_SCRIPTS_MAX]; /*Tableau des indices : Index[0] = 0, Index[1] = 1, ..., Index[NB_SCRIPTS_MAX-1] = NB_SCRIPTS_MAX-1;
 Ce tableau permet à une fonction signal de retenir la valeur qu'avait i au moment où on a connecté le signal*/

int number_of_scripts = 0; //Nombre de scripts à afficher
type_script *scripts[NB_SCRIPTS_MAX]; //Tableau des scripts à afficher
int zMax; //la plus grande valeur de z parmi les scripts ouverts
int buffered = 0; //Nombre d'instantanés actuellement en mémoire
int period;

type_group *selected_group = NULL; //Pointeur sur le groupe actuellement sélectionné

GtkWidget *neurons_frame, *zone_neurons; //Panneau des neurones

int nbColonnesTotal = 0; //Nombre total de colonnes de neurones dans les fenêtres du bandeau du bas
int nbLignesMax = 0; //Nombre maximal de lignes de neurones à afficher dans l'une des fenêtres du bandeau du bas


int move_neurons_old_x, move_neurons_old_y, move_neurons_start = 0;
type_group *move_neurons_group = NULL; //Pour bouger un frame_neuron
int open_neurons_start = 0; //Pour ouvrir un frame_neuron
type_group *open_group = NULL;

guint refresh_timer_id; //id du timer actualement utiliser pour le rafraichissement des frame_neurons ouvertes

type_script_link net_links[SCRIPT_LINKS_MAX];
int number_of_net_links = 0;
type_group *groups_to_display[NB_WINDOWS_MAX];
int number_of_groups_to_display = 0;

char bus_id[BUS_ID_MAX];
char bus_ip[HOST_NAME_MAX];


void on_search_group(int index);
void group_display_new(type_group *group, float pos_x, float pos_y);
void group_display_destroy(type_group *group);
gboolean neurons_refresh_display_without_change_values();


float**** createTab4(int nbRows, int nbColumns)
{
    float **** tab = malloc(nbRows * sizeof(float ***));
    int i, j, k, l;
    for(i=0; i<nbRows; i++)
    {
        tab[i] = malloc(nbColumns * sizeof(float **));
        for(j=0; j<nbColumns; j++)
        {
            tab[i][j] = malloc(3 * sizeof(float *));
            for(k=0; k<3; k++)
            {
                tab[i][j][k] = malloc(NB_Max_VALEURS_ENREGISTREES * sizeof(float));
                for(l=0; l<NB_Max_VALEURS_ENREGISTREES; l++)
                    tab[i][j][k][l] = 0.0;// initialisation du tableau multi-dimensionnel.
            }
        }
    }
    return tab;
}

void destroy_tab_4(float **** tab, int nbColumns, int nbRows)
{
    int i, j, k;
    for(i=0; i<nbRows; i++)
    {
        for(j=0; j<nbColumns; j++)
        {
            for(k=0; k<3; k++)
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
    for(i=0; i<nbRows; i++)
    {
        tab[i] = malloc(nbColumns * sizeof(int));
        for(j=0; j<nbColumns; j++)
            tab[i][j] = init_value;
    }
    return tab;
}

void destroy_tab_2(int **tab, int nbRows)
{
    int i;
    for(i=0; i<nbRows; i++)
    {
        free(tab[i]);
    }
    free(tab);
}



void pandora_quit()
{
  pandora_bus_send_message(bus_id, "pandora(%d,0)", PANDORA_STOP);
  enet_deinitialize();
  gtk_main_quit();
  gtk_widget_destroy(window);
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

//-----------------------------------------------------4. SIGNAUX---------------------------------------------
void on_script_displayed_toggled(GtkWidget *pWidget, gpointer user_data)
{
  type_script *script = user_data;

  //Identification du script à afficher ou à masquer
  script->displayed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));

  architecture_display_update(architecture_display, NULL);
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

  pandora_file_save("./pandora.pandora"); // enregistrement de l'état actuel. cet état sera appliqué au prochain démarrage de pandora.
  gtk_main_quit();
  return FALSE;
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

  for(i=0; i<number_of_scripts; i++)
	  script_update_positions(scripts[i]);

  architecture_display_update(architecture_display, NULL); //On redessine avec la nouvelle valeur de z
}

gboolean on_scale_change_value(GtkWidget *gtk_range, GtkScrollType scroll, gdouble value, float* value_pointer)
{
  (void) gtk_range;
  (void) scroll;
  *value_pointer = (float) value;

  architecture_display_update(architecture_display, NULL);
  return FALSE;
}

/**
 *
 * Modification d'une échelle
 *
 */
gboolean refresh_scale_change_value(GtkWidget *pWidget, GtkScrollType scroll, gdouble value, gpointer pData) //Modification d'une échelle
{
  (void) pWidget;
  (void) scroll;
  (void) pData;

  if (refresh_timer_id) g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
  if (value > 0.1) refresh_timer_id = g_timeout_add((guint)(1000 / value), neurons_refresh_display, NULL);
  else refresh_timer_id = 0;
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
void zoom_in()
{
    int i, j, x_rel, y_rel;
    float x, y, previous_xscale = graphic.x_scale, previous_yscale = graphic.y_scale;

    gtk_widget_get_pointer(scrollbars, &x_rel, &y_rel); // permet de savoir les coordonnées du pointeur de la souris.

    x = (float) gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars))->value + x_rel; // scrollbars->allocation.width/2;
    y = (float) gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars))->value + y_rel; //scrollbars->allocation.height/2;

    // calcul des coordonnées du nouveau point pointé par la souris avec les nouvelles échelles.

    for(i=0; (i+1) * (LARGEUR_GROUPE + graphic.x_scale) < x;i++);
    for(j=0; (j+1) * (HAUTEUR_GROUPE + graphic.y_scale) < y; j++);
    x -= i * (LARGEUR_GROUPE + previous_xscale);
    y -= j * (HAUTEUR_GROUPE + previous_yscale);

    graphic.x_scale += ZOOM_GAP;
    if(graphic.x_scale > XSCALE_MAX)
        graphic.x_scale = XSCALE_MAX;

    graphic.y_scale += ZOOM_GAP;
    if(graphic.y_scale > YSCALE_MAX)
        graphic.y_scale = YSCALE_MAX;

    x = i * (LARGEUR_GROUPE + graphic.x_scale) + x * graphic.x_scale / previous_xscale - x_rel;
    y = j * (HAUTEUR_GROUPE + graphic.y_scale) + y * graphic.y_scale / previous_yscale - y_rel;

    // application des nouvelles coordonnées du point en haut à gauche de la zone à afficher et propagation du changement de ces valeurs.
    gtk_range_set_value(GTK_RANGE(xScale), graphic.x_scale);
    gtk_range_set_value(GTK_RANGE(yScale), graphic.y_scale);
    architecture_set_view_point(scrollbars, x, y);
    architecture_display_update(architecture_display, NULL); //On redessine la grille avec la nouvelle échelle.
}

// zoom arrière sur la zone "architecture_display".
void zoom_out()
{
    int i, j, x_rel, y_rel;
    float x, y, previous_xscale = graphic.x_scale, previous_yscale = graphic.y_scale;

    gtk_widget_get_pointer(scrollbars, &x_rel, &y_rel);

    x = (float) gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars))->value + x_rel; // scrollbars->allocation.width/2;
    y = (float) gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars))->value + y_rel; //scrollbars->allocation.height/2;

    for(i=0; (i+1) * (LARGEUR_GROUPE + graphic.x_scale) < x;i++);
    for(j=0; (j+1) * (HAUTEUR_GROUPE + graphic.y_scale) < y; j++);
    x -= i * (LARGEUR_GROUPE + previous_xscale);
    y -= j * (HAUTEUR_GROUPE + previous_yscale);

    graphic.x_scale -= ZOOM_GAP;
    if(graphic.x_scale < XSCALE_MIN)
        graphic.x_scale = XSCALE_MIN;

    graphic.y_scale -= ZOOM_GAP;
    if(graphic.y_scale < YSCALE_MIN)
        graphic.y_scale = YSCALE_MIN;



    x = i * (LARGEUR_GROUPE + graphic.x_scale) + x * graphic.x_scale / previous_xscale - x_rel; // scrollbars->allocation.width/2;
    y = j * (HAUTEUR_GROUPE + graphic.y_scale) + y * graphic.y_scale / previous_yscale - y_rel; //scrollbars->allocation.height/2;scrollbars->allocation.height/2;

    gtk_range_set_value(GTK_RANGE(xScale), graphic.x_scale);
    gtk_range_set_value(GTK_RANGE(yScale), graphic.y_scale);
    architecture_set_view_point(scrollbars, x, y);
    architecture_display_update(architecture_display, NULL); //On redessine la grille avec la nouvelle échelle.
}

void architecture_display_button_pressed(GtkWidget *pWidget, GdkEventButton *event, gpointer pdata)
{
    (void) pWidget;
    (void) pdata;

    if(event->button == 2)   // boutton molette
        architecture_display_dragging_currently = TRUE;

    architecture_display_cursor_x = (float) event->x;
    architecture_display_cursor_y = (float) event->y;
    new_x = ((float) gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars))->value);
    new_y = ((float) gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars))->value);
}

void architecture_display_button_released(GtkWidget *pWidget, GdkEventButton *event, gpointer pdata)
{
    (void) pWidget;
    (void) pdata;

    if(event->button == 2)   // boutton molette
        architecture_display_dragging_currently = FALSE;

    architecture_display_cursor_x = (float) event->x;
    architecture_display_cursor_y = (float) event->y;
}

// permet de déplacer la zone à afficher s'active en appuyant sur la molette.
void architecture_display_drag_motion(GtkWidget *pWidget, GdkEventMotion *event, gpointer pdata)
{
    (void) pWidget;
    (void) pdata;

    if(architecture_display_dragging_currently == FALSE)
      return;

    new_x = new_x - ((float) event->x) + architecture_display_cursor_x;
    if(new_x < 0) new_x = 0.0;
    else if(new_x > 10000 - scrollbars->allocation.width) new_x = (float) 10000 - scrollbars->allocation.width;
    new_y = new_y - ((float) event->y) + architecture_display_cursor_y;
    if(new_y < 0) new_y = 0.0;
    else if(new_y > 10000 - scrollbars->allocation.height) new_y = (float) 10000 - scrollbars->allocation.height;

    architecture_set_view_point(scrollbars, new_x, new_y);
    architecture_display_cursor_x = (float) event->x;
    architecture_display_cursor_y = (float) event->y;
}

// ouvre la fenetre de recherche en sélectionnant par défaut le script passé en paramètre.
void on_search_group_button_active(GtkWidget *pWidget, type_script *script)
{
    int i, index = 0;
    (void) pWidget;

    for(i=0; i<number_of_scripts && scripts[i] != script; i++);

    index = (i==number_of_scripts ? 0 : i);

    on_search_group(index);
}

//
void on_search_group(int index)
{
  static type_script *script = NULL;
  static int index_script = 0;

  float x = 0.0, y = 0.0;
  GtkWidget *search_dialog, *search_entry, *h_box, *scripts_h_box, *name_radio_button_search, *function_radio_button_search, *scripts_combo_box, *scripts_label, *search_label, *search_h_box;
  int i, last_occurence = 0, stop = 0, previous_index = -1;
  type_script *selected_script = NULL;
  char previous_research[TAILLE_CHAINE];

  if(number_of_scripts <= 0)
    return;

  if(index >= 0 && index < number_of_scripts)
    index_script = index;
  else
    for(i=0; i < number_of_scripts; i++)
      if(scripts[i] == script)
        index_script = i;

  previous_research[0] = '\0';

//On crée une fenêtre de dialogue
  search_dialog = gtk_dialog_new_with_buttons("Recherche d'un groupe", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  gtk_window_set_default_size(GTK_WINDOW(search_dialog), 300, 75);

  name_radio_button_search = gtk_radio_button_new_with_label_from_widget(NULL, "by name");
  function_radio_button_search = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(name_radio_button_search), "by function");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(name_radio_button_search), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(function_radio_button_search), FALSE);


  h_box = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h_box), name_radio_button_search, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h_box), function_radio_button_search, TRUE, FALSE, 0);

  scripts_label = gtk_label_new("script : ");

  scripts_combo_box = gtk_combo_box_new_text();
  for(i=0; i< number_of_scripts; i++)
    gtk_combo_box_append_text(GTK_COMBO_BOX(scripts_combo_box), scripts[i]->name);
  gtk_combo_box_set_active(GTK_COMBO_BOX(scripts_combo_box), index_script);

  scripts_h_box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(scripts_h_box), scripts_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(scripts_h_box), scripts_combo_box, TRUE, TRUE, 0);


  search_label = gtk_label_new("");
//On lui ajoute une entrée de saisie de texte
  search_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(search_entry), "");
  g_signal_connect(G_OBJECT(search_entry), "activate", G_CALLBACK(validate_gtk_entry_text), search_dialog);
  //g_signal_connect(G_OBJECT(search_entry), "clicked", G_CALLBACK(show_or_mask_default_text), search_dialog);
  search_h_box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(search_h_box), search_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(search_h_box), search_entry, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(search_dialog)->vbox), h_box, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(search_dialog)->vbox), scripts_h_box, TRUE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(search_dialog)->vbox), search_h_box, TRUE, FALSE, 0);

  gtk_widget_show_all(GTK_DIALOG(search_dialog)->vbox);

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
      if(previous_index != gtk_combo_box_get_active(GTK_COMBO_BOX(scripts_combo_box)))
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
            x -= (scrollbars->allocation.width - LARGEUR_GROUPE)/2;
            if(x < 0.0) x = 0.0;
            y -= (scrollbars->allocation.height - HAUTEUR_GROUPE)/2;
            if(y < 0.0) y = 0.0;
            architecture_set_view_point(scrollbars, x, y);
            break;
          }
        }
        stop = 1;
      }
      else
      {
        if(strcmp(previous_research, gtk_entry_get_text(GTK_ENTRY(search_entry))))
          {last_occurence = 0; strcpy(previous_research, gtk_entry_get_text(GTK_ENTRY(search_entry)));}

        for (i = last_occurence; i < selected_script->number_of_groups; i++)
        {
          if (strcmp(gtk_entry_get_text(GTK_ENTRY(search_entry)), selected_script->groups[i].function) == 0)
          {
            //Le groupe correspondant est séléctionné
            selected_group = &selected_script->groups[i];
            // les coordonnées de ce groupe sont récupérées et le groupe est placé au centre de l'affichage.
            architecture_get_group_position(selected_group, &x, &y);
            x -= (scrollbars->allocation.width - LARGEUR_GROUPE)/2;
            if(x < 0.0) x = 0.0;
            y -= (scrollbars->allocation.height - HAUTEUR_GROUPE)/2;
            if(y < 0.0) y = 0.0;

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
    architecture_display_update(architecture_display, NULL);
  } while (stop != 1);

  index_script = gtk_combo_box_get_active(GTK_COMBO_BOX(scripts_combo_box));
  gtk_widget_destroy(search_dialog);
}

void on_hide_see_scales_button_active(GtkWidget *hide_see_scales_button, gpointer pData)
{
  (void) pData;

//Si le bouton est activé
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_see_scales_button)))
  {
    //On cache le menu de droite
    gtk_widget_hide(pPane);
    //On actualise le label du boutton
    gtk_button_set_label(GTK_BUTTON(hide_see_scales_button), "Hide left panel");
  }
//Si le bouton est désactivé
  else
  {
    //On montre le menu de droite
    gtk_widget_show(pPane);
    //On actualise le label du boutton
    gtk_button_set_label(GTK_BUTTON(hide_see_scales_button), "Show left panel");
  }
}

void on_check_button_draw_active(GtkWidget *check_button, gpointer data)
{
  (void) check_button;
  (void) data;

  architecture_display_update(architecture_display, NULL);
}

/**
 *
 * Clic souris
 *
 */
void button_press_event(GtkWidget *pWidget, GdkEventButton *event)
{
  (void) pWidget;

  selected_group = NULL;
  architecture_display_update(architecture_display, event);
  architecture_display_update(architecture_display, NULL);

  if (event->button == 3)
  {
    open_neurons_start = 1;
    move_neurons_start = 0;
    open_group = selected_group;

    if(open_group != NULL)
      group_display_new(open_group, 25, 25);
  }
  else
      move_neurons_start = open_neurons_start = 0;
}

// permet la gestion du zoom avec CTRL + molette. Molette vers le haut pour le zoom avant et vers le bas pour le zoom arrière.
gboolean architecture_display_scroll_event(GtkWidget *pWidget, GdkEventScroll *event, gpointer pdata)
{
    (void) pWidget;
    (void) pdata;

    if(event->device->source != GDK_SOURCE_MOUSE)
      return FALSE;

    if(event->direction == GDK_SCROLL_UP && event->state & GDK_CONTROL_MASK)
    {
        zoom_in();
        return TRUE;
    }

    if(event->direction == GDK_SCROLL_DOWN && event->state & GDK_CONTROL_MASK)
    {
        zoom_out();
        return TRUE;
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
  (void) pWidget;

  switch (event->keyval)
  {
  case GDK_Up: //Touche "Page up", on réduit le y du groupe sélectionné pour le faire monter
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
      architecture_display_update(architecture_display, NULL);
    }
    break;
  case GDK_Down: //Touche "Page down", on augmente le y du groupe sélectionné pour le faire descendre
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
      architecture_display_update(architecture_display, NULL);
    }
    break;

  case GDK_f : if(event->state & GDK_CONTROL_MASK) on_search_group(-1);
  break;
  case GDK_s : if(event->state & GDK_CONTROL_MASK) save_preferences(NULL, NULL);
  break;
  case GDK_S : if(event->state & GDK_CONTROL_MASK) save_preferences_as(NULL, NULL);
  break;
  case GDK_KP_Add :
  case GDK_plus : if(event->state & GDK_CONTROL_MASK) zoom_in();
  break;
  case GDK_minus :
  case GDK_KP_Subtract : if(event->state & GDK_CONTROL_MASK) zoom_out();
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
    switch(gtk_dialog_run(GTK_DIALOG(confirmation)))
    {
        case GTK_RESPONSE_YES :
            gtk_widget_destroy(confirmation);
        break;
        default :
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

  architecture_display_update(architecture_display, NULL); //On redessine la grille avec la nouvelle échelle.
  refresh_scale_change_value(NULL, GTK_SCROLL_NONE, (gdouble) REFRESHSCALE_DEFAULT, NULL); // modifie la fréquence de rafraichissement.
}

void on_group_display_output_combobox_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = data;
  int new_output = gtk_combo_box_get_active(combo_box);
  if(new_output == 3 && group->output_display != 3)
  {
	  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, group->script->name);
	  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_START, group->id, group->script->name);
  }
  else if(new_output != 3 && group->output_display == 3)
  {

	  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_STOP, group->id, group->script->name);
	  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, group->script->name);
  }
  group->output_display = gtk_combo_box_get_active(combo_box);
}

void on_group_display_mode_combobox_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = data;
  group->previous_display_mode = group->display_mode;
  group->display_mode = gtk_combo_box_get_active(combo_box);
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

void on_group_display_clicked(GtkButton *button, type_group *group)
{
  char builder_file_name[PATH_MAX];
  GError *g_error = NULL;
  GtkBuilder *builder;
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

  gtk_widget_show_all(selected_group_dialog);

}

int button_press_neurons(GtkWidget *zone2D, GdkEventButton *event, type_group *group)
{
  (void) zone2D;
  if (event->button == 2) //Si clic molette dans une petite fenêtre, on la supprime
  {
    group_display_destroy(group);

    /* Suppresion du groupe a afficher */

  }
  else if (event->button == 3) //Si droit, la fenêtre peut être déplacée
  {
    move_neurons_old_x = event->x;
    move_neurons_old_y = event->y;
    move_neurons_start = 1;
    move_neurons_group = group;
  }
  else if (event->button == 1) //Si clic gauche la fenêtre est séléctionnée
  {
    selected_group = group; //Le groupe associé à la fenêtre dans laquelle on a cliqué est également sélectionné
    on_group_display_clicked(NULL, group);
  }

//On actualise l'affichage
  architecture_display_update(architecture_display, NULL);

  return TRUE;
}


void graph_stop_or_not(GtkWidget *pWidget, gpointer pData)
{
    (void) pData;
    stop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));
    if(stop == TRUE)
        gtk_button_set_label(GTK_BUTTON(pWidget), "Record graphs");
    else
        gtk_button_set_label(GTK_BUTTON(pWidget), "Stop graphs");
}

void on_group_display_show_or_mask_neuron(GtkWidget *pWidget, gpointer pData)
{
    int *data = pData;
    *data = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));
}

void group_display_new(type_group *group, float pos_x, float pos_y)
{
  GtkWidget *hbox, *principal_hbox, *button, *button_label;
  int i, j;
  float r, g, b;
  char p_legende_texte[100];
  if (group->widget != NULL) return;

  group->label = gtk_label_new("");

  //initialisation
  if(group->rows <= 10 && group->columns <= 10)
  {
    group->tabValues = createTab4(group->rows, group->columns);
    group->indexAncien = createTab2(group->rows, group->columns, -1);
    group->indexDernier = createTab2(group->rows, group->columns, -1);
    if(group->rows * group->columns <= BIG_GRAPH_MAX_NEURONS_NUMBER)
        group->afficher = createTab2(group->rows, group->columns, TRUE);
  }

  group->previous_display_mode = DISPLAY_MODE_INTENSITY;
  group->frequence_index_last = -1;
  for(i=0; i<FREQUENCE_MAX_VALUES_NUMBER; i++)
    group->frequence_values[i] = -1;


//Création de la petite fenêtre
  group->widget = gtk_frame_new("");
  gtk_widget_set_double_buffered(group->widget, TRUE);
  hbox = gtk_hbox_new(FALSE, 0);

  gtk_layout_put(GTK_LAYOUT(zone_neurons), group->widget, pos_x, pos_y);

  gtk_box_pack_start(GTK_BOX(hbox), group->label, TRUE, FALSE, 0);

  gtk_frame_set_label_widget(GTK_FRAME(group->widget), hbox);

  group->button_vbox = gtk_vbox_new(TRUE, 0);

  if(group->rows * group->columns <= BIG_GRAPH_MAX_NEURONS_NUMBER)
    for (j = 0; j < group->rows; j++)
    {
        for (i = 0; i < group->columns; i++)
        {
            graph_get_line_color(j*group->columns + i, &r, &g, &b);
            sprintf(p_legende_texte, "<b><span foreground=\"#%02X%02X%02X\"> %dx%d</span></b>\n", (int) (r*255), (int) (g*255), (int) (b*255), i, j);
            button_label = gtk_label_new("");
            gtk_label_set_markup(GTK_LABEL(button_label), p_legende_texte);
            button = gtk_check_button_new();
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), group->afficher[j][i]);
            gtk_container_add(GTK_CONTAINER(button), button_label);
            gtk_widget_set_size_request(button, BUTTON_WIDTH, BUTTON_HEIGHT);
            gtk_box_pack_start(GTK_BOX(group->button_vbox), button, TRUE, FALSE, 0);
            g_signal_connect(GTK_OBJECT(button), "toggled", G_CALLBACK(on_group_display_show_or_mask_neuron), &(group->afficher[j][i]));
        }
    }

  group->drawing_area = gtk_drawing_area_new();
  gtk_widget_set_double_buffered(group->drawing_area, TRUE);
  principal_hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(principal_hbox), group->drawing_area, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(principal_hbox), group->button_vbox , FALSE, FALSE, 0);


  gtk_container_add(GTK_CONTAINER(group->widget), principal_hbox);
  gtk_widget_add_events(group->drawing_area, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(GTK_OBJECT(group->drawing_area), "button_press_event", (GtkSignalFunc) button_press_neurons, group);

  gtk_widget_set_size_request(group->widget, get_width_height(group->columns), get_width_height(group->rows));
  group->counter = 0;
  group->timer = g_timer_new();

  gtk_widget_show_all(group->widget);
  gtk_widget_hide_all(group->button_vbox);
  if(group->output_display == 3)
	  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_START, group->id, group->script->name);
  else
	  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, group->script->name);
  groups_to_display[number_of_groups_to_display] = group;
  number_of_groups_to_display++;
}

void group_display_destroy(type_group *group)
{

  int i;

  if(group->output_display == 3)
	  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_STOP, group->id, group->script->name);
  else
	  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, group->script->name);

  for (i = 0; i < number_of_groups_to_display; i++)
  {
    if (groups_to_display[i] == group)
    {
      /* Le group en cours est remplacé par le dernier group de la liste qui ne sera plus pris en compte */
      groups_to_display[i] = groups_to_display[number_of_groups_to_display - 1];
      number_of_groups_to_display--;
      break;
    }
  }
  if(group->indexAncien != NULL)
	  destroy_tab_2(group->indexAncien, group->rows);
  group->indexAncien = NULL;
  if(group->indexDernier != NULL)
	  destroy_tab_2(group->indexDernier, group->rows);
  group->indexDernier = NULL;
  if(group->tabValues != NULL)
	  destroy_tab_4(group->tabValues, group->columns, group->rows);
  group->tabValues = NULL;

  gtk_label_set_text(GTK_LABEL(group->label), "");
  gtk_widget_destroy(group->widget);
  group->widget = NULL;
}

void script_widget_update(type_script *script)
{
  char label_text[LABEL_MAX];
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
    else
    	break;
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

  script->widget = gtk_hbox_new(FALSE, 0);
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
  g_signal_connect(GTK_OBJECT(script->z_spinnner), "value-changed", (GCallback) change_plan, script);

  script->search_button = gtk_toggle_button_new();
  search_icon = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
  gtk_button_set_image(GTK_BUTTON(script->search_button), search_icon);
  gtk_box_pack_start(GTK_BOX(script->widget), script->search_button, FALSE, TRUE, 0);
  g_signal_connect(GTK_OBJECT(script->search_button), "toggled", (GtkSignalFunc) on_search_group_button_active, script);

//Pour que les cases soient cochées par défaut
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script->checkbox), TRUE);
  script->displayed = TRUE;

  gtk_widget_show_all(window); //Affichage du widget pWindow et de tous ceux qui sont dedans
  script_widget_update(script);
  architecture_display_update(architecture_display, NULL);

  gdk_threads_leave();
}

void drag_drop_neuron_frame(GtkWidget *zone_neurons, GdkEventButton *event, gpointer data)
{
  (void) data;

  if (move_neurons_group != NULL && move_neurons_start == 1)
  {
    gtk_layout_move(GTK_LAYOUT(zone_neurons), move_neurons_group->widget, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
    move_neurons_group = NULL;
    move_neurons_start = 0;
  }
  else if (open_group != NULL && open_neurons_start == 1)
  {
    gtk_layout_move(GTK_LAYOUT(zone_neurons), open_group->widget, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
    //group_display_new(open_group, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
    open_neurons_start = 0;

  }
}

void neurons_frame_drag_group(GtkWidget *pWidget, GdkEventButton *event, gpointer pdata)
{
    (void) pWidget;
    (void) pdata;

    if (move_neurons_group != NULL && move_neurons_start == 1)
    {
        gtk_layout_move(GTK_LAYOUT(zone_neurons), move_neurons_group->widget, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
    }
    else if (open_group != NULL && open_neurons_start == 1)
    {
        gtk_layout_move(GTK_LAYOUT(zone_neurons), open_group->widget, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
    }
}

gboolean script_caracteristics(type_script *script, int action)
{
    static type_script_display_save *saves[NB_SCRIPTS_MAX];
    static int number_of_saves = 0;

    type_group *group;
    int i, j, save_id=0, number_of_group_displayed = 0;
    type_script_display_save *save = NULL;

    /// return FALSE;// permet d'inhiber la fonction.


    if(action == UNLOCK_SCRIPT_CARACTERISTICS_FUNCTION)
    {
        return TRUE;
    }

    if(action == LOCK_SCRIPT_CARACTERISTICS_FUNCTION)
       {return TRUE;}


    if(script == NULL)
        return FALSE;

    if(action == APPLY_SCRIPT_GROUPS_CARACTERISTICS)
    {
        for(i=0; i<number_of_saves; i++)
            if(strcmp(saves[i]->name, script->name) == 0)
                {save = saves[i]; save_id = i;}
        if(save == NULL)
            return FALSE;

        group = script->groups;
        number_of_group_displayed = script->number_of_groups;
        for(i=0; i < number_of_group_displayed; i++)
            for(j=0; j < save->number_of_groups; j++)
                if(strcmp(group[i].name, save->groups[j].name) == 0)
                {
                    group[i].display_mode = save->groups[j].display_mode;
                    group[i].normalized = save->groups[j].normalized;
                    group[i].output_display = save->groups[j].output_display;
                    group[i].val_min = save->groups[j].val_min;
                    group[i].val_max = save->groups[j].val_max;
                    gdk_threads_enter();
                    group_display_new(&(group[i]), save->groups[j].x, save->groups[j].y);
                    gdk_threads_leave();
                }
        number_of_saves--;
        free(save->groups);
        free(save);
        if(number_of_saves > 0)
            saves[save_id] = saves[number_of_saves];

        return TRUE;
    }
    else if(action == SAVE_SCRIPT_GROUPS_CARACTERISTICS && number_of_saves < NB_SCRIPTS_MAX)
    {
        for (i = 0; i < number_of_groups_to_display; i++)
            if (groups_to_display[i]->script == script)
                number_of_group_displayed++;

        if(number_of_group_displayed == 0)
            return TRUE;

        for(j=0; j < number_of_saves; j++)
            if(strcmp(saves[j]->name, script->name) == 0)
                save = saves[j];

        if(save == NULL)
        {
            save = malloc(sizeof(type_script_display_save));
            saves[number_of_saves] = save;
            number_of_saves++;
        }
        save->groups = malloc(number_of_group_displayed * sizeof(type_group_display_save));
        save->number_of_groups = number_of_group_displayed;
        strcpy(save->name, script->name);

        j=0;
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
                save->groups[j].x = GTK_WIDGET(group->widget)->allocation.x;
                save->groups[j].y = GTK_WIDGET(group->widget)->allocation.y;
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

  pthread_mutex_lock(&mutex_script_caracteristics);
  script_caracteristics(script, SAVE_SCRIPT_GROUPS_CARACTERISTICS);
  pthread_mutex_unlock(&mutex_script_caracteristics);

  for (i = 0; i < number_of_groups_to_display; i++)
  {
    group = groups_to_display[i];
    if (group->script == script)
    {
      gtk_label_set_text(GTK_LABEL(group->label), "");
      gtk_widget_destroy(groups_to_display[i]->widget);
      if(group->indexAncien != NULL)
    	  destroy_tab_2(group->indexAncien, group->rows);
      group->indexAncien = NULL;
      if(group->indexDernier != NULL)
    	  destroy_tab_2(group->indexDernier, group->rows);
      group->indexDernier = NULL;
      if(group->tabValues != NULL)
    	  destroy_tab_4(group->tabValues, group->columns, group->rows);
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
  for(i=script->id; i<number_of_scripts; i++)
  {
	  scripts[i] = scripts[i+1];
	  scripts[i]->id = i;
	  scripts[i]->color = i % COLOR_MAX;
  }
  // scripts[script->id]->z = script->id;

  // mise à jour des coordonnées des scripts

  for(i=script->id; i<number_of_scripts; i++)
  {
	  script_update_positions(scripts[i]);
	  script_widget_update(scripts[i]);
  }

  script_widget_update(scripts[script->id]);
  architecture_display_update(architecture_display, NULL);
}

/**
 * Cette fonction permet de traiter les informations provenant de prométhé pour mettre à jour
 * l'affichage. Elle est appelée automatiquement plusieurs fois par seconde.
 */
gboolean neurons_refresh_display()
{
	int i;
	if(refresh_mode != REFRESH_MODE_AUTO)
		return TRUE;

	pthread_mutex_lock(&mutex_script_caracteristics);
	for (i = 0; i < number_of_groups_to_display; i++)
	{
		group_expose_neurons(groups_to_display[i], TRUE);
	}
	pthread_mutex_unlock(&mutex_script_caracteristics);
	return TRUE;
}

const char* tcolor(type_script *script)
{
  switch (script->color)
  {
  case 0:
    return TDARKGREEN;
  case 1:
    return TDARKMAGENTA;
  case 2:
    return TDARKYELLOW;
  case 3:
    return TDARKBLUE;
  case 4:
    return TDARKCYAN;
  case 5:
    return TDARKORANGE;
  case 6:
    return TDARKGREY;
  default:
    return TCOBALT;
  }
}

/**
 *
 * Donne au pinceau la couleur foncée correspondant à une certaine valeur de z
 *
 */
void color(cairo_t *cr, type_group *group)
{
  switch (group->script->color)
//La couleur d'un groupe ou d'une liaison dépend du plan dans lequel il/elle se trouve
  {
  case 0:
    cairo_set_source_rgb(cr, DARKGREEN);
    break;
  case 1:
    cairo_set_source_rgb(cr, DARKMAGENTA);
    break;
  case 2:
    cairo_set_source_rgb(cr, DARKYELLOW);
    break;
  case 3:
    cairo_set_source_rgb(cr, DARKBLUE);
    break;
  case 4:
    cairo_set_source_rgb(cr, DARKCYAN);
    break;
  case 5:
    cairo_set_source_rgb(cr, DARKORANGE);
    break;
  case 6:
    cairo_set_source_rgb(cr, DARKGREY);
    break;
  }
}

/**
 *
 * Donne au pinceau la couleur claire correspondant à une certaine valeur de z
 *
 */
void clearColor(cairo_t *cr, type_group group)
{
  switch (group.script->z)
//La couleur d'un groupe ou d'une liaison dépend du plan dans lequel il/elle se trouve
  {
  case 0:
    cairo_set_source_rgb(cr, GREEN);
    break;
  case 1:
    cairo_set_source_rgb(cr, MAGENTA);
    break;
  case 2:
    cairo_set_source_rgb(cr, YELLOW);
    break;
  case 3:
    cairo_set_source_rgb(cr, BLUE);
    break;
  case 4:
    cairo_set_source_rgb(cr, CYAN);
    break;
  case 5:
    cairo_set_source_rgb(cr, ORANGE);
    break;
  case 6:
    cairo_set_source_rgb(cr, GREY);
    break;
  }
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

void pandora_file_save(const char *filename)
{
  int script_id, i, width, height;
  GtkWidget *widget;
  Node *tree, *script_node, *group_node, *preferences_node;
  type_group *group;
  type_script *script;

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
        widget = group->widget;
        group_node = mxmlNewElement(script_node, "group");
        xml_set_string(group_node, "name", group->name);
        xml_set_int(group_node, "output_display", group->output_display);
        xml_set_int(group_node, "display_mode", group->display_mode);
        xml_set_float(group_node, "min", group->val_min);
        xml_set_float(group_node, "max", group->val_max);
        xml_set_int(group_node, "normalized", group->normalized);
        xml_set_int(group_node, "x", widget->allocation.x);
        xml_set_int(group_node, "y", widget->allocation.y);
      }
    }
  }

  xml_save_file((char *) filename, tree, whitespace_callback_pandora);
}

void pandora_file_load(const char *filename)
{
  int script_id, group_id;
  Node *tree, *script_node, *group_node, *preferences_node;
  type_group *group;
  type_script *script;

  while (number_of_groups_to_display)
  {
    group_display_destroy(groups_to_display[0]);
  }

  tree = xml_load_file((char *) filename);

  preferences_node = xml_get_first_child_with_node_name(tree, "preferences");
  /* On change le bus id s'il n'a pas été mis en paramètre */
  if (bus_id[0] == 0) strcpy(bus_id, xml_get_string(preferences_node, "bus_id"));
  if (bus_ip[0] == 0) strcpy(bus_id, xml_get_string(preferences_node, "bus_ip"));

  gtk_window_resize(GTK_WINDOW(window), xml_get_int(preferences_node, "window_width"), xml_get_int(preferences_node, "window_height"));
  gtk_range_set_value(GTK_RANGE(refreshScale), xml_get_int(preferences_node, "refresh"));
  gtk_range_set_value(GTK_RANGE(xScale), (double) xml_get_int(preferences_node, "x_scale"));
  gtk_range_set_value(GTK_RANGE(yScale), xml_get_int(preferences_node, "y_scale"));
  gtk_range_set_value(GTK_RANGE(zxScale), xml_get_int(preferences_node, "z_x_scale"));
  gtk_range_set_value(GTK_RANGE(zyScale), xml_get_int(preferences_node, "z_y_scale"));
  gtk_paned_set_position(GTK_PANED(vpaned), xml_get_int(preferences_node, "architecture_window_height"));

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
                group_display_new(group, xml_get_float(group_node, "x"), xml_get_float(group_node, "y"));

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
        script->displayed = xml_get_int(script_node, "visibility");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(script->checkbox), script->displayed);

        if (xml_get_number_of_childs(script_node) > 0)
        {
          for (group_id = 0; group_id < script->number_of_groups; group_id++)
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
                group_display_new(group, xml_get_float(group_node, "x"), xml_get_float(group_node, "y"));

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
  printf("\n            erreur !!!!!!!!!!!!!!\n");
  dialog = gtk_message_dialog_new(GTK_WINDOW(window), (window == NULL ? 0 : GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s \t %s \t %i :\n \t Error: %s \n", name_of_file, name_of_function, numero_of_line, total_message);
  gtk_dialog_run(GTK_DIALOG(dialog));
  if(window != NULL)
  {
  	  gtk_widget_destroy(window);
	  gtk_main_quit();
  }
  exit(EXIT_FAILURE);
}

void refresh_mode_combo_box_value_changed(GtkComboBox *comboBox, gpointer data)
{
	int i;
	static int id = 0;


	(void) data;
	refresh_mode = gtk_combo_box_get_active(comboBox);
	if(refresh_mode == REFRESH_MODE_MANUAL)
	{
		if(!id) id = g_timeout_add((guint) 50, neurons_refresh_display_without_change_values, NULL);
		for (i = 0; i < number_of_groups_to_display; i++)
			pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_STOP : PANDORA_SEND_NEURONS_STOP), groups_to_display[i]->id, groups_to_display[i]->script->name);

		gtk_widget_show_all(GTK_WIDGET(data));
	}
	else
	{
		 g_source_destroy(g_main_context_find_source_by_id(NULL, id));
		for (i = 0; i < number_of_groups_to_display; i++)
			pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_START : PANDORA_SEND_NEURONS_START), groups_to_display[i]->id, groups_to_display[i]->script->name);

		gtk_widget_hide_all(GTK_WIDGET(data));
	}
}

void neurons_manual_refresh(GtkWidget *pWidget, gpointer pdata)
{
	int i;

	(void) pWidget;
	(void) pdata;

	for (i = 0; i < number_of_groups_to_display; i++)
	{
		printf("\n           traitement d'un groupe\n");
		pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", (groups_to_display[i]->output_display == 3 ? PANDORA_SEND_EXT_ONE : PANDORA_SEND_NEURONS_ONE), groups_to_display[i]->id, groups_to_display[i]->script->name);
		//group_expose_neurons(groups_to_display[i], FALSE);
	}
}


gboolean neurons_refresh_display_without_change_values()
{
	int i, current_stop = stop;
	if(refresh_mode != REFRESH_MODE_MANUAL)
		return TRUE;
	pthread_mutex_lock(&mutex_script_caracteristics);
	stop = TRUE;
	for (i = 0; i < number_of_groups_to_display; i++)
	{
		group_expose_neurons(groups_to_display[i], TRUE);
	}
	stop = current_stop;
	pthread_mutex_unlock(&mutex_script_caracteristics);
	return TRUE;
}

void pandora_window_new()
{
  char path[PATH_MAX];
  GtkWidget *h_box_main, *v_box_main, *pFrameEchelles, *pVBoxEchelles, *hbox_buttons, *refreshModeHBox, *refreshModeComboBox, *refreshModeLabel, *refreshManualButton,  *refreshSetting, *refreshLabel, *xSetting, *xLabel, *menuBar, *fileMenu;
  GtkWidget *ySetting, *yLabel, *zxSetting, *zxLabel, *pFrameGroupes, *zySetting, *zyLabel;
  GtkWidget *pBoutons, *boutonSave, *boutonLoad, *boutonDefault, *boutonPause;
  GtkWidget *pFrameScripts, *scrollbars2;
  GtkWidget *load, *save, *saveAs, *quit, *itemFile;
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
  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(window_close), (GtkWidget*) window);


// création de la barre de menus.
  menuBar = gtk_menu_bar_new();

  fileMenu = gtk_menu_new();

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
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(window_close), (GtkWidget*) window);


  itemFile = gtk_menu_item_new_with_label("File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemFile), fileMenu);

  gtk_menu_append(menuBar, itemFile);


  /*Création d'une VBox (boîte de widgets disposés verticalement) */
  v_box_main = gtk_vbox_new(FALSE, 0);
  /*ajout de v_box_main dans pWindow, qui est alors vu comme un GTK_CONTAINER*/
  gtk_container_add(GTK_CONTAINER(window), v_box_main);

  hide_see_scales_button = gtk_toggle_button_new_with_label("Hide scales");
  g_signal_connect(G_OBJECT(hide_see_scales_button), "toggled", (GtkSignalFunc) on_hide_see_scales_button_active, NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_see_scales_button), FALSE);
  gtk_widget_set_size_request(hide_see_scales_button, 150, 30);
  gtk_box_pack_start(GTK_BOX(v_box_main), menuBar, FALSE, FALSE, 0); // ajout de la barre de menus.
  gtk_box_pack_start(GTK_BOX(v_box_main), hide_see_scales_button, FALSE, FALSE, 0);

  /*Création de deux HBox : une pour le panneau latéral et la zone principale, l'autre pour les 6 petites zones*/
  h_box_main = gtk_hbox_new(FALSE, 0);
  vpaned = gtk_vpaned_new();
  gtk_paned_set_position(GTK_PANED(vpaned), 600);
  neurons_frame = gtk_frame_new("Neurons' frame");
  gtk_box_pack_start(GTK_BOX(v_box_main), h_box_main, TRUE, TRUE, 0);

  /*Panneau latéral*/
  pPane = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h_box_main), pPane, FALSE, TRUE, 0);

//Les échelles
  pFrameEchelles = gtk_frame_new("Scales");
  gtk_container_add(GTK_CONTAINER(pPane), pFrameEchelles);
  pVBoxEchelles = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(pFrameEchelles), pVBoxEchelles);


  // Mode de réactualisation des groupes de neurones contenus dans le panneau du bas.
  refreshModeHBox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshModeHBox, FALSE, TRUE, 0);
  refreshModeLabel = gtk_label_new("Refresh mode : ");
  refreshModeComboBox = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(refreshModeComboBox), "Auto");
  gtk_combo_box_append_text(GTK_COMBO_BOX(refreshModeComboBox), "Semi-auto");
  gtk_combo_box_append_text(GTK_COMBO_BOX(refreshModeComboBox), "Manual");
  gtk_combo_box_set_active(GTK_COMBO_BOX(refreshModeComboBox), REFRESH_MODE_AUTO);
  gtk_box_pack_start(GTK_BOX(refreshModeHBox), refreshModeLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(refreshModeHBox), refreshModeComboBox, TRUE, TRUE, 0);

  refreshManualButton = gtk_button_new_with_label("Refresh");
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshManualButton, FALSE, TRUE, 0);

  g_signal_connect(G_OBJECT(refreshModeComboBox), "changed", (GCallback) refresh_mode_combo_box_value_changed, refreshManualButton);
  g_signal_connect(G_OBJECT(refreshManualButton), "clicked", (GCallback) neurons_manual_refresh, NULL);



//Fréquence de réactualisation de l'affichage, quand on est en mode échantillonné (Sampled)
  refreshSetting = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshSetting, FALSE, TRUE, 0);
  refreshLabel = gtk_label_new("Refresh (Hz):");
  refreshScale = gtk_hscale_new_with_range(0, 50, 1); //Ce widget est déjà déclaré comme variable globale
//On choisit le nombre de réactualisations de l'affichage par seconde, entre 0 et 50
  gtk_box_pack_start(GTK_BOX(refreshSetting), refreshLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(refreshSetting), refreshScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(refreshScale), REFRESHSCALE_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(refreshScale), "change-value", (GtkSignalFunc) refresh_scale_change_value, NULL);

//Echelle de l'axe des x
  xSetting = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), xSetting, FALSE, TRUE, 0);
  xLabel = gtk_label_new("x scale:");
  xScale = gtk_hscale_new_with_range(XSCALE_MIN, XSCALE_MAX, 1); //Ce widget est déjà déclaré comme variable globale
  gtk_box_pack_start(GTK_BOX(xSetting), xLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(xSetting), xScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(xScale), graphic.x_scale);
  g_signal_connect(GTK_OBJECT(xScale), "change-value", (GCallback) on_scale_change_value, &graphic.x_scale);

//Echelle de l'axe des y
  ySetting = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), ySetting, FALSE, TRUE, 0);
  yLabel = gtk_label_new("y scale:");
  yScale = gtk_hscale_new_with_range(YSCALE_MIN, YSCALE_MAX, 1); //Ce widget est déjà déclaré comme variable globale
  gtk_box_pack_start(GTK_BOX(ySetting), yLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(ySetting), yScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(yScale), graphic.y_scale);
  g_signal_connect(GTK_OBJECT(yScale), "change-value", (GCallback) on_scale_change_value, &graphic.y_scale);

//Décalage des plans selon x
  zxSetting = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zxSetting, FALSE, TRUE, 0);
  zxLabel = gtk_label_new("x gap:");
  zxScale = gtk_hscale_new_with_range(0, 200, 1); //Ce widget est déjà déclaré comme variable globale
  gtk_box_pack_start(GTK_BOX(zxSetting), zxLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(zxSetting), zxScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(zxScale), graphic.zx_scale);
  g_signal_connect(GTK_OBJECT(zxScale), "change-value", (GCallback) on_scale_change_value, &graphic.zx_scale);

//Décalage des plans selon y
  zySetting = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zySetting, FALSE, TRUE, 0);
  zyLabel = gtk_label_new("y gap:");
  zyScale = gtk_hscale_new_with_range(0, 2000, 1); //Ce widget est déjà déclaré comme variable globale
  gtk_box_pack_start(GTK_BOX(zySetting), zyLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(zySetting), zyScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(zyScale), graphic.zy_scale);
  g_signal_connect(GTK_OBJECT(zyScale), "change-value", (GCallback) on_scale_change_value, &graphic.zy_scale);

  hbox_buttons = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), hbox_buttons, FALSE, TRUE, 12);

  boutonPause = gtk_toggle_button_new_with_label("Stop graphs");
  gtk_box_pack_start(GTK_BOX(hbox_buttons), boutonPause, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonPause), "toggled", G_CALLBACK(graph_stop_or_not), NULL);

  boutonDefault = gtk_button_new_with_label("Default scales");
  gtk_box_pack_start(GTK_BOX(hbox_buttons), boutonDefault, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonDefault), "clicked", G_CALLBACK(defaultScale), NULL);


//3 boutons
  pBoutons = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), pBoutons, FALSE, TRUE, 0);
  boutonSave = gtk_button_new_with_label("Save");
  gtk_box_pack_start(GTK_BOX(pBoutons), boutonSave, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonSave), "clicked", G_CALLBACK(save_preferences), NULL);
  boutonLoad = gtk_button_new_with_label("Load");
  gtk_box_pack_start(GTK_BOX(pBoutons), boutonLoad, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonLoad), "clicked", G_CALLBACK(pandora_load_preferences), NULL);

  check_button_draw_connections = gtk_check_button_new_with_label("draw connections");
  check_button_draw_net_connections = gtk_check_button_new_with_label("draw net connections");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_draw_connections), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_draw_net_connections), TRUE);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), check_button_draw_connections, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), check_button_draw_net_connections, FALSE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(check_button_draw_connections), "toggled", (GtkSignalFunc) on_check_button_draw_active, NULL);
  gtk_signal_connect(GTK_OBJECT(check_button_draw_net_connections), "toggled", (GtkSignalFunc) on_check_button_draw_active, NULL);

  displayMode = "Sampled mode";

  modeLabel = gtk_label_new(displayMode);
  gtk_container_add(GTK_CONTAINER(pPane), modeLabel);

//Les scripts
  pFrameScripts = gtk_frame_new("Open scripts");
  gtk_container_add(GTK_CONTAINER(pPane), pFrameScripts);
  pVBoxScripts = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(pFrameScripts), pVBoxScripts);

//La zone principale
  pFrameGroupes = gtk_frame_new("Neural groups");
  gtk_container_add(GTK_CONTAINER(vpaned), pFrameGroupes);
  scrollbars = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(pFrameGroupes), scrollbars);
  architecture_display = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(architecture_display), 10000, 10000);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars), architecture_display);
  gtk_signal_connect(GTK_OBJECT(architecture_display), "scroll-event", (GtkSignalFunc) architecture_display_scroll_event, NULL);
  gtk_signal_connect(GTK_OBJECT(architecture_display), "expose_event", (GtkSignalFunc) architecture_display_update, NULL);
  gtk_widget_set_events(architecture_display, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON2_MOTION_MASK); //Détecte quand on appuie OU quand on relache un bouton de la souris alors que le curseur est dans la zone3D
  gtk_signal_connect(GTK_OBJECT(architecture_display), "button_press_event", (GtkSignalFunc) button_press_event, NULL);
  gtk_signal_connect(GTK_OBJECT(architecture_display), "button_press_event", (GtkSignalFunc) architecture_display_button_pressed, NULL);
  gtk_signal_connect(GTK_OBJECT(architecture_display), "button_release_event", (GtkSignalFunc) architecture_display_button_released, NULL);
  gtk_signal_connect(GTK_OBJECT(architecture_display), "motion-notify-event", (GtkSignalFunc) architecture_display_drag_motion, NULL);
  gtk_signal_connect(GTK_OBJECT(window), "key_press_event", (GtkSignalFunc) key_press_event, NULL);

//la zone des groupes de neurones
  gtk_container_add(GTK_CONTAINER(vpaned), neurons_frame);
  scrollbars2 = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(neurons_frame), scrollbars2);
  zone_neurons = gtk_layout_new(NULL, NULL);
  gtk_widget_set_size_request(GTK_WIDGET(zone_neurons), 3000, 3000);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars2), zone_neurons);
  gtk_widget_add_events(zone_neurons, GDK_ALL_EVENTS_MASK);
  g_signal_connect(GTK_OBJECT(zone_neurons), "button-release-event", (GtkSignalFunc) drag_drop_neuron_frame, NULL);
  g_signal_connect(GTK_OBJECT(zone_neurons), "motion-notify-event", (GtkSignalFunc) neurons_frame_drag_group, NULL);

  gtk_box_pack_start(GTK_BOX(h_box_main), vpaned, TRUE, TRUE, 0);

  gtk_widget_show_all(window); //Affichage du widget pWindow et de tous ceux qui sont dedans
  gtk_widget_hide_all(refreshManualButton);
}

/**
 *
 * name: Programme Principale (Main)
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
  char chemin[] = "./pandora.pandora";

  stop = FALSE;
  load_temporary_save = FALSE;
  architecture_display_dragging_currently = FALSE;
  refresh_mode = REFRESH_MODE_AUTO;
  graphic.x_scale = XSCALE_DEFAULT;
  graphic.y_scale = YSCALE_DEFAULT;
  graphic.zx_scale = XGAP_DEFAULT;
  graphic.zy_scale = YGAP_DEFAULT;

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

  g_thread_init(NULL); /* useless since glib 2.32 */
  gdk_threads_init();

  pthread_mutex_init(&mutex_script_caracteristics, NULL);

// Initialisation de GTK+
  gtk_init(&argc, &argv);

//Initialisation d'ENet
  if (enet_initialize() != 0)
  {
    printf("An error occurred while initializing ENet.\n");
    exit(EXIT_FAILURE);
  }
  atexit(pandora_quit);
  server_for_promethes();

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

  pandora_window_new();

//si après chargement il n'y a pas de bus_id
  if ((bus_id[0] == 0) || (bus_ip[0] == 0))
  {
    EXIT_ON_ERROR("You miss bus_ip or bus_id \n\tUsage: %s [-b bus_ip -i bus_id] \n", argv[0]);
  }

  if (access(preferences_filename, R_OK) == 0) {pandora_file_load(preferences_filename);   load_temporary_save = FALSE;}
  else if (access(chemin, R_OK) == 0) {pandora_file_load(chemin); load_temporary_save = TRUE;}
  prom_bus_init(bus_ip);

//Appelle la fonction refresh_display à intervalles réguliers si on est en mode échantillonné ('a' est la deuxième lettre de "Sampled mode")
  refresh_timer_id = g_timeout_add((guint)(1000 / (int) gtk_range_get_value(GTK_RANGE(refreshScale))), neurons_refresh_display, NULL);

  gdk_threads_enter();
  gtk_main(); //Boucle infinie : attente des événements
  gdk_threads_leave();
  return EXIT_SUCCESS;
}
