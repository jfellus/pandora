/*
 * graphic.h
 *
 *  Created on: 2 août 2012
 *      Author: arnablan
 */

#ifndef GRAPHIC_H
#define GRAPHIC_H

#include "japet.h"

#include <gtk/gtk.h>
#include <cairo.h>
/* cairo_t ne devrait pas exister ici */

typedef struct graphic
{
	int draw_links;
	int draw_net_links;
}type_graphic;

type_graphic graphic;

void dessinGroupe(cairo_t *cr, int a, int b, int c, int d, group *g, int y_offset,  int z, int zMax);

#endif /* GRAPHIC_H */
