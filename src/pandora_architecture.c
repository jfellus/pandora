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
 * pandora_architecture.c
 *
 *  Created on: 5 août 2013
 *      Author: Nils Beaussé
 **/

/** Contient les fonctions, variables et evenements associés à la zone nomée architecture_display,
 zone d'affichage des scripts **/

#include "pandora_architecture.h"
#include "pandora_graphic.h"

type_graphic graphic;
GtkWidget *architecture_display = NULL; //La grande zone de dessin des liaisons entre groupes
gboolean architecture_display_dragging_currently = 0;
gdouble architecture_display_cursor_x = 0;
gdouble architecture_display_cursor_y = 0;
gdouble new_x, new_y, old_x, old_y;

// permet de connaitre les coordonnées d'un groupe affiché dans la zone "architecture_display" (point en haut à gauche du groupe).
void architecture_get_group_position(type_group *group, float *x, float *y)
{
  int i, group_id = 0, z;
  double x_offset = 0, y_offset = 0;
  type_script *script;
  type_group *tmp_group;

//On recalcule zMax, la plus grande valeur de z parmi les scripts ouverts
  zMax = 0;
  for (i = 0; i < number_of_scripts; i++)
    if (scripts[i]->z > zMax && scripts[i]->displayed) zMax = scripts[i]->z;

//Dessin des groupes : on dessine d'abord les scripts du plan z = 0 (s'il y en a), puis ceux du plan z = 1, etc., jusqu'à zMax inclus
  for (z = 0; z <= zMax; z++)
  {
    for (i = 0; i < number_of_scripts; i++)
    {
      script = scripts[i];
      x_offset = graphic.zx_scale * z;
      y_offset = graphic.zy_scale * z + script->y_offset * graphic.y_scale;

      if (script->z == z && script->displayed)
      {
        for (group_id = 0; group_id < script->number_of_groups; group_id++)
        {
          tmp_group = &script->groups[group_id];

          /* Test selection */
          if (tmp_group == group)
          {
            *x = x_offset + tmp_group->x * graphic.x_scale;
            *y = y_offset + tmp_group->y * graphic.y_scale;
            return;
          }
        }
      }
    }
  }
}

void choice_combo_box_changed(GtkComboBox *comboBox, gpointer data)
{
  gint forma;
  type_group* group=(type_group*)data;
  forma = gtk_combo_box_get_active(comboBox);

  group->thing_to_save=(int)forma;
}

void add_scroll_to_save(type_group *group, float pos_x, float pos_y, GtkWidget* architecture_display)
{
  GtkWidget* choice_combo_box;
  GtkRequisition minimum_size;
  GtkRequisition natural_size;
  GtkCellRenderer * cells_render ;
  GtkTreeStore * ts ;
  GtkTreeIter iter;


  if (group->scroll_to_save_widget != NULL || group->ok != TRUE) return;


  //Création de la petite fenétre
 // group->scroll_to_save_widget = gtk_aspect_frame_new("", 0, 0, 1, TRUE);
//  gtk_widget_set_double_buffered(group->scroll_to_save_widget, TRUE);



  // gtk_layout_put(GTK_LAYOUT(zone_neurons), group->widget, pos_x, pos_y);

//  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), column, TRUE);
  ts = gtk_tree_store_new(1, G_TYPE_STRING);
  gtk_tree_store_clear (ts);
  gtk_tree_store_insert(ts, &iter, NULL, 0);
  gtk_tree_store_set(ts, &iter, 0, "s", -1);
  gtk_tree_store_insert(ts, &iter, NULL, 0);
  gtk_tree_store_set(ts, &iter, 0, "s1", -1);
  gtk_tree_store_insert(ts, &iter, NULL, 0);
  gtk_tree_store_set(ts, &iter, 0, "s2", -1);
  gtk_tree_store_insert(ts, &iter, NULL, 0);
  gtk_tree_store_set(ts, &iter, 0, "d", -1);


  group->scroll_to_save_widget = choice_combo_box =  gtk_combo_box_new_with_model(GTK_TREE_MODEL(ts));
  gtk_widget_set_name (choice_combo_box,"Combo_box_architecture");
//  g_object_set (G_OBJECT(model), "font", FONT_DESC_LEVEL_3, NULL );

  cells_render = gtk_cell_renderer_text_new();
  g_object_set (G_OBJECT(cells_render), "font", FONT_COMBO, NULL );
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(choice_combo_box), cells_render, TRUE);
  gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( choice_combo_box ), cells_render, "text", 0, NULL );


  g_signal_connect(G_OBJECT(group->scroll_to_save_widget), "changed", (GCallback ) choice_combo_box_changed, group);

  gtk_combo_box_set_active(GTK_COMBO_BOX(choice_combo_box), group->thing_to_save);
  //gtk_container_add(GTK_CONTAINER(group->scroll_to_save_widget), choice_combo_box);


  gtk_widget_get_preferred_size (group->scroll_to_save_widget, &minimum_size, &natural_size);


  gtk_layout_put(GTK_LAYOUT(architecture_display), group->scroll_to_save_widget, (gint) (pos_x-30), (gint) (pos_y));


  gtk_widget_show_all(group->scroll_to_save_widget);
  gtk_widget_show_all(architecture_display);
}

