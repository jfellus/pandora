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
 * pandora_graphic.h
 *
 *  Created on: 2 août 2012
 *      Author: arnablan
 *
 **/

#ifndef PANDORA_GRAPHIC_H
#define PANDORA_GRAPHIC_H

#include "pandora.h"


#define TEXT_LINE_HEIGHT 12
//#ifndef _SEE_TIME
//#define _SEE_TIME
//#endif

/** Structures **/
typedef struct new_group_argument {
  type_group *group;
  float posx;
  float posy;
  GtkWidget *zone_neuron;
  gboolean blocked;
} new_group_argument;

typedef struct group_expose_argument {
  type_group *group;
  gboolean freq;
} group_expose_argument;

typedef struct type_link_draw {
  type_group* group_pointed; //groupe correspondant au lien en cours
  int no_neuro_rel_pointed; //numéro de neuronne relativement au groupe
  int x_dep; //départ et arrivé du trait du lien
  int y_dep;
  int x_arr;
  int y_arr;

// type_link_draw* elem_suivant;
} type_link_draw;

/** "En-tête" de variables globales **/
extern gdouble move_neurons_old_x, move_neurons_old_y;
extern gboolean move_neurons_start;
extern gboolean open_neurons_start;
extern type_group *move_neurons_group;
extern type_link_draw links_to_draw;

/** En-tête de fonctions **/
float niveauDeGris(float val, float valMin, float valMax);
float CoordonneeYPoint(float val, float valMin, float valMax, float hauteurNeurone);
float coordonneeYZero(float valMin, float valMax, float hauteurNeurone);
//void group_expose_neurons(type_group *group, gboolean lock_gdk_threads, gboolean update_frequence);

void group_update_frequence_values(type_group *group);
void draw_big_graph(type_group *group, cairo_t *cr, float frequence);
void update_big_graph_data(type_group *group);
void update_graph_data(type_group *group);

void graph_get_line_color(int num, float *r, float *g, float *b);

gboolean group_display_new_threaded(gpointer data);
gboolean group_expose_refresh(GtkWidget *widget, cairo_t *cr, gpointer user_data);
const char* tcolor(type_script *script);
void color(cairo_t *cr, type_group *g);
void clearColor(cairo_t *cr, type_group g);

void group_expose_neurons(type_group *group, gboolean update_frequence, cairo_t *cr);
void group_display_new(type_group *group, float pos_x, float pos_y, GtkWidget *zone_neurons);
void group_display_destroy(type_group *group);

void zoom_neurons(type_group* group, gboolean direction, float *final_height, float *final_width);
gboolean neuron_zooming(GtkWidget *pwidget, GdkEvent *user_event, gpointer user_data);
void determine_ideal_length(type_group* group, float* largeur, float* hauteur);
gboolean button_press_neurons(GtkWidget *widget, GdkEvent *event, type_group *group);
gboolean button_press_on_neuron(GtkWidget *widget, GdkEvent *event, type_group *group);
gboolean button_release_on_neuron(GtkWidget *widget, GdkEvent *event, type_group *group);
void draw_links(GtkWidget *zone_neuron, cairo_t *cr, void *data, int x_zone, int y_zone, int xd_zone, int yd_zone, float info);
void emit_signal_stop_to_promethe(int no_neuro, type_script* script);
void emit_signal_to_promethe(int no_neuro, type_script* script);
gboolean draw_all_links(GtkWidget *zone_neuron, cairo_t *cr, void *data);
float calcul_pallier(float actual_length, gboolean direction);
void test_selection(type_group* group, int u, int i, int j, float largeurNeuron, float hauteurNeuron, int incrementation);
void destroy_links(type_group* group);
void init_image(unsigned char* image_data, prom_images_struct * const prom_images, type_group * const group, cairo_t * const cr, int nb_band);
void correspondance_3b(unsigned char * image_data, prom_images_struct * const prom_images, type_group* const group);
void correspondance_4b(unsigned char * image_data, prom_images_struct * const prom_images, type_group* const group);
void correspondance_reste(unsigned char * image_data, prom_images_struct * const prom_images, type_group* const group);
void destruction_image(type_group * const group);
void check_ok_bord(type_group *group);
void correspondance_display_intensity(unsigned char * image_data, prom_images_struct * const prom_images, type_group* const group);
static inline void display_image(type_group *group, prom_images_struct* prom_images, unsigned char* image_data, cairo_t * const cr);
static inline void common_display_inside(int *u, int i, int j, int incrementation, type_group *group, float largeurNeuron, float hauteurNeuron, cairo_t * const cr, float* r, float* v, float* b);
static inline void common_display(type_group *group, cairo_t * const cr, float frequence, int adresse, int incrementation, float largeurNeuron, float hauteurNeuron, char* label_text);
static inline void mode_graph(type_group *group, int i, int j, int incrementation, int sortie, float largeurNeuron, float hauteurNeuron, cairo_t *cr);


