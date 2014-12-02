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
 * pandora_graphic.c
 *
 *  Created on: ???
 *      Author: ???
 *
 **/

#include "pandora_graphic.h"
#include "common.h"
#include "pandora_architecture.h"

#define DIGITS_MAX 32
#define NB_DIGITS 7
#define TEXT_LINE_HEIGHT 12

const GdkRGBA colors[] = {{LIGHTGREEN}, {LIGHTMAGENTA}, {DARKYELLOW}, {LIGHTBLUE}, {LIGHTRED}, {LIGHTORANGE}, {LIGHTANISE}};


/** Contient les fonctions, variables et evenements associés à la zone nomée neurons_frame,
 zone d'affichage des groupes de neuronne
 Contient également les fonctions de dessins globale et à partager avec pandora_architecture,
 ceci afin de creer un nouveau fichier ne contenant que peu de chose**/

/* Variables Globales pour ce fichier*/
gdouble move_neurons_old_x, move_neurons_old_y;
gboolean move_neurons_start = FALSE;
gboolean open_neurons_start = FALSE; //Pour ouvrir un frame_neuron
type_group *move_neurons_group = NULL; //Pour bouger un frame_neuron
extern pthread_cond_t cond_loading;
#ifdef _SEE_TIME
  struct timeval tv_start, tv_end;
#endif

/* TODO optimiser */
float niveauDeGris(float val, float valMin, float valMax)
{
  float ndg = (val - valMin) / (valMax - valMin);
  if (ndg < 0) ndg = 0;
  else if (ndg > 1) ndg = 1;
  return ndg;
}

float CoordonneeYPoint(float val, float valMin, float valMax, float hauteurNeurone)
{
  if (val <= valMin) return hauteurNeurone;
  if (val >= valMax) return 0;

  return ((valMax - val) / (valMax - valMin)) * hauteurNeurone;
}

float coordonneeYZero(float valMin, float valMax, float hauteurNeurone)
{
  return CoordonneeYPoint(0, valMin, valMax, hauteurNeurone);
}

void graph_get_line_color(int num, float *r, float *g, float *b)
{
  switch (num)
  {
  case 0:
    *r = 0.0;
    *g = 0.0;
    *b = 0.0;
    break;
  case 1:
    *r = 0.0;
    *g = 1.0;
    *b = 0.0;
    break;
  case 2:
    *r = 0.0;
    *g = 0.0;
    *b = 1.0;
    break;
  case 3:
    *r = 1.0;
    *g = 0.0;
    *b = 0.0;
    break;
  case 4:
    *r = 0.5;
    *g = 0.5;
    *b = 0.5;
    break;
  case 5:
    *r = 0.0;
    *g = 0.85;
    *b = 0.85;
    break;
  case 6:
    *r = 0.85;
    *g = 0.85;
    *b = 0.0;
    break;
  case 7:
    *r = 1.0;
    *g = 0.0;
    *b = 1.0;
    break;

  }
}

/**
 *  Affiche les neurones d'une petite fenêtre
 */

//transfere les arguments de la structure argument passée en parametre a la fonction group_display_new, évite de reccreer une fonction.
gboolean group_display_new_threaded(gpointer data)
{
  new_group_argument* argument_recup = NULL;
  float positionx = 0;
  float positiony = 0;
  type_group *group_to_send = NULL;
  GtkWidget *zone_neuron = NULL;
  gboolean blocked = FALSE;

  //Récuperation des données.
  argument_recup = (new_group_argument*) data;
  positionx = argument_recup->posx;
  positiony = argument_recup->posy;
  group_to_send = argument_recup->group;
  zone_neuron = argument_recup->zone_neuron;
  blocked = argument_recup->blocked;

  pthread_mutex_lock(&mutex_script_caracteristics);
  group_display_new(group_to_send, (float) positionx, (float) positiony, zone_neuron);
  if (group_to_send->previous_output_display != group_to_send->output_display) resize_group(group_to_send);
  pthread_mutex_unlock(&mutex_script_caracteristics);

  //pthread_mutex_lock (&mutex_loading);
  //On signal au programme ayant appelé que cette appel est terminé et qu'il peut donc continuer
  free(argument_recup);
  if (blocked)
  {
    pthread_cond_signal(&cond_loading);
  }

  //pthread_mutex_unlock (&mutex_loading);

  return FALSE;
}

/**Cette fonction est appelé par l'evenement draw de chaque groupe affichés et permet de les raffraichir si besoin**/
gboolean group_expose_refresh(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  gboolean ref_freq = FALSE;
  type_group *group = NULL;
  (void) widget;
  group = (type_group*) user_data;

  if (group->ok == TRUE && group->ok_display == TRUE && cr != NULL)
  {
    ref_freq = group->refresh_freq;

    pthread_mutex_lock(&mutex_script_caracteristics);
#ifdef _SEE_TIME
  gettimeofday(&tv_start, NULL);
#endif
    group_expose_neurons(group, ref_freq, cr);
#ifdef _SEE_TIME
  gettimeofday(&tv_end, NULL);
#endif
    //group_expose_neurons(group, FALSE, FALSE);
    pthread_mutex_unlock(&mutex_script_caracteristics);
#ifdef _SEE_TIME
  printf("Delay: %d\n", tv_end.tv_usec - tv_start.tv_usec);
#endif
  }


  return TRUE;
}

void group_update_frequence_values(type_group *group)
{
  float time = g_timer_elapsed(group->timer, NULL);
  g_timer_start(group->timer);

  // calcul de la fréquence moyenne basée sur les FREQUENCE_MAX_VALUES_NUMBER dernières itérations.
  if (group->frequence_index_last == -1)
  {
    group->frequence_index_last = group->frequence_index_older = 0;
    group->frequence_values[0] = group->counter / time;
  }
  else if (group->frequence_index_older == group->frequence_index_last + 1 || (group->frequence_index_last == FREQUENCE_MAX_VALUES_NUMBER - 1 && group->frequence_index_older == 0))
  {
    if (group->frequence_index_last == FREQUENCE_MAX_VALUES_NUMBER - 1)
    {
      group->frequence_index_last = 0;
      group->frequence_index_older = 1;
    }
    else if (group->frequence_index_older == FREQUENCE_MAX_VALUES_NUMBER - 1)
    {
      group->frequence_index_last++;
      group->frequence_index_older = 0;
    }
    else
    {
      group->frequence_index_last++;
      group->frequence_index_older++;
    }
    group->frequence_values[group->frequence_index_last] = group->counter / time;
  }
  else
  {
    group->frequence_index_last++;
    group->frequence_values[group->frequence_index_last] = group->counter / time;
  }
}

