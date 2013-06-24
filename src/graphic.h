/*
 * graphic.h
 *
 *  Created on: 2 août 2012
 *      Author: arnablan
 */

#ifndef GRAPHIC_H
#define GRAPHIC_H


#include "pandora.h"

#include <graphic_Tx.h>


typedef struct graphic
{
	int draw_links;
	int draw_net_links;
	float x_scale, y_scale, zx_scale, zy_scale;
}type_graphic;

type_graphic graphic;




void expose_group_ext(type_group *group);

void graph_get_line_color(int num, float *r, float *g, float *b);
void group_expose_neurons(type_group *group, gboolean lock_gdk_threads);
void group_update_display(TxGraphic *cr, int a, int b, int c, int d, type_group *g, int y_offset,  int z, int zMax);
void architecture_set_view_point(GtkWidget *scrollbars, float x, float y);
void architecture_get_group_position(type_group *group, float *x, float *y);
void architecture_display_update(TxWidget *group_display, void *event);
void pandora_window_new();


extern TxWidget *check_button_draw_connections, *check_button_draw_net_connections;


#endif /* GRAPHIC_H */