static inline float select_compo(float a, float b, float c, float alpha, char no)
{
  switch (no)
  {
  case 1:
    return a;
    break;
  case 2:
    return b;
    break;
  case 3:
    return c;
    break;
  case 4:
    return alpha;
    break;
  default:
    EXIT_ON_ERROR("Mauvaise utilisation de la fonction select_compo dans pandora_graphic.h\n");
    break;
  }
  return -1;
}

static inline void common_display_inside(int *u, int i, int j, int incrementation, type_group *group, float largeurNeuron, float hauteurNeuron, cairo_t * const cr, float* r, float* v, float* b)
{

  *u = (i - incrementation + 1) / incrementation;
  group->param_neuro_pandora[i + j * group->columns * incrementation].center_x = (double) (((*u * largeurNeuron) + (largeurNeuron - 5) / 2));
  group->param_neuro_pandora[i + j * group->columns * incrementation].center_y = (double) (((j * hauteurNeuron + hauteurNeuron / 2)));
  test_selection(group, *u, i, j, largeurNeuron, hauteurNeuron, incrementation);
  if (group->param_neuro_pandora[i + j * group->columns * incrementation].selected)
  {
    *r = select_compo(RED, 1);
    *v = select_compo(RED, 2);
    *b = select_compo(RED, 3);
    cairo_set_source_rgba(cr, RED);
  }
  else
  {
    *r = select_compo(WHITE, 1);
    *v = select_compo(WHITE, 2);
    *b = select_compo(WHITE, 3);
    cairo_set_source_rgba(cr, WHITE);
  }
}