void draw_big_graph(type_group *group, cairo_t *cr, float frequence)
{
  int i, k, indexTmp, indexDernier, indexAncien, sortie = group->output_display;
  float hauteur = (float) gtk_widget_get_allocated_height(GTK_WIDGET(group->drawing_area));
  float largeur = (float) gtk_widget_get_allocated_width(GTK_WIDGET(group->drawing_area));
  float min = group->val_min, max = group->val_max, tmp, r, g, b, x, yVal;
  float **values;
  char label_text[LABEL_MAX];

  if (group->normalized)
  {
    min = 0.0;
    max = -1.0;
    for (i = 0; i < group->number_of_courbes; i++)
    {
      values = group->courbes[i].values;
      if (group->courbes[i].last_index != -1) for (k = 0; k < NB_Max_VALEURS_ENREGISTREES; k++)
      {
        tmp = values[sortie][k];
        if (max < min) min = max = tmp;
        else if (tmp < min) min = tmp;
        else if (max < tmp) max = tmp;
      }
    }
  }

  if (min > max) min = max - 1;

  snprintf(label_text, LABEL_MAX, "<b>%s</b> - %s \n[%.2f | %.2f] - %.3f Hz", group->name, group->function, min, max, frequence);
  gtk_label_set_markup(group->label, label_text);

  // trace la droite d'équation y=0.
  cairo_set_source_rgba(cr, BLACK);
  cairo_rectangle(cr, 1, coordonneeYZero(min, max, hauteur) + 0.5, largeur - 2, 0.5);
  cairo_fill(cr);

  for (i = 0; i < group->number_of_courbes; i++)
  {
    if (group->courbes[i].show == TRUE) // si il faut tracer la courbe pour ce neurone, l'exécution continue.
    {
      // application de la couleur associée à la courbe.
      graph_get_line_color(i, &r, &g, &b);
      cairo_set_source_rgba(cr, r, g, b, 1);

      indexAncien = group->courbes[i].old_index;
      indexDernier = group->courbes[i].last_index;
      values = group->courbes[i].values;

      // trace la courbe (les point sont reliés).
      indexTmp = indexDernier;
      tmp = 0;
      k = 0;
      for (x = largeur - 2.0; !(x < 1); x -= 2.0, k = 1)
      {
        tmp = values[sortie][indexTmp];
        yVal = CoordonneeYPoint(tmp, min, max, hauteur - 4.5);
        if (k == 0) cairo_move_to(cr, x, yVal + 1);
        else cairo_line_to(cr, x, yVal + 1);
        if (indexAncien == indexTmp) break;
        indexTmp--;
        if (indexTmp < 0) indexTmp = NB_Max_VALEURS_ENREGISTREES - 1;
      }
      cairo_stroke(cr); // relie les points.
    }
  }
}

void update_graph_data(type_group *group)
{
  int indexDernier, indexAncien, i, j, k, u;
  int incrementation = group->number_of_neurons / (group->columns * group->rows);
  float **values;

  if (stop == TRUE || group->rows > 10 || group->columns > 10) return;

  for (j = 0; j < group->rows; j++)
  {
    for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
    {
      u = (i - incrementation + 1);
      values = group->tabValues[j][u / incrementation];
      indexDernier = group->indexDernier[j][u / incrementation];
      indexAncien = group->indexAncien[j][u / incrementation];

      // ajout de la derniére valeur au tableau des valeurs utilisé pour tracer le graphe.

      // si aucune valeur enregistrée, initialisation du tableau.
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
      // si le tableau est plein, remplacement de la valeur la plus ancienne par la valeur à ajouter. l'enregistrement des valeurs est similaire à une file circulaire.
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

      // sauvegarde des indices courants.
      group->indexDernier[j][u / incrementation] = indexDernier;
      group->indexAncien[j][u / incrementation] = indexAncien;
    }
  }
}

void update_big_graph_data(type_group *group)
{
  int indexDernier, indexAncien, column, line, k, l, incrementation = group->number_of_neurons / (group->columns * group->rows);
  float **values;

  if (stop == TRUE) return;

  for (l = 0; l < group->number_of_courbes; l++)
  {
    values = group->courbes[l].values;
    column = group->courbes[l].column;
    line = group->courbes[l].line;
    indexAncien = group->courbes[l].old_index;
    indexDernier = group->courbes[l].last_index;

    // si aucune valeur enregistrée, initialisation du tableau.
    if (indexDernier == -1)
    {
      indexDernier = indexAncien = 0;
      for (k = 0; k < NB_Max_VALEURS_ENREGISTREES; k++)
      {
        values[0][k] = group->neurons[column + line * group->columns * incrementation].s;
        values[1][k] = group->neurons[column + line * group->columns * incrementation].s1;
        values[2][k] = group->neurons[column + line * group->columns * incrementation].s2;
      }
    }
    // si le tableau est plein, remplacement de la valeur la plus ancienne par la valeur à ajouter. l'enregistrement des valeurs est similaire à une file circulaire.
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
      values[0][indexDernier] = group->neurons[column + line * group->columns * incrementation].s;
      values[1][indexDernier] = group->neurons[column + line * group->columns * incrementation].s1;
      values[2][indexDernier] = group->neurons[column + line * group->columns * incrementation].s2;
    }
    // si le tableau n'est pas plein, ajout de la valeur à la suite des valeurs déja présentes.
    else
    {
      indexDernier++;
      values[0][indexDernier] = group->neurons[column + line * group->columns * incrementation].s;
      values[1][indexDernier] = group->neurons[column + line * group->columns * incrementation].s1;
      values[2][indexDernier] = group->neurons[column + line * group->columns * incrementation].s2;
    }

    // sauvegarde des indices courants.
    group->courbes[l].last_index = indexDernier;
    group->courbes[l].old_index = indexAncien;
  }
}

float calcul_pallier(float actual_length, gboolean direction)
{
  int retour = 0;
  float retour_ree = 0.0;

  if (direction == TRUE)
  {
    if (20 <= (int) actual_length && (int) actual_length < 100)
    {
      retour = (int) actual_length + 10;
      retour_ree = (float) retour;
    }
    else if (2 <= (int) actual_length && (int) actual_length < 20)
    {
      retour = (int) actual_length + 2;
      retour_ree = (float) retour;
    }
    else if ((int) actual_length == 1)
    {
      retour = (int) actual_length + 1;
      retour_ree = (float) retour;
    }
    else if ((int) actual_length >= 100)
    {
      retour = 100;
      retour_ree = (float) retour;
    }
    else
    {
      retour_ree = actual_length * 1.5;
    }
  }
  if (direction == FALSE)
  {
    if ((int) actual_length >= 20)
    {
      retour = (int) actual_length - 10;
      retour_ree = (float) retour;
    }
    else if (2 < (int) actual_length && (int) actual_length < 20)
    {
      retour = (int) actual_length - 2;
      retour_ree = (float) retour;
    }
    else if ((int) actual_length == 2)
    {
      retour = (int) actual_length - 1;
      retour_ree = (float) retour;
    }
    else
    {
      retour_ree = actual_length / 1.5;
    }
  }
  return retour_ree;
}