void manage_scroll_to_save(type_group *group,gboolean selected_for_save,int x,int y,GtkWidget* architecture_display)
{
  if(selected_for_save)
  {
    add_scroll_to_save(group, x, y, architecture_display);
  }
  else
  {
    gtk_widget_destroy(group->scroll_to_save_widget);
    group->scroll_to_save_widget=NULL;
  }
}

// modifie la zone affichée. x et y représentent le point situé en haut a gauche de la zone à afficher. //TODO : necessaire?
void architecture_set_view_point(GtkWidget *scrollbars, gdouble x, gdouble y)
{
  GtkAdjustment *a_x, *a_y;
  a_x = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars));
  a_y = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars));
  // modification des valeurs.
  gtk_adjustment_set_value(a_x, x);
  gtk_adjustment_set_value(a_y, y);
  // propagation de l'évènement. permet d'actualiser la zone à afficher.
  // gtk_adjustment_value_changed(a_x);
  // gtk_adjustment_value_changed(a_y);
  // l'evenement semble capté par un expose event.
}

// Met à jour l'affichage des groupes de neuronnes
void architecture_display_update(GtkWidget *architecture_display, cairo_t *cr, void *data)
{
  // cairo_t *cr;
  char text[TEXT_MAX];
  double dashes[] =
    { 10.0, 20.0 };
  int i, group_id = 0, link_id, z;
  double x_offset = 0, y_offset = 0;
  type_script *script;
  type_group *group = NULL;
  type_group *input_group = NULL;
  GdkEventButton *event = data;
  int x_centre = 0;
  int y_centre = 0;
  int y_control = 0;
  int x_control = 0;

  //cairo_t *cr;

  //cr = gdk_cairo_create(gtk_widget_get_window(architecture_display)); //Crée un contexte Cairo associé à la drawing_area "zone"

  if (gtk_cairo_should_draw_window(cr, gtk_widget_get_window(architecture_display)))
  {
    cairo_set_source_rgba(cr, WHITE);
    cairo_paint(cr);

//On recalcule zMax, la plus grande valeur de z parmi les scripts ouverts
    zMax = 0;
    for (i = 0; i < number_of_scripts; i++)
      if (scripts[i]->z > zMax && scripts[i]->displayed) zMax = scripts[i]->z;

//Dessin des groupes : on dessine d'abord les scripts du plan z = 0 (s'il y en a), puis ceux du plan z = 1, etc., jusqu'à zMax inclus
    for (z = 0; z <= zMax; z++)
    {
      for (i = 0; i < number_of_scripts; i++)
      {
        script = scripts[i];
        x_offset = graphic.zx_scale * z;
        y_offset = graphic.zy_scale * z + script->y_offset * graphic.y_scale;

        if (script->z == z && script->displayed)
        {
          for (group_id = 0; group_id < script->number_of_groups; group_id++)
          {
            group = &(script->groups[group_id]);

            /* Box */
            if (group->on_saving == 1) cairo_set_line_width(cr, 4);
            if (group->on_saving == 0) cairo_set_line_width(cr, 2);
            cairo_rectangle(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale, LARGEUR_GROUPE, HAUTEUR_GROUPE);

            /* Test selection */
            if (event != NULL)
            {
              if (cairo_in_fill(cr, event->x, event->y)) // Selection pour sauvegarde (touche control activée)
              {
                selected_group = group;
                if ((event->state & GDK_CONTROL_MASK) && !saving_press)
                {
                  group->selected_for_save = !group->selected_for_save;
                  manage_scroll_to_save(group,group->selected_for_save,x_offset + group->x * graphic.x_scale+LARGEUR_GROUPE,y_offset + group->y * graphic.y_scale, architecture_display);


              //    if (group->selected_for_save == 1) pandora_bus_send_message(bus_id, "pandora(%d,%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, 0, group->script->name + strlen(bus_id) + 1);
              //    else pandora_bus_send_message(bus_id, "pandora(%d,%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, 0, group->script->name + strlen(bus_id) + 1);
                }

                if ((event->state & GDK_SHIFT_MASK) && !saving_link_press)
                {
                  group->selected_for_save_link = !group->selected_for_save_link;

                 // if (group->selected_for_save_link == 1) pandora_bus_send_message(bus_id, "pandora(%d,%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, 0, group->script->name + strlen(bus_id) + 1);
                 // else pandora_bus_send_message(bus_id, "pandora(%d,%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, 0, group->script->name + strlen(bus_id) + 1);
                }

              }

            }

            //remplissage du groupe
            if (group == selected_group) cairo_set_source_rgba(cr, RED);
            else if (group->is_in_a_loop == TRUE) cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
            else gdk_cairo_set_source_rgba(cr, &colors[group->script->color]);
            cairo_fill(cr); // preserve le rectangle



            if (group->selected_for_save_link == 1)
            {
              cairo_save(cr);
         //     cairo_clip(cr);
              cairo_rectangle(cr, x_offset - 8 + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale, 2, HAUTEUR_GROUPE);
              cairo_set_line_width(cr, 5);

              if (group->on_saving_link == 1)
                  cairo_set_source_rgba(cr, GREEN);
              if (group->on_saving_link == 0)
                  cairo_set_source_rgba(cr, RED);

              cairo_stroke(cr);
           //   cairo_set_line_width(cr, 2);
              cairo_restore(cr);
            }
            cairo_rectangle(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale, LARGEUR_GROUPE, HAUTEUR_GROUPE);


            if (group->selected_for_save == 1)
            {

              if (group->on_saving == 1)
                {
                  cairo_set_line_width(cr, 4);
                  cairo_set_source_rgba(cr, YELLOW);
                }
              if (group->on_saving == 0)
                {
                  cairo_set_line_width(cr, 2);
                  cairo_set_source_rgba(cr, BLUE);
                }

              cairo_stroke_preserve(cr);
            }
            else
            {
              cairo_set_line_width(cr, 1);
              cairo_set_source_rgba(cr, BLACK);
              cairo_stroke_preserve(cr);
            }





            /* Texts  */
            cairo_save(cr);
            if (group != selected_group) cairo_clip(cr);
            cairo_set_source_rgba(cr, BLACK);
            cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + TEXT_LINE_HEIGHT);
            if (calculate_executions_times == TRUE)
            {
              snprintf(text, TEXT_MAX, "%s   | %sµs", group->name, group->stats.message);
            }
            else
            {
              snprintf(text, TEXT_MAX, "%s", group->name);
            }
            cairo_show_text(cr, text);
            cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + 2 * TEXT_LINE_HEIGHT);
            cairo_show_text(cr, group->function);
            snprintf(text, TEXT_MAX, "%d x %d", group->columns, group->rows);
            cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + 3 * TEXT_LINE_HEIGHT);
            cairo_show_text(cr, text);
            cairo_stroke(cr);
            cairo_restore(cr); /* unclip */

          }

          for (group_id = 0; group_id < script->number_of_groups; group_id++)
          {
            group = &(script->groups[group_id]);

            /* Internal links */
            if (graphic.draw_links)
            {
              for (link_id = 0; link_id < group->number_of_links; link_id++)
              {
                input_group = group->previous[link_id];

                if ((group == selected_group) || (input_group == selected_group))
                {
                  cairo_set_source_rgba(cr, RED);
                  cairo_set_line_width(cr, 3);
                }
                else if (group->is_in_a_loop == TRUE && input_group->is_in_a_loop == TRUE)
                {
                  cairo_set_source_rgba(cr, BLACK);
                  cairo_set_line_width(cr, 4);
                }
                else
                {
                  gdk_cairo_set_source_rgba(cr, &colors[group->script->color]);
                  cairo_set_line_width(cr, 1);
                }
                cairo_move_to(cr, x_offset + input_group->x * graphic.x_scale + LARGEUR_GROUPE, y_offset + input_group->y * graphic.y_scale + HAUTEUR_GROUPE / 2); //Début de la liaison (à droite du groupe prédécesseur)
                cairo_line_to(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + HAUTEUR_GROUPE / 2); //Fin de la liaison (à gauche du groupe courant)
                cairo_stroke(cr);
              }
            }

            if (graphic.draw_links)
            {

              for (link_id = 0; link_id < group->number_of_links_second; link_id++)
              {
                input_group = group->previous_second[link_id];

                if ((group == selected_group) || (input_group == selected_group))
                {

                  cairo_set_source_rgba(cr, BLUE_TRAN);
                  cairo_set_line_width(cr, 1);
                  cairo_move_to(cr, x_offset + input_group->x * graphic.x_scale + LARGEUR_GROUPE, y_offset + input_group->y * graphic.y_scale + HAUTEUR_GROUPE / 2); //Début de la liaison (à droite du groupe prédécesseur)

                  x_centre = (x_offset + group->x * graphic.x_scale + x_offset + input_group->x * graphic.x_scale + LARGEUR_GROUPE) / 2;
                  y_centre = (y_offset + group->y * graphic.y_scale + HAUTEUR_GROUPE / 2 + y_offset + input_group->y * graphic.y_scale + HAUTEUR_GROUPE / 2) / 2;
                  x_control = x_centre;
                  y_control = y_centre - 30;

                  cairo_curve_to(cr, x_control, y_control, x_control, y_control, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + HAUTEUR_GROUPE / 2);

                  // cairo_line_to(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + HAUTEUR_GROUPE / 2); //Fin de la liaison (à gauche du groupe courant)
                  cairo_stroke(cr);
                }
              }
            }
          }
        }
      }
    }
    /* Net links */
    if (graphic.draw_net_links)
    {
      for (i = 0; i < number_of_net_links; i++)
      {
        input_group = net_links[i].previous;
        group = net_links[i].next;

        if (input_group != NULL && group != NULL)
        {
          if ((input_group == selected_group) || (group == selected_group))
          {
            cairo_set_source_rgba(cr, RED);
            cairo_set_line_width(cr, 5);
          }
          else
          {
            cairo_set_source_rgba(cr, RED);
            cairo_set_line_width(cr, 3);

          }

          if (!(net_links[i].type & NET_LINK_BLOCK)) cairo_set_dash(cr, dashes, 2, 0);
          x_offset = graphic.zx_scale * input_group->script->z;
          y_offset = graphic.zy_scale * input_group->script->z + input_group->script->y_offset * graphic.y_scale;
          cairo_move_to(cr, x_offset + input_group->x * graphic.x_scale + LARGEUR_GROUPE, y_offset + input_group->y * graphic.y_scale + HAUTEUR_GROUPE / 2); //Début de la liaison (à droite du groupe prédécesseur)
          x_offset = graphic.zx_scale * group->script->z;
          y_offset = graphic.zy_scale * group->script->z + group->script->y_offset * graphic.y_scale;
          cairo_line_to(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + HAUTEUR_GROUPE / 2); //Fin de la liaison (à gauche du groupe courant)
          cairo_stroke(cr);
        }
      }
    }
    //cairo_destroy(cr);
  }
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
void architecture_display_button_pressed(GtkWidget *pWidget, GdkEvent *user_event, gpointer pdata)
{
  cairo_t *cr;
  GdkEventButton *event = NULL;
  GtkWidget *scrollbars2 = (GtkWidget*) pdata;
  float hscroll, vscroll;
  (void) pWidget;
  // (void) pdata;
  event = (GdkEventButton*) user_event;

  selected_group = NULL;

  // architecture_display_update(architecture_display, NULL);
  //TODO : pas bien, on perd l'avantage de GTK3 qui créé déja la zone, à faire : separer detection du clic de mise à jour affichage
  //g_object_set_data(architecture_display,"event",event);
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
  case 2:
    //Sur un clic du milieu, on supprime le détail du neurone
    open_group = selected_group;
    if (open_group != NULL) group_display_destroy(open_group);
    break;
  case 3:
    open_neurons_start = TRUE;
    move_neurons_start = FALSE;
    open_group = selected_group;
    hscroll = (float) gtk_adjustment_get_value(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars2)));
    vscroll = (float) gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars2)));

    if (open_group != NULL) group_display_new(open_group, 25.0 + hscroll, 25.0 + vscroll, zone_neurons);
    break;
  default:
    move_neurons_start = FALSE;
    open_neurons_start = FALSE;
    break;
  }

  gtk_widget_queue_draw(architecture_display);

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
    else if ((event->direction == GDK_SCROLL_DOWN) && (event->state & GDK_CONTROL_MASK))
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

gboolean queue_draw_archi(gpointer data)
{
  (void) data;
  gtk_widget_queue_draw(GTK_WIDGET(architecture_display));
  //pthread_cond_signal(&cond_copy_arg_group_display);
  return FALSE;
}