static inline void common_display(type_group *group, cairo_t * const cr, float frequence, int adresse, int incrementation, float largeurNeuron, float hauteurNeuron, char* label_text)
{
  int sortie=group->output_display, i, j;
  float val;
  int u;
  float r, v, b;
  prom_images_struct* prom_images;
  unsigned char * image_data=NULL;
  float ndg=0, yVal, y0=0;
  float min = group->val_min, max = group->val_max;
  float tmp=0;
  cairo_surface_t * image;

  switch (group->display_mode)
  {
  case DISPLAY_MODE_SQUARE:
    cairo_set_source_rgba(cr, BLACK);
    cairo_paint(cr);
    sortie = group->output_display;
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
      {
        val = *(&(group->neurons[i + j * group->columns * incrementation].s) + adresse);
        common_display_inside(&u, i, j, incrementation, group, largeurNeuron, hauteurNeuron, cr, &r, &v, &b);
        ndg = niveauDeGris(val, min, max);
        cairo_rectangle(cr, (u + (1 - ndg) * .5) * largeurNeuron + 0.5, (j + (1 - ndg) * .5) * hauteurNeuron + 0.5, (largeurNeuron - 2) * ndg + 1, (hauteurNeuron - 2) * ndg + 1);
        cairo_fill(cr);
      }
    }

    break;

  case DISPLAY_MODE_CIRCLE:
    cairo_set_source_rgba(cr, BLACK);
    cairo_paint(cr);
    sortie = group->output_display;
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
      {
        val = *(&(group->neurons[i + j * group->columns * incrementation].s) + adresse);
        common_display_inside(&u, i, j, incrementation, group, largeurNeuron, hauteurNeuron, cr, &r, &v, &b);
        ndg = niveauDeGris(val, min, max);
        cairo_arc(cr, (i + .5) * largeurNeuron, (j + .5) * hauteurNeuron, ndg * largeurNeuron * .5, 0, 2 * M_PI);
        cairo_fill(cr);
      }
    }
    break;

  case DISPLAY_MODE_INTENSITY:
    cairo_set_source_rgba(cr, BLACK);
    cairo_paint(cr);
    sortie = group->output_display;
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
      {
        val = *(&(group->neurons[i + j * group->columns * incrementation].s) + adresse);
        common_display_inside(&u, i, j, incrementation, group, largeurNeuron, hauteurNeuron, cr, &r, &v, &b);
        ndg = niveauDeGris(val, min, max);
        cairo_set_source_rgba(cr, ndg * r, ndg * v, ndg * b, 1);
        cairo_rectangle(cr, u * largeurNeuron, j * hauteurNeuron, largeurNeuron - 1, hauteurNeuron - 1);
        cairo_fill(cr);
      }
    }
    break;

  case DISPLAY_MODE_INTENSITY_FAST:
    cairo_set_source_rgba(cr, BLACK);
    cairo_paint(cr);
    sortie = group->output_display;
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
      {
        val = *(&(group->neurons[i + j * group->columns * incrementation].s) + adresse);
        common_display_inside(&u, i, j, incrementation, group, largeurNeuron, hauteurNeuron, cr, &r, &v, &b);
        prom_images = (prom_images_struct*) group->ext;
        if (group->image_ready == FALSE)
        {
          init_image(image_data, prom_images, group, cr, -1);
          image = group->surface_image;
          image_data = cairo_image_surface_get_data(image);
          image_data[j * group->stride + i] = 255 - niveauDeGris(val, min, max) * 255;
        }
        else
        {
          image = group->surface_image;
          image_data = cairo_image_surface_get_data(image);
          image_data[j * group->stride + i] = 255 - niveauDeGris(val, min, max) * 255;
        }
      }
    }

    cairo_set_source_rgba(cr, WHITE);
    cairo_paint(cr);

    cairo_save(cr);
    cairo_scale(cr, largeurNeuron, hauteurNeuron);

    image = group->surface_image;

    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    break;

  case DISPLAY_MODE_BAR_GRAPH:
    cairo_set_source_rgba(cr, BLACK);
    cairo_paint(cr);
    sortie = group->output_display;
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
      {
        val = *(&(group->neurons[i + j * group->columns * incrementation].s) + adresse);
        common_display_inside(&u, i, j, incrementation, group, largeurNeuron, hauteurNeuron, cr, &r, &v, &b);
        if (group->rows > 10 || group->columns > 10) break;
        y0 = coordonneeYZero(min, max, hauteurNeuron);
        yVal = CoordonneeYPoint(val, min, max, hauteurNeuron);
        if (yVal < y0)
        {
          tmp = yVal;
          yVal = y0;
          y0 = tmp;
        }
        yVal = yVal - y0; // hauteur du rectangle à afficher.
        y0 += j * hauteurNeuron + 1;

        cairo_rectangle(cr, u * largeurNeuron, y0, largeurNeuron - 1, yVal);
        cairo_fill(cr);
      }
      cairo_move_to(cr, 0, y0 + .5);
      cairo_line_to(cr, group->columns * largeurNeuron, y0 + .5);
      cairo_set_line_width(cr, (double) 1);
      cairo_stroke(cr);
    }

  //  cairo_fill(cr);

    break;

  case DISPLAY_MODE_TEXT:
    cairo_set_source_rgba(cr, BLACK);
    cairo_paint(cr);
    sortie = group->output_display;
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
      {
        val = *(&(group->neurons[i + j * group->columns * incrementation].s) + adresse);
        common_display_inside(&u, i, j, incrementation, group, largeurNeuron, hauteurNeuron, cr, &r, &v, &b);
        sprintf(label_text, "%f", val);
        group->param_neuro_pandora[i + j * group->columns * incrementation].center_x = (double) (((u * largeurNeuron) + (largeurNeuron - 5) / 2));
        group->param_neuro_pandora[i + j * group->columns * incrementation].center_y = (double) (((j * hauteurNeuron + hauteurNeuron / 2)));

        cairo_rectangle(cr, u * largeurNeuron, j * hauteurNeuron, largeurNeuron - 5, hauteurNeuron);
        cairo_save(cr);
        cairo_clip(cr);
        cairo_move_to(cr, u * largeurNeuron, j * hauteurNeuron + TEXT_LINE_HEIGHT);
        cairo_show_text(cr, label_text);
        cairo_restore(cr); /* unclip */
      }
    }
    break;

  case DISPLAY_MODE_GRAPH:
    cairo_set_source_rgba(cr, WHITE);
    cairo_paint(cr);
    sortie = group->output_display;
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
      {
        val = *(&(group->neurons[i + j * group->columns * incrementation].s) + adresse);
        common_display_inside(&u, i, j, incrementation, group, largeurNeuron, hauteurNeuron, cr, &r, &v, &b);
        mode_graph(group, i, j, incrementation, sortie, largeurNeuron, hauteurNeuron, cr);
      }
    }
    break;

  case DISPLAY_MODE_BIG_GRAPH:
    cairo_set_source_rgba(cr, WHITE);
    cairo_paint(cr);
    sortie = group->output_display;
    update_big_graph_data(group);
    draw_big_graph(group, cr, frequence);
    break;

  }

}

