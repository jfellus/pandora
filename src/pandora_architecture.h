/**
 * pandora_architecure.h
 *
 *  Created on: 5 août 2013
 *      Author: Nils Beaussé
 **/

#ifndef PANDORA_ARCHITECURE_H_
#define PANDORA_ARCHITECURE_H_

#include "common.h"
#include "pandora.h"

#define TEXT_MAX 32
#define TEXT_OFFSET 4
#define TEXT_LINE_HEIGHT 12

/** Structures **/
typedef struct graphic {
  int draw_links;
  int draw_net_links;
  float x_scale, y_scale, zx_scale, zy_scale;
} type_graphic;

/** "En-tete" de variables globales **/
extern type_graphic graphic;
extern GtkWidget *architecture_display;
extern gboolean architecture_display_dragging_currently;
extern gdouble architecture_display_cursor_x;
extern gdouble architecture_display_cursor_y;
extern gdouble new_x, new_y, old_x, old_y;

void zoom_in(GdkDevice *pointer);
void zoom_out(GdkDevice *pointer);
void architecture_display_button_pressed(GtkWidget *pWidget, GdkEvent *user_event, gpointer pdata);
void architecture_display_button_released(GtkWidget *pWidget, GdkEventButton *event, gpointer pdata);
void architecture_display_drag_motion(GtkWidget *pWidget, GdkEventMotion *event, gpointer pdata);
gboolean architecture_display_scroll_event(GtkWidget *pWidget, GdkEventScroll *event, gpointer pdata);
void key_press_event(GtkWidget *pWidget, GdkEventKey *event);
void architecture_get_group_position(type_group *group, float *x, float *y);
void architecture_set_view_point(GtkWidget *scrollbars, gdouble x, gdouble y);
void architecture_display_update(GtkWidget *architecture_display, cairo_t *cr, void *data);
gboolean queue_draw_archi(gpointer data);

#endif /* PANDORA_ARCHITECURE_H_ */