void zoom_neurons(type_group* group, gboolean direction, float *final_height, float *final_width)
{
  float actual_height = group->neurons_height;
  float actual_width = group->neurons_width;

  *final_height = calcul_pallier(actual_height, direction);
  *final_width = calcul_pallier(actual_width, direction);

  if (*final_height * (float) group->rows < 40.0)
  {
    *final_height = 40.0 / (float) group->rows;
  }
  if (*final_width * (float) group->columns < 40.0)
  {
    *final_width = 40.0 / (float) group->columns;
  }

  if (abs((int) (*final_height * (float) group->rows - *final_width * (float) group->columns)) < 20) // Si on est pas dans un cas spécial on peut mettre les neuronne au format carré
  {
    if (*final_height > *final_width) *final_height = *final_width;
    if (*final_height < *final_width) *final_width = *final_height;
  }

  if (*final_height * (float) group->rows < 40.0)
  {
    *final_height = 40.0 / (float) group->rows;
  }
  if (*final_width * (float) group->columns < 40.0)
  {
    *final_width = 40.0 / (float) group->columns;
  }

}

gboolean neuron_zooming(GtkWidget *pwidget, GdkEvent *user_event, gpointer user_data)
{
  type_group* group = (type_group*) user_data;
  GdkEventScroll *event = (GdkEventScroll*) user_event;
  (void) pwidget;

  if ((event->direction == GDK_SCROLL_UP) && (event->state & GDK_CONTROL_MASK))
  {
    zoom_neurons(group, TRUE, &(group->neurons_height), &(group->neurons_width));
    resize_group(group);
    return TRUE;
  }
  else if ((event->direction == GDK_SCROLL_DOWN) && (event->state & GDK_CONTROL_MASK))
  {
    zoom_neurons(group, FALSE, &(group->neurons_height), &(group->neurons_width));
    resize_group(group);
    return TRUE;
  }
  else
  {
    return FALSE;
  }

}

float rectifi_pallier(float ideal_neuro)
{
  float retour;
  //test de bonne utilisation de la fonction
  if (ideal_neuro > 0.0)
  {
    // si on est au dela de 20 on va de 10 en 10 donc remise du retour à la dixaine inférieure
    if (ideal_neuro > 20.0)
    {
      retour = ideal_neuro / 10.0;
      retour = (float) floor(retour);
      retour = retour * 10;
    }
    //sinon on va de 2 en 2 donc on commence par en faire un int à l'unité inférieur puis on le ramene à un chiffre pair (inférieur d'une unité)
    else if (ideal_neuro >= 2.0)
    {
      retour = (float) floor(ideal_neuro);
      if (((int) retour % 2) != 0)
      {
        retour = retour - 1;
      }
    }
    else if (ideal_neuro < 2 && ideal_neuro > 1)
    {
      retour = 1;
    }
    // en dessous de 1 et supérieur à 0 on garde le float trouvé;
    else retour = ideal_neuro;
  }
  else
  {
    printf("ATTENTION : Mauvaise utilisation de la fonction determine_ideal_length, remise par defaut de la taille du groupe");
    retour = 20.0;
  }
  return retour;
}
void determine_ideal_length(type_group* group, float* largeur, float* hauteur)
{

  int column = group->columns;
  int rows = group->rows;
  int ideal_largeur = 0;
  int ideal_hauteur = 0;
  float ideal_largeur_neuro = 0.0;
  float ideal_hauteur_neuro = 0.0;

  //On établis des cran sur l'apparence génerale comme utilisé avant
  if (column == 0) ideal_largeur = 40;
  if (column == 1) ideal_largeur = 100;
  else if (column <= 16) ideal_largeur = 300;
  else if (column <= 128) ideal_largeur = 400;
  else if (column <= 256) ideal_largeur = 700;
  else ideal_largeur = 1000;

  if (rows == 0) ideal_hauteur = 40;
  if (rows == 1) ideal_hauteur = 100;
  else if (rows <= 16) ideal_hauteur = 300;
  else if (rows <= 128) ideal_hauteur = 400;
  else if (rows <= 256) ideal_hauteur = 700;
  else ideal_hauteur = 1000;

  // on en déduit la largeur et la hauteur des neuronnes
  ideal_largeur_neuro = ((float) ideal_largeur) / ((float) column);
  ideal_hauteur_neuro = ((float) ideal_hauteur) / ((float) rows);

  // on remet les neuronnes en carré en prenant le plus petit si on est dans un cas classique
  if (ideal_largeur_neuro >= 1.0 && ideal_hauteur_neuro >= 1)
  {
    if (ideal_hauteur_neuro < ideal_largeur_neuro) ideal_largeur_neuro = ideal_hauteur_neuro;
    else ideal_hauteur_neuro = ideal_largeur_neuro;
  }

  ideal_hauteur_neuro = rectifi_pallier(ideal_hauteur_neuro);
  ideal_largeur_neuro = rectifi_pallier(ideal_largeur_neuro);

  // traitement de cas particulier.

  if (ideal_hauteur_neuro * (float) group->rows < 40.0)
  {
    ideal_hauteur_neuro = 50.0 / (float) group->rows;
  }
  if (ideal_largeur_neuro * (float) group->columns < 40.0)
  {
    ideal_largeur_neuro = 40.0 / (float) group->columns;
  }
  if (ideal_hauteur_neuro * (float) group->rows > 1000)
  {
    ideal_hauteur_neuro = 1000.0 / (float) group->rows;
  }
  if (ideal_largeur_neuro * (float) group->columns > 1000)
  {
    ideal_largeur_neuro = 1000.0 / (float) group->columns;
  }

  *largeur = ideal_largeur_neuro;
  *hauteur = ideal_hauteur_neuro;

  return;
}

