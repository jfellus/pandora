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

/** Structures **/

typedef struct graphic {
  int draw_links;
  int draw_net_links;
  float x_scale, y_scale, zx_scale, zy_scale;
} type_graphic;

typedef struct new_group_argument {
  type_group *group;
  float posx;
  float posy;
  GtkWidget *zone_neuron;
} new_group_argument;

typedef struct group_expose_argument {
  type_group *group;
  gboolean freq;
} group_expose_argument;

/** "En-tete" de variables globales **/

extern type_graphic graphic;
extern GdkColor couleurs[];

float niveauDeGris(float val, float valMin, float valMax);
float CoordonneeYPoint(float val, float valMin, float valMax, float hauteurNeurone);
float coordonneeYZero(float valMin, float valMax, float hauteurNeurone);
void group_expose_neurons(type_group *group, gboolean lock_gdk_threads, gboolean update_frequence);

void group_update_frequence_values(type_group *group);
void draw_big_graph(type_group *group, cairo_t *cr, float frequence);
void update_big_graph_data(type_group *group);
void update_graph_data(type_group *group);

void graph_get_line_color(int num, float *r, float *g, float *b);
void architecture_set_view_point(GtkWidget *scrollbars, gdouble x, gdouble y);
void architecture_get_group_position(type_group *group, float *x, float *y);
void architecture_display_update(GtkWidget *architecture_display, cairo_t *cr, void *data);

gboolean architecture_display_update_group(gpointer *user_data);

gboolean group_display_new_threaded(gpointer data);
gboolean group_expose_refresh(GtkWidget *widget, cairo_t *cr, gpointer user_data);
const char* tcolor(type_script *script);
void color(cairo_t *cr, type_group *g);
void clearColor(cairo_t *cr, type_group g);

void group_expose_neurons_test(type_group *group, gboolean update_frequence, cairo_t *cr);
void group_display_new(type_group *group, float pos_x, float pos_y, GtkWidget *zone_neurons);
void group_display_destroy(type_group *group);

int zoom_neurons(type_group* group, gboolean direction);
gboolean neuron_zooming(GtkWidget *pwidget, GdkEvent  *user_event, gpointer user_data);
float determine_ideal_length(type_group* group);
#endif /* GRAPHIC_H */