static inline void mode_graph(type_group *group, int i, int j, int incrementation, int sortie, float largeurNeuron, float hauteurNeuron, cairo_t *cr)
{
  float **values = NULL;
  float tmp;
  int indexDernier=-1, x;
  int indexAncien=-1, indexTmp;
  float min = group->val_min, max = group->val_max;
  float yVal;
  int k;

  //L'init est ici, si jamais tabvalue n'existe pas
  if (group->tabValues == NULL)
  {
    group->tabValues = createTab4(group->rows, group->columns);
    group->indexAncien = createTab2(group->rows, group->columns, -1);
    group->indexDernier = createTab2(group->rows, group->columns, -1);
    if (group->rows * group->columns <= BIG_GRAPH_MAX_NEURONS_NUMBER) group->afficher = createTab2(group->rows, group->columns, TRUE);
  }

  values = group->tabValues[j][i];
  indexDernier = group->indexDernier[j][i];
  indexAncien = group->indexAncien[j][i];

  if (indexDernier == -1)
  {
    indexDernier = indexAncien = 0;
    for (k = 0; k < NB_Max_VALEURS_ENREGISTREES; k++)
    {
      values[0][k] = group->neurons[i + j * group->columns * incrementation].s;
      values[1][k] = group->neurons[i + j * group->columns * incrementation].s1;
      values[2][k] = group->neurons[i + j * group->columns * incrementation].s2;
    }
  }
  //Update tabValues
  else if (indexAncien == indexDernier + 1 || (indexDernier == NB_Max_VALEURS_ENREGISTREES - 1 && indexAncien == 0))
  {
    if (indexDernier == NB_Max_VALEURS_ENREGISTREES - 1)
    {
      indexDernier = 0;
      indexAncien = 1;
    }
    else if (indexAncien == NB_Max_VALEURS_ENREGISTREES - 1)
    {
      indexDernier++;
      indexAncien = 0;
    }
    else
    {
      indexDernier++;
      indexAncien++;
    }
    values[0][indexDernier] = group->neurons[i + j * group->columns * incrementation].s;
    values[1][indexDernier] = group->neurons[i + j * group->columns * incrementation].s1;
    values[2][indexDernier] = group->neurons[i + j * group->columns * incrementation].s2;
  }
  // si le tableau n'est pas plein, ajout de la valeur à la suite des valeurs déja présentes.
  else
  {
    indexDernier++;
    values[0][indexDernier] = group->neurons[i + j * group->columns * incrementation].s;
    values[1][indexDernier] = group->neurons[i + j * group->columns * incrementation].s1;
    values[2][indexDernier] = group->neurons[i + j * group->columns * incrementation].s2;
  }

  indexTmp = indexDernier;
  tmp = 0;
  k = 0;
  // permet de tracer le graphe dans la zone réservée au neurone (le graphe n'est pas relié).
  for (x = ((i - incrementation + 1) / incrementation + 1) * largeurNeuron - 2; x > i * largeurNeuron + 3; x--, k = 1)
  {
    tmp = values[sortie][indexTmp];
    yVal = CoordonneeYPoint(tmp, min, max, hauteurNeuron);
    //cairo_rectangle(cr, x, yVal + j * hauteurNeuron + 1, 1, 2);
    if (k == 0) cairo_move_to(cr, x, yVal);
    else cairo_line_to(cr, x, yVal + j * hauteurNeuron + 1);
    //if (indexAncien == indexTmp) break;
    indexTmp--;
    if (indexTmp < 0) indexTmp = NB_Max_VALEURS_ENREGISTREES - 1;
  }

  if (tmp < 0)
  {
    cairo_stroke(cr);
    cairo_set_source_rgba(cr, WHITE);
  }
  /// permet de tracer les traits verticaux (séparation entre deux graphes situés sur la même ligne dans le groupe).
  if (i < (group->columns - 1) * incrementation)
  {
    cairo_rectangle(cr, ((i - incrementation + 1) / incrementation + 1) * largeurNeuron, j * hauteurNeuron, 0.5, hauteurNeuron);
  }
  /// permet de tracer la droite y=0.
  if (!(min > 0 || 0 > max))
  {
    cairo_rectangle(cr, ((i - incrementation + 1) / incrementation) * largeurNeuron, j * hauteurNeuron + coordonneeYZero(min, max, hauteurNeuron) + 0.5, largeurNeuron, 0.5);
  }
  cairo_stroke(cr);
  // sauvegarde des indices courants.
  group->indexDernier[j][i] = indexDernier;
  group->indexAncien[j][i] = indexAncien;
}

