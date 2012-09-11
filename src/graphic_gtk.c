#include <graphic_Tx.h> /* Pour essayer d'homogeneiser les fonctions promethe */

#include "graphic.h"
#include "pandora.h"

#define DIGITS_MAX 32
#define NB_DIGITS 7
#define TEXT_LINE_HEIGHT 12
#define TEXT_MAX 32
#define TEXT_OFFSET 4




GtkWidget *check_button_draw_connections, *check_button_draw_net_connections;

/* TODO optimiser */
float niveauDeGris(float val, float valMin, float valMax)
{
  float ndg = (val - valMin) / (valMax - valMin);
  if (ndg < 0) ndg = 0;
  else  if (ndg > 1) ndg = 1;
  return ndg;
}
/**
 *  Affiche les neurones d'une petite fenêtre
 */
void group_expose_neurons(type_group *group)
{
  cairo_t *cr;

  float ndg = 0, val;
  float largeurNeuron, hauteurNeuron;
  int i, j;
  int incrementation;
  int stride;
  cairo_format_t format;
  cairo_surface_t *image;
  prom_images_struct *prom_images;

  float time;
  char label_text[LABEL_MAX];

  incrementation = group->number_of_neurons / (group->columns * group->rows);

  //Début du dessin
  gdk_threads_enter();

  //Dimensions d'un neurone
  largeurNeuron = GTK_WIDGET(group->drawing_area)->allocation.width / (float) group->columns;
  hauteurNeuron = GTK_WIDGET(group->drawing_area)->allocation.height / (float) group->rows;

  time = g_timer_elapsed(group->timer, NULL);
  g_timer_start(group->timer);
  snprintf(label_text, LABEL_MAX, "%.3f Hz", group->counter / time);
  group->counter = 0;
  gtk_label_set_text(group->label, label_text);

  cr = gdk_cairo_create(GTK_WIDGET(group->drawing_area)->window); //Crée un contexte Cairo associé à la drawing_area "zone"
  if (group->output_display == 3)
  {
    if (group->ext != NULL)
    {
      prom_images = (prom_images_struct*) group->ext;

      switch (prom_images->nb_band)
      {
      case 1:
        format = CAIRO_FORMAT_A8; /* grey with 8bits */
        break;
      case 3:
        format = CAIRO_FORMAT_RGB24; /* colors */
        break;
      case 4:
        format = CAIRO_FORMAT_A8; /*grey with floats*/
        break;
      default:
        format = CAIRO_FORMAT_A8; /* grey with 8bits */
      }

      stride = cairo_format_stride_for_width(format, prom_images->sx);
      image = cairo_image_surface_create_for_data(prom_images->images_table[0], format, prom_images->sx, prom_images->sy, stride);
      cairo_set_source_surface(cr, image, 0, 0);
      cairo_paint(cr);
      cairo_surface_destroy(image);
    }
  }
  else
  {
    if (group->normalized)
    {

      for (i = 0; i < group->number_of_neurons * incrementation; i += incrementation)
      {
        switch (group->output_display)
        {
        case 0:
          val = group->neurons[i].s;
          break;
        case 1:
          val = group->neurons[i].s1;
          break;
        case 2:
          val = group->neurons[i].s2;
          break;
        default:
          val = 0;
          break;
        }
        if (i == 0)
        {
          group->val_min = val;
          group->val_max = val;
        }
        else
        {
          if (val < group->val_min) group->val_min = val;
          if (val > group->val_max) group->val_max = val;
        }
      }
    }

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0; i < group->columns * incrementation; i += incrementation)
      {
        switch (group->output_display)
        {
        case 0:
          val = group->neurons[i + j * group->columns * incrementation].s;
          break;
        case 1:
          val = group->neurons[i + j * group->columns * incrementation].s1;
          break;
        case 2:
          val = group->neurons[i + j * group->columns * incrementation].s2;
          break;
        default:
          val = 0;
          break;
        }
        ndg = niveauDeGris(val, group->val_min, group->val_max);

        switch (group->display_mode)
        {
        case DISPLAY_MODE_SQUARE:
          cairo_rectangle(cr, (i + (1 - ndg) / 2) * largeurNeuron+0.5, (j + (1 - ndg) / 2) * hauteurNeuron+0.5, (largeurNeuron - 2) * ndg + 1, (hauteurNeuron - 2) * ndg + 1); /* Pour garder un point central */
          break;
        case DISPLAY_MODE_INTENSITY:
          cairo_set_source_rgb(cr, ndg, ndg, ndg);
          cairo_rectangle(cr, i * largeurNeuron, j * hauteurNeuron, largeurNeuron - 1, hauteurNeuron - 1);
          cairo_fill(cr);
          break;
        case DISPLAY_MODE_BAR_GRAPH:
          cairo_rectangle(cr, i * largeurNeuron, (j + (1 - ndg)) * hauteurNeuron + 1, largeurNeuron - 1, hauteurNeuron * ndg - 1);
          break;
        case DISPLAY_MODE_TEXT:

          sprintf(label_text, "%f", val);
          cairo_rectangle(cr, i * largeurNeuron, j * hauteurNeuron, largeurNeuron - 5, hauteurNeuron);
          cairo_save(cr);
          cairo_clip(cr);
          cairo_move_to(cr, i * largeurNeuron, j * hauteurNeuron + TEXT_LINE_HEIGHT);
          cairo_show_text(cr, label_text);
          cairo_restore(cr); /* unclip */
          break;
        }
      }
    }
    cairo_fill(cr);
  }

  if (group == selected_group) //Si le groupe affiché dans cette fenêtre est sélectionné
  {
    //  cairo_set_line_width(cr, 10); Traits plus épais
    cairo_set_source_rgb(cr, RED);
    cairo_rectangle(cr, 0, 0, GTK_WIDGET(group->drawing_area)->allocation.width - 1, GTK_WIDGET(group->drawing_area)->allocation.height - 1);
    /*cairo_move_to(cr, 0, GTK_WIDGET(group->drawing_area)->allocation.height);
     cairo_line_to(cr, GTK_WIDGET(group->drawing_area)->allocation.width - 1, GTK_WIDGET(group->drawing_area)->allocation.height);*/
    cairo_stroke(cr); //Le contenu de cr est appliqué sur "zoneNeurones"
    //  cairo_set_line_width(cr, GRID_WIDTH); Retour aux traits fins*/
  }
  cairo_destroy(cr); //Destruction du calque
  gdk_threads_leave();

}