gboolean button_press_neurons(GtkWidget *widget, GdkEvent *event, type_group *group)
{
  GdkEventButton *event_button;
  GtkAdjustment *vert, *horiz;
  event_button = (GdkEventButton*) event;
  (void) widget;

  switch (event_button->button)
  {
  case 1:
    if(event_button->state & GDK_CONTROL_MASK)
    {
      //ctrl + clic = centrage sur architecture grp
      horiz = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars));
      vert = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars));
      gtk_adjustment_set_value(horiz, group->x * graphic.x_scale - (gtk_widget_get_allocated_width(scrollbars))/2 + LARGEUR_GROUPE/2 );
      gtk_adjustment_set_value(vert, group->y * graphic.y_scale - (gtk_widget_get_allocated_height(scrollbars))/2 + HAUTEUR_GROUPE/2 );
    }
    else
    {
      move_neurons_old_x = event_button->x;
      move_neurons_old_y = event_button->y;
      move_neurons_start = TRUE;
      move_neurons_group = group;

      selected_group = group; //Le groupe associé à la fenêtre dans laquelle on a cliqué est également sélectionné

      if (event_button->type == GDK_2BUTTON_PRESS) on_group_display_clicked(NULL, group); //Affichage de la petite fenetre etc.. (?)
    }

    break;

  case 2:
    group_display_destroy(group);
    break;

  case 3:
    break;

  default:
    break;
  }

//On actualise l'affichage
  gtk_widget_queue_draw(architecture_display);
  //architecture_display_update(architecture_display, NULL);

  return TRUE;
}

void group_display_new(type_group *group, float pos_x, float pos_y, GtkWidget *zone_neurons)
{
  GtkWidget *hbox, *principal_hbox, *button, *button_label;
  int i, j, k;
  float r, g, b;
  char p_legende_texte[100];
  int nb_lin, nb_col;
  float largeur, hauteur;
  //int largeur_neur=0;
  //group_expose_argument arg;
  if (group->widget != NULL || group->ok != TRUE) return;

  group->label = gtk_label_new("");

  //initialisation
  /* Test : on init uniquement les param de ce qui concerne le big graph a la selection de ce mode.
  if (group->rows <= 10 && group->columns <= 10)
  {
    group->tabValues = createTab4(group->rows, group->columns);
    group->indexAncien = createTab2(group->rows, group->columns, -1);
    group->indexDernier = createTab2(group->rows, group->columns, -1);
    if (group->rows * group->columns <= BIG_GRAPH_MAX_NEURONS_NUMBER) group->afficher = createTab2(group->rows, group->columns, TRUE);
  }
 */
  group->previous_display_mode = DISPLAY_MODE_INTENSITY;
  group->frequence_index_last = -1;
  for (i = 0; i < FREQUENCE_MAX_VALUES_NUMBER; i++)
    group->frequence_values[i] = -1;

  //Création de la petite fenétre
  group->widget = gtk_aspect_frame_new("", 0, 0, 1, TRUE);
  gtk_widget_set_double_buffered(group->widget, TRUE);
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  gtk_layout_put(GTK_LAYOUT(zone_neurons), group->widget, (gint) pos_x, (gint) pos_y);
  // gtk_layout_put(GTK_LAYOUT(zone_neurons), group->widget, pos_x, pos_y);

  gtk_box_pack_start(GTK_BOX(hbox), group->label, TRUE, FALSE, 0);

  gtk_frame_set_label_widget(GTK_FRAME(group->widget), hbox);

  group->button_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(group->button_vbox), TRUE);

  if (group->courbes == NULL)
  {
    group->number_of_courbes = (group->rows * group->columns < BIG_GRAPH_MAX_NEURONS_NUMBER ? group->rows * group->columns : BIG_GRAPH_MAX_NEURONS_NUMBER);
    group->courbes = malloc(group->number_of_courbes * sizeof(data_courbe));
    i = 0;
    for (j = 0; j < group->rows && j * group->columns + i < group->number_of_courbes; j++)
    {
      for (i = 0; i < group->columns && j * group->columns + i < group->number_of_courbes; i++)
      {
        graph_get_line_color(j * group->columns + i, &r, &g, &b);
        sprintf(p_legende_texte, "<b><span foreground=\"#%02X%02X%02X\"> %dx%d</span></b>\n", (int) (r * 255), (int) (g * 255), (int) (b * 255), i, j);
        button_label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(button_label), p_legende_texte);
        button = gtk_check_button_new();
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
        gtk_container_add(GTK_CONTAINER(button), button_label);
        gtk_widget_set_size_request(button, BUTTON_WIDTH, BUTTON_HEIGHT);
        gtk_box_pack_start(GTK_BOX(group->button_vbox), button, TRUE, FALSE, 0);
        g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(on_group_display_show_or_mask_neuron), &(group->courbes[j * group->columns + i].show));

        group->courbes[j * group->columns + i].check_box = button;
        group->courbes[j * group->columns + i].check_box_label = button_label;
        group->courbes[j * group->columns + i].column = i;
        group->courbes[j * group->columns + i].line = j;
        group->courbes[j * group->columns + i].last_index = -1;
        group->courbes[j * group->columns + i].show = TRUE;
        group->courbes[j * group->columns + i].values = (float **) malloc(sizeof(float *) * 3);
        group->courbes[j * group->columns + i].indice = j * group->columns + i;
        for (k = 0; k < 3; k++)
          group->courbes[j * group->columns + i].values[k] = (float *) malloc(sizeof(float) * NB_Max_VALEURS_ENREGISTREES);
      }
      if (i == group->columns) i = 0;
    }
  }
  else
  {
    for (i = 0; i < group->number_of_courbes; i++)
    {
      graph_get_line_color(i, &r, &g, &b);
      sprintf(p_legende_texte, "<b><span foreground=\"#%02X%02X%02X\"> %dx%d</span></b>\n", (int) (r * 255), (int) (g * 255), (int) (b * 255), group->courbes[i].column, group->courbes[i].line);
      button_label = gtk_label_new("");
      gtk_label_set_markup(GTK_LABEL(button_label), p_legende_texte);
      button = gtk_check_button_new();
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), group->courbes[i].show);
      gtk_container_add(GTK_CONTAINER(button), button_label);
      gtk_widget_set_size_request(button, BUTTON_WIDTH, BUTTON_HEIGHT);
      gtk_box_pack_start(GTK_BOX(group->button_vbox), button, TRUE, FALSE, 0);
      g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(on_group_display_show_or_mask_neuron), &(group->courbes[i].show));

      group->courbes[i].check_box = button;
      group->courbes[i].check_box_label = button_label;
      group->courbes[i].last_index = -1;
    }
  }

  group->drawing_area = gtk_drawing_area_new();
  gtk_widget_set_double_buffered(group->drawing_area, TRUE);

  if (group->columns > 0) nb_col = group->columns;
  else nb_col = 1;
  if (group->rows > 0) nb_lin = group->rows;
  else nb_lin = 1;

  if (!(group->from_file))
  {
    determine_ideal_length(group, &(group->neurons_width), &(group->neurons_height));
  }
  largeur = (float) nb_col * group->neurons_width;
  hauteur = (float) nb_lin * group->neurons_height;

  //gtk_widget_set_size_request(group->drawing_area, get_width_height(group->columns), get_width_height(group->rows));
  gtk_widget_set_size_request(group->drawing_area, (gint) largeur, (gint) hauteur);

  principal_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous(GTK_BOX(principal_hbox), FALSE);

  gtk_box_pack_start(GTK_BOX(principal_hbox), group->drawing_area, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(principal_hbox), group->button_vbox, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(group->widget), principal_hbox);
  //gtk_widget_add_events(GTK_WIDGET(group->widget), GDK_SCROLL_MASK);
  gtk_widget_set_events(GTK_WIDGET(group->drawing_area), GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK | GDK_EXPOSURE_MASK | GDK_BUTTON_RELEASE_MASK);

  //gtk_widget_add_events(group->drawing_area, GDK_BUTTON_RELEASE_MASK);
  // gtk_widget_set_events(group->drawing_area, GDK_ALL_EVENTS_MASK);
  g_signal_connect(G_OBJECT(group->drawing_area), "scroll-event", G_CALLBACK(neuron_zooming), group);
  g_signal_connect(G_OBJECT(group->widget), "button-press-event", G_CALLBACK(button_press_neurons), group);

  //g_signal_connect(G_OBJECT(group->widget), "draw", G_CALLBACK(group_expose_refresh), group); //TODO : supprimer si aucun bug à long terme
  g_signal_connect(G_OBJECT(group->drawing_area), "draw", G_CALLBACK(group_expose_refresh), group); //semble fonctionner plutot bien
  g_signal_connect(G_OBJECT(group->drawing_area), "button-press-event", G_CALLBACK(button_press_on_neuron), group);
  g_signal_connect(G_OBJECT(group->drawing_area), "button-release-event", G_CALLBACK(button_release_on_neuron), group);
  //g_signal_connect(G_OBJECT(group->drawing_area), "button-release-event", G_CALLBACK(drag_drop_neuron_frame), NULL);
  g_signal_connect(G_OBJECT(group->widget), "motion-notify-event", G_CALLBACK(neurons_frame_drag_group), NULL);
  g_signal_connect(G_OBJECT(group->widget), "button-release-event", G_CALLBACK(drag_drop_neuron_frame), NULL);

  group->counter = 0;
  group->timer = g_timer_new();

  gtk_widget_show_all(group->widget);
  gtk_widget_hide(group->button_vbox);
  if (refresh_mode != REFRESH_MODE_MANUAL)
  {
    if (group->output_display == 3) pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_START, group->id, group->script->name);
    else pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, group->script->name);
  }
  groups_to_display[number_of_groups_to_display] = group;
  group->idDisplay = number_of_groups_to_display;
  number_of_groups_to_display++;
  group->ok_display = TRUE;

}

