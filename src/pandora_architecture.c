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

                  if (group->selected_for_save == 1) pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, group->script->name);
                  else pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, group->script->name);
                }
              }

            }
            if (group == selected_group) cairo_set_source_rgba(cr, RED);
            else if (group->is_in_a_loop == TRUE) cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
            else color(cr, group);
            cairo_fill_preserve(cr);
            if (group->selected_for_save == 1)
            {

              if (group->on_saving == 1) cairo_set_source_rgba(cr, YELLOW);
              if (group->on_saving == 0) cairo_set_source_rgba(cr, BLUE);

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
                  color(cr, group);
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