void architecture_display_update(GtkWidget *architecture_display, void *data)
{
  cairo_t *cr;
  char text[TEXT_MAX];
  double dashes[] =
    { 10.0, 20.0 };
  int i, group_id = 0, link_id, z;
  double x_offset = 0, y_offset = 0;
  type_script *script;
  type_group *group, *input_group;
  GdkEventButton *event = data;

  cr = gdk_cairo_create(architecture_display->window); //Crée un contexte Cairo associé à la drawing_area "zone"
  graphic.draw_links = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_connections));
  graphic.draw_net_links = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_net_connections));

  cairo_set_source_rgb(cr, WHITE);
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
          group = &script->groups[group_id];

          /* Box */
          cairo_rectangle(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale, LARGEUR_GROUPE, HAUTEUR_GROUPE);

          /* Test selection */
          if (event != NULL)
          {
            if (cairo_in_fill(cr, event->x, event->y)) selected_group = group;
          }
          if (group == selected_group) cairo_set_source_rgb(cr, RED);
          else color(cr, group);
          cairo_fill_preserve(cr);

          /* Texts  */
          cairo_save(cr);
          if (group != selected_group) cairo_clip(cr);
          gdk_cairo_set_source_color(cr, &couleurs[noir]);
          cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + TEXT_LINE_HEIGHT);
          cairo_show_text(cr, group->name);
          cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + 2 * TEXT_LINE_HEIGHT);
          cairo_show_text(cr, group->function);
          snprintf(text, TEXT_MAX, "%d x %d", group->columns, group->rows);
          cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + 3 * TEXT_LINE_HEIGHT);
          cairo_show_text(cr, text);
          cairo_stroke(cr);
          cairo_restore(cr); /* unclip */

          /* Internal links */
          if (graphic.draw_links)
          {
            for (link_id = 0; link_id < group->number_of_links; link_id++)
            {
              input_group = group->previous[link_id];

              if ((group == selected_group) || (input_group == selected_group))
              {
                cairo_set_source_rgb(cr, RED);
                cairo_set_line_width(cr, 3);
              }
              else cairo_set_line_width(cr, 1);
              cairo_move_to(cr, x_offset + input_group->x * graphic.x_scale + LARGEUR_GROUPE, y_offset + input_group->y * graphic.y_scale + HAUTEUR_GROUPE / 2); //Début de la liaison (à droite du groupe prédécesseur)
              cairo_line_to(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + HAUTEUR_GROUPE / 2); //Fin de la liaison (à gauche du groupe courant)
              cairo_stroke(cr);
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
          cairo_set_source_rgb(cr, RED);
          cairo_set_line_width(cr, 5);
        }
        else
        {
          gdk_cairo_set_source_color(cr, &couleurs[rouge]);
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
  cairo_destroy(cr);
}