void group_display_destroy(type_group *group)
{

  int i;
  destroy_links(group);
  if (group->output_display == 3) pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_EXT_STOP, group->id, group->script->name);
  else pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, group->script->name);

  for (i = 0; i < number_of_groups_to_display; i++)
  {
    if (groups_to_display[i] == group)
    {
      /* Le group en cours est remplacé par le dernier group de la liste qui ne sera plus pris en compte */
      groups_to_display[i] = groups_to_display[number_of_groups_to_display - 1];
      number_of_groups_to_display--;
      group->ok_display = FALSE;
      break;
    }
  }
  if (group->indexAncien != NULL) destroy_tab_2(group->indexAncien, group->rows);
  group->indexAncien = NULL;
  if (group->indexDernier != NULL) destroy_tab_2(group->indexDernier, group->rows);
  group->indexDernier = NULL;
  if (group->tabValues != NULL) destroy_tab_4(group->tabValues, group->columns, group->rows);
  group->tabValues = NULL;

  gtk_label_set_text(GTK_LABEL(group->label), "");
  gtk_widget_destroy(group->widget);
  group->widget = NULL;
  group->drawing_area = NULL;
  group->ok_display = FALSE;
  group->from_file = FALSE;

}

gboolean button_release_on_neuron(GtkWidget *widget, GdkEvent *event, type_group *group)
{
  struct timespec present_time;
  GdkEventButton *event_button = (GdkEventButton*) event;
  ;
  (void) widget;

  if (event != NULL && event_button->button == 1)
  {
    clock_gettime( CLOCK_REALTIME, &present_time);
    if (((present_time.tv_nsec + 1000000000 * present_time.tv_sec) - (group->press_time.tv_nsec + 1000000000 * group->press_time.tv_sec)) <= 250000000)
    {
      group->x_event = event_button->x;
      group->y_event = event_button->y;
    }
    else
    {
      group->x_event = -1;
      group->y_event = -1;
    }

  }

  return FALSE;
}

gboolean button_press_on_neuron(GtkWidget *widget, GdkEvent *event, type_group *group)
{
  //  cairo_t *cr;
  GdkEventButton *event_button;
  event_button = (GdkEventButton*) event;
  //struct timespec tp;
  (void) widget;
  // float length_x=0;
  // float length_y=0;
  // int column=0;
  // int row=0;
  // int incrementation = 0;
  // int neuron_number_display = 0;
  // int neuron_real_number = 0;

  // cr = gdk_cairo_create(gtk_widget_get_window(widget)); //Crée un contexte Cairo associé à la drawing_area "zone"
  //gdk_cairo_set_source_window(cr, gtk_widget_get_window(widget), 0, 0);
  // cairo_destroy(cr);

  /* Test selection */
  if (event != NULL && event_button->button == 1)
  {
    //setlocale(LC_NUMERIC, "C");
    clock_gettime( CLOCK_REALTIME, &(group->press_time));
    //group->x_event=event_button->x;
    //group->y_event=event_button->y;
    /*length_x=(float)(group->columns)/(float)gtk_widget_get_allocated_width(widget);
     length_y=(float)(group->rows)/(float)gtk_widget_get_allocated_height(widget);


     row=(event_button->y)/length_y;
     column=(event_button->x)/length_x;
     incrementation = group->number_of_neurons / (group->columns * group->rows);
     //neuron_number_display=column + row * group->columns * incrementation + incrementation - 1;
     neuron_number_display=column + row * group->columns;
     neuron_real_number=neuron_number_display*incrementation+incrementation - 1;

     printf("indice visible = %d \n",neuron_number_display);
     printf("indice veritable = %d \n",neuron_real_number);
     */
  }
  else
  {
    group->x_event = -1;
    group->y_event = -1;
  }
  return FALSE;
}

