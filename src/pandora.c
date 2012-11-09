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

char label_text[LABEL_MAX];

GtkWidget *window; //La fenêtre de l'application Pandora
GtkWidget *selected_group_dialog;
GtkWidget *vpaned;
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

char preferences_filename[PATH_MAX]; //fichier de préférences (*.jap)

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

void group_display_destroy(type_group *group);

void pandora_quit()
{
  pandora_bus_send_message(bus_id, "pandora(%d,0)", PANDORA_STOP);
  enet_deinitialize();
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
void window_close(GtkWidget *pWidget, gpointer pData) //Fonction de fermeture de Pandora
{
  (void) pWidget;
  (void) pData;

  exit(EXIT_SUCCESS);
}

/**
 *
 * Un script change de plan
 *
 */

void change_plan(GtkWidget *pWidget, gpointer data) //Un script change de plan
{

  type_script *script = data;
  script->z = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(pWidget));

  script_update_positions(script);

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

void on_search_group_button_active(GtkWidget *pWidget, type_script *script)
{
  GtkWidget *search_dialog, *search_entry, *h_box, *name_radio_button_search, *function_radio_button_search;
  int i, last_occurence = 0, stop = 0;
  (void) pWidget;

//On crée une fenêtre de dialogue
  search_dialog = gtk_dialog_new_with_buttons("Recherche d'un groupe", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  gtk_window_set_default_size(GTK_WINDOW(search_dialog), 300, 75);

  name_radio_button_search = gtk_radio_button_new_with_label_from_widget(NULL, "par nom");
  function_radio_button_search = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(name_radio_button_search), "par fonction");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(name_radio_button_search), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(function_radio_button_search), FALSE);

  h_box = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h_box), name_radio_button_search, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h_box), function_radio_button_search, TRUE, FALSE, 0);

//On lui ajoute une entrée de saisie de texte
  search_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(search_entry), "nom/fonction du groupe recherché");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(search_dialog)->vbox), h_box, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(search_dialog)->vbox), search_entry, TRUE, FALSE, 0);

  gtk_widget_show_all(GTK_DIALOG(search_dialog)->vbox);

//Aucun groupe n'est séléctionné
  selected_group = NULL;

  do
  {
    switch (gtk_dialog_run(GTK_DIALOG(search_dialog)))
    {
    //Si la réponse est OK
    case GTK_RESPONSE_OK:
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(name_radio_button_search)))
      {
        for (i = 0; i < script->number_of_groups; i++)
        {
          if (strcmp(gtk_entry_get_text(GTK_ENTRY(search_entry)), script->groups[i].name) == 0)
          {
            //Le groupe correspondant est séléctionné
            selected_group = &script->groups[i];
            break;
          }
        }
        stop = 1;
      }
      else
      {
        for (i = last_occurence; i < script->number_of_groups; i++)
        {
          if (strcmp(gtk_entry_get_text(GTK_ENTRY(search_entry)), script->groups[i].function) == 0)
          {
            //Le groupe correspondant est séléctionné
            selected_group = &script->groups[i];
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
  }
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
    else return;
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

  architecture_display_update(architecture_display, NULL); //On redessine la grille avec la nouvelle échelle
}

void on_group_display_output_combobox_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = data;
  group->output_display = gtk_combo_box_get_active(combo_box);
}

