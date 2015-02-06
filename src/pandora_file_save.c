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
/**
 * pandora_file_save.c
 *
 *  Created on: 5 août 2013
 *      Author: Nils Beaussé
 */

#include "pandora_file_save.h"
#include "pandora_graphic.h"
#include "pandora_architecture.h"

/**
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

// Sauvegarde des preferences dans un fichier XML
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
  xml_set_int(preferences_node, "compress", (int) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compress_button)));

  for (script_id = 0; script_id < number_of_scripts; script_id++)
  {
    script = scripts[script_id];
    script_node = mxmlNewElement(preferences_node, "script");
    xml_set_string(script_node, "name", scripts[script_id]->nom_gene);

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
        xml_set_float(group_node, "neurons_height", group->neurons_height);
        xml_set_float(group_node, "neurons_width", group->neurons_width);

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

// Chargement des preferences depuis un fichier XML
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
  if (xml_try_to_get_int(preferences_node, "compress", &value)) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compress_button), (gboolean) value);
  else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compress_button), FALSE);

  for (script_id = 0; script_id < number_of_scripts; script_id++)
  {
    script = scripts[script_id];
    for (script_node = xml_get_first_child_with_node_name(preferences_node, "script"); script_node; script_node = xml_get_next_homonymous_sibling(script_node))
    {
      if (!strcmp(script->nom_gene, xml_get_string(script_node, "name")))
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
                group->from_file = TRUE;
                if (xml_try_to_get_float(group_node, "neurons_width", &(group->neurons_width)) != 1 || xml_try_to_get_float(group_node, "neurons_height", &(group->neurons_height)) != 1) determine_ideal_length(group, &(group->neurons_width), &(group->neurons_height));
                group_display_new(group, xml_get_float(group_node, "x"), xml_get_float(group_node, "y"), zone_neurons);
                if (group->previous_output_display != group->output_display) resize_group(group);
              }
            }
          }
        }
      }
    }
  }
  xml_delete_tree(tree);
}

// Chargement des preferences specifiques à un script.
void pandora_file_load_script(const char *filename, type_script *script)
{
  int group_id;
  Node *tree, *script_node, *group_node, *preferences_node;
  type_group *group;
  new_group_argument *arguments;

  tree = xml_load_file((char *) filename);
  preferences_node = xml_get_first_child_with_node_name(tree, "preferences");
  /* On change le bus id s'il n'a pas été mis en paramètre */
  if (bus_id[0] == 0) strcpy(bus_id, xml_get_string(preferences_node, "bus_id"));
  if (bus_ip[0] == 0) strcpy(bus_id, xml_get_string(preferences_node, "bus_ip"));

  for (script_node = xml_get_first_child_with_node_name(preferences_node, "script"); script_node; script_node = xml_get_next_homonymous_sibling(script_node))
  {
    if (!strcmp(script->nom_gene, xml_get_string(script_node, "name")))
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
              group->from_file = TRUE;
              if (xml_try_to_get_float(group_node, "neurons_width", &(group->neurons_width)) != 1 || xml_try_to_get_float(group_node, "neurons_height", &(group->neurons_height)) != 1) determine_ideal_length(group, &(group->neurons_width), &(group->neurons_height));

              arguments = ALLOCATION(new_group_argument);
              arguments->group = group;
              arguments->posx = xml_get_float(group_node, "x");
              arguments->posy = xml_get_float(group_node, "y");
              arguments->zone_neuron = zone_neurons;
              arguments->blocked = FALSE;

              g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc) group_display_new_threaded, (gpointer) arguments, NULL);
              //g_main_context_invoke (NULL,(GSourceFunc)group_display_new_threaded,(gpointer)(&arguments));//TODO : resoudre ce probleme !
              //  pthread_mutex_lock(&mutex_loading);
              //  pthread_cond_wait(&cond_loading, &mutex_loading);
              //  pthread_mutex_unlock(&mutex_loading);
              // group_display_new(group, xml_get_float(group_node, "x"), xml_get_float(group_node, "y"));

            }
          }
        }
      }
    }
  }
  xml_delete_tree(tree);
}

/**
 *
 * Enregistrer les préférences dans un fichier, appel les fonctions au dessus
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
 * Ouvrir un fichier de préférences, appel les fonctions au dessus
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