void group_expose_neurons(type_group *group, gboolean update_frequence, cairo_t *cr)
{
  //Attention : il est tres important de garder cet ordre pour les mutex, notemment le fait de ne pas entourer gdk_threads_enter, au risque de provoquer des dead-end due à plusieurs mutex
  float ndg = 0, val, tmp = 0, x, y0, yVal, min = group->val_min, max = group->val_max, frequence = 0;
  float largeurNeuron, hauteurNeuron;
  //float r, g, b;
  int i, j, k = 0;
  int u;
  int incrementation;
  int stride;
  cairo_surface_t *image;
  prom_images_struct *prom_images;
  unsigned char *image_data;

  // float time;
  char label_text[LABEL_MAX];

  int sortie = group->output_display;
  float **values = NULL;
  int indexDernier = -1, indexAncien = -1, indexTmp;

  gtk_container_set_reallocate_redraws(GTK_CONTAINER(group->widget), FALSE);
  //gtk_container_set_reallocate_redraws (GTK_CONTAINER(group->drawing_area), FALSE);
  //gtk_container_set_resize_mode(GTK_CONTAINER(group->widget), GTK_RESIZE_QUEUE);
  //gtk_widget_set_size_request(group->widget, get_width_height(group->columns), get_width_height(group->rows));
  incrementation = group->number_of_neurons / (group->columns * group->rows);

  // ajustement de la taille du widget en fonction du mode d'affichage et de la sortie.

  if (group->previous_display_mode != group->display_mode || group->previous_output_display != group->output_display)
  {
    group->previous_display_mode = group->display_mode;
    group->previous_output_display = group->output_display;
  }

  //Début du dessin
  //printf("je dessine le groupe no %d\n",group->id);
  //Dimensions d'un neurone
  largeurNeuron = (float) gtk_widget_get_allocated_width(GTK_WIDGET(group->drawing_area)) / (float) group->columns;
  hauteurNeuron = (float) gtk_widget_get_allocated_height(GTK_WIDGET(group->drawing_area)) / (float) group->rows;
  //largeurNeuron=5;
  //hauteurNeuron=5;
  //  if (largeurNeuron <= hauteurNeuron) hauteurNeuron = largeurNeuron;
  // else largeurNeuron = hauteurNeuron;
  //largeurNeuron=hauteurNeuron=get_width_height(group->columns,group->rows);
  if (update_frequence == TRUE) group_update_frequence_values(group);

  for (i = 0; i < FREQUENCE_MAX_VALUES_NUMBER; i++)
    if (group->frequence_values[i] > -1)
    {
      k++;
      frequence += group->frequence_values[i];
    }
  if (k > 0) frequence = frequence / k;

  group->counter = 0;

  // construction du label du groupe sous la forme :   nom_groupe (en gras) - nom_fonction
  //                                                   [min | max] - fréquence moyenne
  snprintf(label_text, LABEL_MAX, "<b>%s</b> - %s \n[%.2f | %.2f] - %.3f Hz", group->name, group->function, min, max, frequence);
  gtk_label_set_markup(group->label, label_text);

  // si la sortie vaut 3, on affiche l'image ou un indicateur si aucune image a été reçue.
  if (group->output_display == 3)
  {
    if (group->ext != NULL && ((prom_images_struct*) group->ext)->image_number > 0)
    {
      prom_images = (prom_images_struct*) group->ext;
      gtk_widget_set_size_request(group->drawing_area, (gint) prom_images->sx, (gint) prom_images->sy);

      if (group->image_selected_index >= (int) prom_images->image_number) group->image_selected_index = 0;
      k = group->image_selected_index;

      stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, (int) prom_images->sx);
      if(prom_images->nb_band == 3)//Affichage couleur
      {
        image_data = malloc(stride * (int) prom_images->sy);
        for (i = 0; (unsigned int) i < prom_images->sx * prom_images->sy; i++)
        {
          image_data[i * 4    ] = prom_images->images_table[k][i * 3 + 2];
          image_data[i * 4 + 1] = prom_images->images_table[k][i * 3 + 1];
          image_data[i * 4 + 2] = prom_images->images_table[k][i * 3    ];
          image_data[i * 4 + 3] = prom_images->images_table[k][i * 3 + 2];
        }
        image = cairo_image_surface_create_for_data(image_data, CAIRO_FORMAT_RGB24, (int) prom_images->sx, (int) prom_images->sy, stride);
        cairo_set_source_surface(cr, image, 0, 0);
        cairo_paint(cr);
        cairo_surface_finish(image);
        cairo_surface_destroy(image);
        free(image_data);
      }
      else if(group->output_display == 4)//N&B float
      {
        cairo_set_source_rgba(cr, WHITE);
        cairo_paint(cr);
        image_data = malloc(stride * (int) prom_images->sy);
        if (image_data != NULL)
        {
          for (i = 0; (unsigned int)i < prom_images->sx * prom_images->sy; i++)
          {
            image_data[i] = (unsigned char) ((float *) prom_images->images_table[k])[i] * 255;
          }
          image = cairo_image_surface_create_for_data(image_data, CAIRO_FORMAT_A8, (int) prom_images->sx, (int) prom_images->sy, stride);
          cairo_set_source_surface(cr, image, 0, 0);
          cairo_paint(cr);
          cairo_surface_finish(image);
          cairo_surface_destroy(image);
          free(image_data);
        }
      }
      else//N&B int
      {
        cairo_set_source_rgba(cr, WHITE);
        cairo_paint(cr);
        image_data = malloc(stride * (int) prom_images->sy);
        if (image_data != NULL)
        {
          for (i = 0; (unsigned int) i < prom_images->sx * prom_images->sy; i++)
          {
            image_data[i] = 255 - prom_images->images_table[k][i];
          }
          image = cairo_image_surface_create_for_data(image_data, CAIRO_FORMAT_A8, (int) prom_images->sx, (int) prom_images->sy, stride);
          cairo_set_source_surface(cr, image, 0, 0);
          cairo_paint(cr);
          cairo_surface_finish(image);
          cairo_surface_destroy(image);
          free(image_data);
        }
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
  } // END si image à afficher
  else if(group->display_mode == DISPLAY_MODE_INTENSITY_FAST)
  {
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, group->columns);
    cairo_set_source_rgba(cr, WHITE);
    cairo_paint(cr);
    image_data = malloc(stride * group->rows);
    for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
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
        image_data[j * stride + i] = 255 - niveauDeGris(val, min, max) * 255;
      }
    }

    cairo_scale(cr, largeurNeuron, hauteurNeuron);
    image = cairo_image_surface_create_for_data(image_data, CAIRO_FORMAT_A8, group->columns, group->rows, stride);
    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
    cairo_scale(cr, 1/largeurNeuron, 1/hauteurNeuron);
    cairo_surface_finish(image);
    cairo_surface_destroy(image);
    free(image_data);
  }
  else
  {
    // fond blanc si le mode est big graph
    if (group->display_mode == DISPLAY_MODE_BIG_GRAPH) cairo_set_source_rgba(cr, WHITE);
    else cairo_set_source_rgba(cr, BLACK);

    cairo_paint(cr);
    cairo_set_source_rgba(cr, WHITE);
    // variables utilisées pour l'affichage des graphe.
    sortie = group->output_display;

    if (group->display_mode == DISPLAY_MODE_BIG_GRAPH)
    {
      update_big_graph_data(group);
      draw_big_graph(group, cr, frequence);
    }
    else for (j = 0; j < group->rows; j++)
    {
      for (i = 0 + incrementation - 1; i < group->columns * incrementation; i += incrementation)
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
        u = (i - incrementation + 1) / incrementation;
        group->param_neuro_pandora[i + j * group->columns * incrementation].center_x = (double) (((u * largeurNeuron) + (largeurNeuron - 5) / 2));
        group->param_neuro_pandora[i + j * group->columns * incrementation].center_y = (double) (((j * hauteurNeuron + hauteurNeuron / 2)));
        test_selection(group, u, i, j, largeurNeuron, hauteurNeuron, incrementation);
cairo_set_source_rgba(cr, WHITE);
        switch (group->display_mode)
        {
          case DISPLAY_MODE_SQUARE:
            ndg = niveauDeGris(val, min, max);

            cairo_rectangle(cr, (u + (1 - ndg) * .5) * largeurNeuron + 0.5, (j + (1 - ndg) *  .5) * hauteurNeuron + 0.5, (largeurNeuron - 2) * ndg + 1, (hauteurNeuron - 2) * ndg + 1);
            break;

        case DISPLAY_MODE_INTENSITY:
            ndg = niveauDeGris(val, min, max);

            cairo_set_source_rgba(cr, ndg, ndg, ndg, 1);
            cairo_rectangle(cr, u * largeurNeuron, j * hauteurNeuron, largeurNeuron - 1, hauteurNeuron - 1);
            cairo_fill(cr);
            break;

        case DISPLAY_MODE_CIRCLE:
            ndg = niveauDeGris(val, min, max);

            cairo_arc(cr, (i+.5) * largeurNeuron, (j+.5) * hauteurNeuron, ndg*largeurNeuron*.5, 0, 2*M_PI);
            cairo_fill(cr);
            break;

          case DISPLAY_MODE_BAR_GRAPH:
            if(group->rows > 10 || group->columns > 10) break;
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
            break;

          case DISPLAY_MODE_TEXT:
            cairo_set_source_rgba(cr, WHITE);
            sprintf(label_text, "%f", val);
            group->param_neuro_pandora[i + j * group->columns * incrementation].center_x = (double) (((u * largeurNeuron) + (largeurNeuron - 5) / 2));
            group->param_neuro_pandora[i + j * group->columns * incrementation].center_y = (double) (((j * hauteurNeuron + hauteurNeuron / 2)));

            cairo_rectangle(cr, u * largeurNeuron, j * hauteurNeuron, largeurNeuron - 5, hauteurNeuron);
            cairo_save(cr);
            cairo_clip(cr);
            cairo_move_to(cr, u * largeurNeuron, j * hauteurNeuron + TEXT_LINE_HEIGHT);
            cairo_show_text(cr, label_text);
            cairo_restore(cr); /* unclip */
            break;

          case DISPLAY_MODE_GRAPH:
            cairo_set_source_rgba(cr, WHITE);

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
            break;
        }
      }
      if(group->display_mode  == DISPLAY_MODE_BAR_GRAPH)
      {
        cairo_fill(cr);
        cairo_move_to(cr, 0, y0 + .5);
        cairo_line_to(cr, group->columns * largeurNeuron, y0 + .5);
        cairo_set_line_width (cr, (double) 1);
        cairo_stroke(cr);
      }
    }
    cairo_fill(cr);
  }//FIN Si pas une image à afficher

  if (group == selected_group) //Si le groupe affiché dans cette fenêtre est sélectionné
  {
    //  cairo_set_line_width(cr, 10); Traits plus épais
    cairo_set_source_rgba(cr, RED);
    cairo_rectangle(cr, 0, 0, gtk_widget_get_allocated_width(GTK_WIDGET(group->drawing_area)) - 1, gtk_widget_get_allocated_height(GTK_WIDGET(group->drawing_area)) - 1);
    cairo_stroke(cr); //Le contenu de cr est appliqué sur "zoneNeurones"

  }
  group->refresh_freq = FALSE;

  //gtk_cairo_transform_to_window (cr,GTK_WIDGET(group->widget),GDK_WINDOW(zone_neurons));
  draw_all_links(zone_neurons, cr, group->drawing_area);
  gtk_container_set_reallocate_redraws(GTK_CONTAINER(group->widget), TRUE);

}