void on_group_display_mode_combobox_changed(GtkComboBox *combo_box, gpointer data)
{
  type_group *group = data;
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

void group_display_new(type_group *group, float pos_x, float pos_y)
{
  GtkWidget *button_frame, *hbox;

  if (group->widget != NULL) return;

  group->label = gtk_label_new("0 Hz");

//Création de la petite fenêtre
  group->widget = gtk_frame_new("");
  hbox = gtk_hbox_new(FALSE, 0);

  gtk_layout_put(GTK_LAYOUT(zone_neurons), group->widget, pos_x, pos_y);

  button_frame = gtk_toggle_button_new_with_label(group->name);

  gtk_box_pack_start(GTK_BOX(hbox), button_frame, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), group->label, TRUE, FALSE, 0);

  gtk_frame_set_label_widget(GTK_FRAME(group->widget), hbox);

  group->drawing_area = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(group->widget), group->drawing_area);
  gtk_widget_add_events(group->drawing_area, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(GTK_OBJECT(group->drawing_area), "button_press_event", (GtkSignalFunc) button_press_neurons, group);

  gtk_widget_set_size_request(group->widget, get_width_height(group->columns), get_width_height(group->rows));
  group->counter = 0;
  group->timer = g_timer_new();

  groups_to_display[number_of_groups_to_display] = group;
  number_of_groups_to_display++;
  gtk_widget_show_all(group->widget);
  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, group->script->name);
}

void group_display_destroy(type_group *group)
{

  int i;

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
    group_display_new(open_group, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
    open_neurons_start = 0;

  }
}

void script_destroy(type_script *script)
{
  int i;
  type_group *group;

  for (i = 0; i < number_of_groups_to_display; i++)
  {
    group = groups_to_display[i];
    if (group->script == script)
    {
      gtk_widget_destroy(groups_to_display[i]->widget);
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
  scripts[script->id] = scripts[number_of_scripts];
  scripts[script->id]->id = script->id;
  scripts[script->id]->color = script->id % COLOR_MAX;
  scripts[script->id]->z = script->id;

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
  for (i = 0; i < number_of_groups_to_display; i++)
  {
    group_expose_neurons(groups_to_display[i]);
  }
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
void pandora_file_save(char *filename)
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

  xml_save_file(filename, tree, whitespace_callback_pandora);
}

void pandora_file_load(char *filename)
{
  int script_id, group_id;
  Node *tree, *script_node, *group_node, *preferences_node;
  type_group *group;
  type_script *script;

  while (number_of_groups_to_display)
  {
    group_display_destroy(groups_to_display[0]);
  }

  tree = xml_load_file(filename);

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

void fatal_error(const char *name_of_file, const char *name_of_function, int numero_of_line, const char *message, ...)
{
  char total_message[MESSAGE_MAX];
  GtkWidget *dialog;
  va_list arguments;

  va_start(arguments, message);
  vsnprintf(total_message, MESSAGE_MAX, message, arguments);
  va_end(arguments);

  dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s \t %s \t %i :\n \t Error: %s \n", name_of_file, name_of_function, numero_of_line, total_message);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  exit(EXIT_FAILURE);
}

void pandora_window_new()
{

  char path[PATH_MAX];
  GtkWidget *h_box_main, *v_box_main, *pFrameEchelles, *pVBoxEchelles, *refreshSetting, *refreshLabel, *xSetting, *xLabel;
  GtkWidget *ySetting, *yLabel, *zxSetting, *zxLabel, *pFrameGroupes, *scrollbars, *zySetting, *zyLabel;
  GtkWidget *pBoutons, *boutonSave, *boutonLoad, *boutonDefault;
  GtkWidget *pFrameScripts, *scrollbars2;
//La fenêtre principale

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  /* Positionne la GTK_WINDOW "pWindow" au centre de l'écran */
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  /* Taille de la fenêtre */
  gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);

  window_title_update();
  sprintf(path, "%s/bin_leto_prom/resources/pandora_icon.png", getenv("HOME"));
  gtk_window_set_icon_from_file(GTK_WINDOW(window), path, NULL);

//Le signal de fermeture de la fenêtre est connecté à la fenêtre (petite croix)
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(window_close), (GtkWidget*) window);

  /*Création d'une VBox (boîte de widgets disposés verticalement) */
  v_box_main = gtk_vbox_new(FALSE, 0);
  /*ajout de v_box_main dans pWindow, qui est alors vu comme un GTK_CONTAINER*/
  gtk_container_add(GTK_CONTAINER(window), v_box_main);

  hide_see_scales_button = gtk_toggle_button_new_with_label("Hide scales");
  g_signal_connect(G_OBJECT(hide_see_scales_button), "toggled", (GtkSignalFunc) on_hide_see_scales_button_active, NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_see_scales_button), FALSE);
  gtk_widget_set_size_request(hide_see_scales_button, 150, 30);
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

//Fréquence de réactualisation de l'affichage, quand on est en mode échantillonné (Sampled)
  refreshSetting = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshSetting, FALSE, TRUE, 0);
  refreshLabel = gtk_label_new("Refresh (Hz):");
  refreshScale = gtk_hscale_new_with_range(0, 50, 1); //Ce widget est déjà déclaré comme variable globale
