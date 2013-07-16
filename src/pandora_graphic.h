/**
 * pandora_graphic.h
 *
 *  Created on: 2 août 2012
 *      Author: arnablan
 *
 */

#ifndef PANDORA_GRAPHIC_H
#define PANDORA_GRAPHIC_H

#include "pandora.h"
#include <graphic_Tx.h>




typedef struct graphic
{
	int draw_links;
	int draw_net_links;
	float x_scale, y_scale, zx_scale, zy_scale;
}type_graphic;

/* "En-tete" de variables globales */
//extern GtkWidget *architecture_display; //La grande zone de dessin des liaisons entre groupes
extern type_graphic graphic;

float niveauDeGris(float val, float valMin, float valMax);
float CoordonneeYPoint(float val, float valMin, float valMax, float hauteurNeurone);
float coordonneeYZero(float valMin, float valMax, float hauteurNeurone);
void group_expose_neurons(type_group *group, gboolean lock_gdk_threads, gboolean update_frequence);

void group_update_frequence_values(type_group *group);
void draw_big_graph(type_group *group, cairo_t *cr, float frequence);
void update_big_graph_data(type_group *group);
void update_graph_data(type_group *group);

void graph_get_line_color(int num, float *r, float *g, float *b);
//void group_update_display(TxGraphic *cr, int a, int b, int c, int d, type_group *g, int y_offset,  int z, int zMax);
void architecture_set_view_point(GtkWidget *scrollbars, gdouble x, gdouble y);
void architecture_get_group_position(type_group *group, float *x, float *y);
void architecture_display_update(TxWidget *group_display, void *event);
void architecture_display_update_group(GtkWidget *architecture_display, type_group *group);
//void pandora_window_new();



#endif /* GRAPHIC_H */