void draw_links(GtkWidget *zone_neuron, cairo_t *cr, void *data, int x_zone, int y_zone, int xd_zone, int yd_zone, float info)
{
  GtkWidget *widget = (GtkWidget*) data;
  gint x_fin = 0;
  gint y_fin = 0;
  gint xd_fin = 0;
  gint yd_fin = 0;
  float largeur = info * 5.0;
  char text[TEXT_MAX];
  (void) data;

  if (data != NULL)
  {

    gtk_widget_translate_coordinates(zone_neuron, widget, x_zone, y_zone, &x_fin, &y_fin);
    gtk_widget_translate_coordinates(zone_neuron, widget, xd_zone, yd_zone, &xd_fin, &yd_fin);

  }
  else
  {
    x_fin = x_zone;
    y_fin = y_zone;
    xd_fin = xd_zone;
    yd_fin = yd_zone;
  }

  if (largeur <= 0.1 && largeur >= -0.1)
  {
    cairo_set_source_rgba(cr, DARKGREEN);
    largeur = 1.0;
  }
  else if (largeur < -0.1)
  {
    cairo_set_source_rgba(cr, DARKBLUE);
    largeur = -largeur;
  }
  else cairo_set_source_rgba(cr, RED);

  cairo_move_to(cr, x_fin, y_fin);
  cairo_line_to(cr, xd_fin, yd_fin);
  cairo_set_line_width(cr, (double) largeur);
  cairo_stroke(cr);
  if (draw_links_info)
  {
    cairo_set_source_rgba(cr, BLACK);
    cairo_move_to(cr, (x_fin + xd_fin) / 2 + 1, (y_fin + yd_fin) / 2 + 1);
    snprintf(text, TEXT_MAX, "%.2f", info);
    cairo_show_text(cr, text);
    cairo_stroke(cr);
  }

}