static inline void display_image(type_group *group, prom_images_struct* prom_images, unsigned char* image_data, cairo_t * const cr)
{
  cairo_surface_t * image;

  if (group->ext != NULL && ((prom_images_struct*) group->ext)->image_number > 0)
  {
    prom_images = (prom_images_struct*) group->ext;
    gtk_widget_set_size_request(group->drawing_area, (gint) prom_images->sx, (gint) prom_images->sy);

    if (group->image_selected_index >= prom_images->image_number) group->image_selected_index = 0;

    switch (prom_images->nb_band)
    {
    case 3:
      if (group->image_ready == FALSE)
      {
        init_image(image_data, prom_images, group, cr, prom_images->nb_band);
      }
      else
      {
        image = group->surface_image;
        cairo_surface_flush(image);
        image_data = cairo_image_surface_get_data(image);
        correspondance_3b(image_data, prom_images, group);
        cairo_surface_mark_dirty(image);
        cairo_set_source_surface(cr, image, 0, 0);
      }
      // cairo_set_source_surface(cr, image, 0, 0);
      cairo_paint(cr);
      break;
    case 4: //N&B float
      if (group->image_ready == FALSE)
      {
        init_image(image_data, prom_images, group, cr, prom_images->nb_band);
      }
      else
      {
        image = group->surface_image;
        cairo_surface_flush(image);
        image_data = cairo_image_surface_get_data(image);
        correspondance_4b(image_data, prom_images, group);
        cairo_surface_mark_dirty(image);
        cairo_set_source_surface(cr, image, 0, 0);
      }
      cairo_paint(cr);
      break;
    default: //N&B int
      if (group->image_ready == FALSE)
      {
        init_image(image_data, prom_images, group, cr, prom_images->nb_band);
      }
      else
      {
        image = group->surface_image;
        cairo_surface_flush(image);
        image_data = cairo_image_surface_get_data(image);
        correspondance_reste(image_data, prom_images, group);
        cairo_surface_mark_dirty(image);
        cairo_set_source_surface(cr, image, 0, 0);
      }
      cairo_paint(cr);
      break;
    }
  }
  else
  {
    gtk_widget_set_size_request(group->drawing_area, 140, 80);
    cairo_set_source_rgba(cr, WHITE);
    cairo_paint(cr);
    cairo_set_source_rgba(cr, BLACK);
    cairo_move_to(cr, gtk_widget_get_allocated_width(GTK_WIDGET(group->drawing_area)) / 2 - 45, gtk_widget_get_allocated_height(GTK_WIDGET(group->drawing_area)) / 2);
    cairo_show_text(cr, "No image received");
  }

}

#endif /* GRAPHIC_H */