//On choisit le nombre de réactualisations de l'affichage par seconde, entre 1 et 24
  gtk_box_pack_start(GTK_BOX(refreshSetting), refreshLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(refreshSetting), refreshScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(refreshScale), REFRESHSCALE_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(refreshScale), "change-value", (GtkSignalFunc) refresh_scale_change_value, NULL);

//Echelle de l'axe des x
  xSetting = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), xSetting, FALSE, TRUE, 0);
  xLabel = gtk_label_new("x scale:");
  xScale = gtk_hscale_new_with_range(10, 350, 1); //Ce widget est déjà déclaré comme variable globale
  gtk_box_pack_start(GTK_BOX(xSetting), xLabel, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(xSetting), xScale, TRUE, TRUE, 0);
  gtk_range_set_value(GTK_RANGE(xScale), graphic.x_scale);
  g_signal_connect(GTK_OBJECT(xScale), "change-value", (GCallback) on_scale_change_value, &graphic.x_scale);

//Echelle de l'axe des y
  ySetting = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), ySetting, FALSE, TRUE, 0);
  yLabel = gtk_label_new("y scale:");
  yScale = gtk_hscale_new_with_range(10, 350, 1); //Ce widget est déjà déclaré comme variable globale
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

//3 boutons
  pBoutons = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pVBoxEchelles), pBoutons, FALSE, TRUE, 0);
  boutonSave = gtk_button_new_with_label("Save");
  gtk_box_pack_start(GTK_BOX(pBoutons), boutonSave, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonSave), "clicked", G_CALLBACK(save_preferences), NULL);
  boutonLoad = gtk_button_new_with_label("Load");
  gtk_box_pack_start(GTK_BOX(pBoutons), boutonLoad, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonLoad), "clicked", G_CALLBACK(pandora_load_preferences), NULL);
  boutonDefault = gtk_button_new_with_label("Default");
  gtk_box_pack_start(GTK_BOX(pBoutons), boutonDefault, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(boutonDefault), "clicked", G_CALLBACK(defaultScale), NULL);

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
  gtk_signal_connect(GTK_OBJECT(architecture_display), "expose_event", (GtkSignalFunc) architecture_display_update, NULL);
  gtk_widget_set_events(architecture_display, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK); //Détecte quand on appuie OU quand on relache un bouton de la souris alors que le curseur est dans la zone3D
  gtk_signal_connect(GTK_OBJECT(architecture_display), "button_press_event", (GtkSignalFunc) button_press_event, NULL);
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

  gtk_box_pack_start(GTK_BOX(h_box_main), vpaned, TRUE, TRUE, 0);

  gtk_widget_show_all(window); //Affichage du widget pWindow et de tous ceux qui sont dedans
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

  g_thread_init(NULL);
  gdk_threads_init();

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

  if (access(preferences_filename, R_OK) == 0) pandora_file_load(preferences_filename);
  prom_bus_init(bus_ip);

//Appelle la fonction refresh_display à intervalles réguliers si on est en mode échantillonné ('a' est la deuxième lettre de "Sampled mode")
  refresh_timer_id = g_timeout_add((guint)(1000 / (int) gtk_range_get_value(GTK_RANGE(refreshScale))), neurons_refresh_display, NULL);

  gdk_threads_enter();
  gtk_main(); //Boucle infinie : attente des événements
  gdk_threads_leave();

  return EXIT_SUCCESS;
}