gboolean draw_all_links(GtkWidget *zone_neuron, cairo_t *cr, void *data)
{
  int i = 0, nbr_coeff = 0, j = 0, no_neuron_pointed = 0;
  gint xdep, ydep, xarr, yarr;
  float info = 0.0;
  type_group* group_pointed = NULL;
  // void * widget_arr, widget_dep;

  //TODO : faire le test d'existence de lien soit par la mise en place d'une variable nbr de lien totale soit autrement.
  for (i = 0; i < number_of_groups_to_display; i++)
  {
    if (groups_to_display[i]->neuro_select >= 0)
    {
      if (groups_to_display[i]->param_neuro_pandora[groups_to_display[i]->neuro_select].links_ok && groups_to_display[i]->param_neuro_pandora[groups_to_display[i]->neuro_select].selected)
      {
        nbr_coeff = groups_to_display[i]->neurons[groups_to_display[i]->neuro_select].nbre_coeff;
        for (j = 0; j < nbr_coeff; j++)
        {
          group_pointed = groups_to_display[i]->param_neuro_pandora[groups_to_display[i]->neuro_select].links_to_draw[j].group_pointed;
          if (group_pointed->ok_display == 0) break;
          no_neuron_pointed = groups_to_display[i]->param_neuro_pandora[groups_to_display[i]->neuro_select].links_to_draw[j].no_neuro_rel_pointed;
          xdep = (gint) (groups_to_display[i]->param_neuro_pandora[groups_to_display[i]->neuro_select].center_x);
          ydep = (gint) (groups_to_display[i]->param_neuro_pandora[groups_to_display[i]->neuro_select].center_y);
          xarr = (gint) (group_pointed->param_neuro_pandora[no_neuron_pointed].center_x);
          yarr = (gint) (group_pointed->param_neuro_pandora[no_neuron_pointed].center_y);

          gtk_widget_translate_coordinates(GTK_WIDGET(groups_to_display[i]->drawing_area), zone_neuron, xdep, ydep, &xdep, &ydep);
          gtk_widget_translate_coordinates(GTK_WIDGET(group_pointed->drawing_area), zone_neuron, xarr, yarr, &xarr, &yarr);

          info = groups_to_display[i]->param_neuro_pandora[groups_to_display[i]->neuro_select].coeff[j].val;
          draw_links(zone_neuron, cr, data, xarr, yarr, xdep, ydep, info);
        }

        //      groups_to_display[i]->param_neuro_pandora->links_to_draw
      }

    }
  }
  return FALSE; // propage l'event
}

void destroy_links(type_group* group)
{
  int no_neuro_rel = 0;
  int no_neuro_abs = 0;
  no_neuro_rel = group->neuro_select;

  if(no_neuro_rel!=-1) {
    group->param_neuro_pandora[no_neuro_rel].links_ok = FALSE;
    group->param_neuro_pandora[no_neuro_rel].selected = FALSE;
  } else
    printf("WARNING : no_neuro_rel==-1 cannot destroy links...\n");
  group->neuro_select = -1;

  no_neuro_abs = group->firstNeuron + no_neuro_rel;

  emit_signal_to_promethe(no_neuro_abs, group->script);

}

void emit_signal_to_promethe(int no_neuro, type_script* script)
{
  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURO_LINKS_START, no_neuro, script->name);
}

void emit_signal_stop_to_promethe(int no_neuro, type_script* script)
{
  pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURO_LINKS_STOP, no_neuro, script->name);
}

//Appelé ~ 53000fois, à simplifier/diminuer le nombre d'appel
void test_selection(type_group* group, int u, int i, int j, float largeurNeuron, float hauteurNeuron, int incrementation)
{
  if (group->x_event > 0) //si l'event de selection n'a pas déja été traité
  {
    if (group->x_event >= (u * largeurNeuron + 0.5) && group->x_event < (u * largeurNeuron + 0.5 + (largeurNeuron - 2) + 1) && group->y_event >= (j * hauteurNeuron + 0.5) && group->y_event < (j * hauteurNeuron + 0.5 + (hauteurNeuron - 2) + 1))
    {
      // on est selectionné
      if (i + j * group->columns * incrementation == group->number_of_neurons - 1) // on est allé jusqu'au bout du tableau de neurone pour la remise à zero
      {
        group->x_event = -1;
        group->y_event = -1;
      }
      if ((group->neuro_select >= 0) && (group->neuro_select != (i + j * group->columns * incrementation)))
      {
        group->param_neuro_pandora[group->neuro_select].selected = FALSE;
        emit_signal_stop_to_promethe((group->firstNeuron + group->neuro_select), group->script);
        emit_signal_to_promethe(group->firstNeuron + i + j * group->columns * incrementation, group->script);
      }

      if (group->neuro_select < 0)
      {
        emit_signal_to_promethe(group->firstNeuron + i + j * group->columns * incrementation, group->script); //déduit du premier ele du groupe et de i et j le num de la neuronne veritable dans le grand tab des neuro de promethe.
        group->param_neuro_pandora[i + j * group->columns * incrementation].selected = TRUE;
        group->neuro_select = i + j * group->columns * incrementation;
      }
      else if (group->neuro_select >= 0 && group->neuro_select == (i + j * group->columns * incrementation))
      {
        emit_signal_stop_to_promethe(group->firstNeuron + i + j * group->columns * incrementation, group->script);
        group->param_neuro_pandora[i + j * group->columns * incrementation].selected = FALSE;
        group->neuro_select = -1;

      }
      else
      {
        group->param_neuro_pandora[i + j * group->columns * incrementation].selected = TRUE;
        group->neuro_select = i + j * group->columns * incrementation;
      }

    }
    else
    {

      if ((group->neuro_select >= 0) && (group->neuro_select != (i + j * group->columns * incrementation)) && (group->param_neuro_pandora[i + j * group->columns * incrementation].selected))
      {
        emit_signal_stop_to_promethe((group->firstNeuron + i + j * group->columns * incrementation), group->script);
      }
      // emit_signal_stop_to_promethe(group->firstNeuron+i + j * group->columns * incrementation,group->script);

      //emit_signal_stop_to_promethe(group->firstNeuron+i + j * group->columns * incrementation,group->script); //déduit du premier ele du groupe et de i et j le num de la neuronne veritable dans le grand tab des neuro de promethe.
      group->param_neuro_pandora[i + j * group->columns * incrementation].selected = FALSE;

      if (i + j * group->columns * incrementation == group->number_of_neurons - 1) // on est allé jusqu'au bout du tableau de neurone pour la remise à zero
      {

        group->x_event = -1;
        group->y_event = -1;
        if (group->param_neuro_pandora[group->neuro_select].selected == FALSE)
        {
          group->neuro_select = -1; // on remet le groupe à -1 uniquement si à la fin le chiffre ne correspond plus à rien.
        }

      }
    }
  }

}
