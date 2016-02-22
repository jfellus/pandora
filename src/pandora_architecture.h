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

enum {
  SAVE_D = 0, SAVE_S2, SAVE_S1, SAVE_S
};


/** Structures **/
typedef struct graphic {
  int draw_links;
  int draw_net_links;
  float x_scale, y_scale, zx_scale, zy_scale;
} type_graphic;

/** "En-tête" de variables globales **/
extern type_graphic graphic;
extern GtkWidget *architecture_display;
extern gboolean architecture_display_dragging_currently;
extern gdouble architecture_display_cursor_x;
extern gdouble architecture_display_cursor_y;
extern gdouble new_x, new_y, old_x, old_y;

/** En-tête de fonctions **/
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
void add_scroll_to_save(type_group *group, float pos_x, float pos_y, GtkWidget* architecture_display);
void choice_combo_box_changed(GtkComboBox *comboBox, gpointer data);
void manage_scroll_to_save(type_group *group,gboolean selected_for_save,int x,int y,GtkWidget* architecture_display);
gboolean queue_draw_archi(gpointer data);

#endif /* PANDORA_ARCHITECURE_H_ */
