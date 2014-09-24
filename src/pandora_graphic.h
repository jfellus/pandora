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
void test_selection(type_group* group, int u,int i,int j, float largeurNeuron, float hauteurNeuron,int incrementation);
void destroy_links(type_group* group);
#endif /* GRAPHIC_H */
