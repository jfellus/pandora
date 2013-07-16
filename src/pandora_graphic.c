/**
 * pandora_graphic.c
 *
 *  Created on: ???
 *      Author: ???
 *
 */


#include "pandora_graphic.h"
#include "common.h"

#define DIGITS_MAX 32
#define NB_DIGITS 7
#define TEXT_LINE_HEIGHT 12
#define TEXT_MAX 32
#define TEXT_OFFSET 4

/* Variables Globales pour ce fichier*/
//extern GtkWidget *check_button_draw_connections, *check_button_draw_net_connections;
extern gboolean saving_press;
type_graphic graphic;

/* TODO optimiser */
float niveauDeGris(float val, float valMin, float valMax)
{
  float ndg = (val - valMin) / (valMax - valMin);
  if (ndg < 0) ndg = 0;
  else  if (ndg > 1) ndg = 1;
  return ndg;
}

float CoordonneeYPoint(float val, float valMin, float valMax, float hauteurNeurone)
{
    if(val < valMin)
        return hauteurNeurone;
    if(val > valMax)
        return 0;

    return (valMax - val)/(valMax - valMin) * hauteurNeurone;
}

float coordonneeYZero(float valMin, float valMax, float hauteurNeurone)
{
	return CoordonneeYPoint(0, valMin, valMax, hauteurNeurone);
}

void graph_get_line_color(int num, float *r, float *g, float *b)
{
    switch(num)
    {
        case 0: *r = 0.0;   *g = 0.0;   *b = 0.0;
        break;
        case 1: *r = 0.0;   *g = 1.0;   *b = 0.0;
        break;
        case 2: *r = 0.0;   *g = 0.0;   *b = 1.0;
        break;
        case 3: *r = 1.0;   *g = 0.0;   *b = 0.0;
        break;
        case 4: *r = 0.5;   *g = 0.5;   *b = 0.5;
        break;
        case 5: *r = 0.0;   *g = 0.85;   *b = 0.85;
        break;
        case 6: *r = 0.85;   *g = 0.85;   *b = 0.0;
        break;
        case 7: *r = 1.0;   *g = 0.0;   *b = 1.0;
        break;

    }
}

/**
 *  Affiche les neurones d'une petite fenêtre
 */
void group_expose_neurons(type_group *group, gboolean lock_gdk_threads, gboolean update_frequence)
{
  cairo_t *cr;

  float ndg = 0, val, tmp = 0, x, y0, yVal, min = group->val_min, max = group->val_max, frequence = 0;
  float largeurNeuron, hauteurNeuron;
//  float r, g, b;
  int i, j, k = 0;
  int incrementation;
  int stride;
  cairo_format_t format;
  cairo_surface_t *image;
  prom_images_struct *prom_images;
  unsigned char *image_data;

//  float time;
  char label_text[LABEL_MAX];

  int sortie = group->output_display;
  float **values = NULL;
  int indexDernier = -1, indexAncien = -1, indexTmp;

  incrementation = group->number_of_neurons / (group->columns * group->rows);

  // ajustement de la taille du widget en fonction du mode d'affichage et de la sortie.
  resize_group(group);
  if(group->previous_display_mode != group->display_mode || group->previous_output_display != group->output_display)
  {
	  group->previous_display_mode = group->display_mode;
	  group->previous_output_display = group->output_display;
  }

  //Début du dessin
  if(lock_gdk_threads == TRUE)
	  gdk_threads_enter();

  //Dimensions d'un neurone
  largeurNeuron = GTK_WIDGET(group->drawing_area)->allocation.width / (float) group->columns;
  hauteurNeuron = GTK_WIDGET(group->drawing_area)->allocation.height / (float) group->rows;

  if(update_frequence == TRUE)
  	  group_update_frequence_values(group);

  for(i=0; i<FREQUENCE_MAX_VALUES_NUMBER; i++)
	if(group->frequence_values[i] > -1)
	  {k++; frequence += group->frequence_values[i];}
  if(k > 0)
	frequence = frequence / k;


  group->counter = 0;


  cr = gdk_cairo_create(GTK_WIDGET(group->drawing_area)->window); //Crée un contexte Cairo associé à la drawing_area "zone" (?????)

  // si la sortie vaut 3, on affiche l'image ou un indicateur si aucune image a été reçue.
  if (group->output_display == 3)
  {
	snprintf(label_text, LABEL_MAX, "<b>%s</b> - %s \n%.3f Hz", group->name, group->function, frequence);
    gtk_label_set_markup(group->label, label_text);
    if (group->ext != NULL && ((prom_images_struct*) group->ext)->image_number > 0)
    {
      prom_images = (prom_images_struct*) group->ext;
      gtk_widget_set_size_request(group->widget, (float) (((int) prom_images->sx) + GTK_WIDGET(group->widget)->allocation.width - GTK_WIDGET(group->drawing_area)->allocation.width), (float) (((int) prom_images->sy) + GTK_WIDGET(group->widget)->allocation.height - GTK_WIDGET(group->drawing_area)->allocation.height));

      if(group->image_selected_index >= (int) prom_images->image_number)
    	  group->image_selected_index = 0;
      k = group->image_selected_index;
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
        break;
      }
      stride = cairo_format_stride_for_width(format, (int) prom_images->sx);
      if(prom_images->nb_band == 3)
      {
		  image_data = malloc(stride * (int) prom_images->sy);
		  for(i=0; (unsigned int) i<prom_images->sx * prom_images->sy; i++)
		  {
			  image_data[i*4] = prom_images->images_table[k][i*3+2];
			  image_data[i*4+1] = prom_images->images_table[k][i*3+1];
			  image_data[i*4+2] = prom_images->images_table[k][i*3];
			  image_data[i*4+3] = prom_images->images_table[k][i*3+2];
		  }
		  image = cairo_image_surface_create_for_data(image_data, format, (int) prom_images->sx, (int) prom_images->sy, stride);
	      cairo_set_source_surface(cr, image, 0, 0);
	      cairo_paint(cr);
	      cairo_surface_finish(image);
	      cairo_surface_destroy(image);
	      free(image_data);
      }
      else if(prom_images->nb_band == 4)
	  {
    	  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    	  cairo_paint(cr);
		  image_data = malloc(stride * (int) prom_images->sy);
		  if(image_data != NULL)
		  {
			  for(i=0; (unsigned int) i<prom_images->sx * prom_images->sy; i++)
			  {
				  image_data[i] = (unsigned char) ((float *)prom_images->images_table[k])[i] * 255;
			  }
			  image = cairo_image_surface_create_for_data(image_data, format, (int) prom_images->sx, (int) prom_images->sy, stride);
			  cairo_set_source_surface(cr, image, 0, 0);
			  cairo_paint(cr);
			  cairo_surface_finish(image);
			  cairo_surface_destroy(image);
			  free(image_data);
		  }
	  }
      else if(format == CAIRO_FORMAT_A8)
      {
    	  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    	  cairo_paint(cr);

		  image_data = malloc(stride * (int) prom_images->sy);
		  if(image_data != NULL)
		  {
			  for(i=0; (unsigned int) i<prom_images->sx * prom_images->sy; i++)
			  {
				  image_data[i] = 255 - prom_images->images_table[k][i];
			  }
			  image = cairo_image_surface_create_for_data(image_data, format, (int) prom_images->sx, (int) prom_images->sy, stride);
			  cairo_set_source_surface(cr, image, 0, 0);
			  cairo_paint(cr);
			  cairo_surface_destroy(image);
			  free(image_data);
		  }
      }
    }
    else
    {
    	gtk_widget_set_size_request(group->widget, 140, 80);
  	    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
  	    cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, GTK_WIDGET(group->drawing_area)->allocation.width / 2 - 45, GTK_WIDGET(group->drawing_area)->allocation.height / 2);
        cairo_show_text(cr, "No image received");
    }
  }
  else
  {
	// si le mode auto est activé, calcul du minimum et maximum instantané
    if (group->normalized && group->display_mode != DISPLAY_MODE_BIG_GRAPH && group->display_mode != DISPLAY_MODE_GRAPH)
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
          min = val;
          max = val;
        }
        else
        {
          if (val < min) min = val;
          if (val > max) max = val;
        }
      }
    }

    // fond blanc si le mode est big graph
    if(group->display_mode == DISPLAY_MODE_BIG_GRAPH)
        cairo_set_source_rgb(cr, 1, 1, 1);
    else
        cairo_set_source_rgb(cr, 0, 0, 0);

    cairo_paint(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);

    /// variables utilisées pour l'affichage des graphe.
    sortie = group->output_display;

    update_graph_data(group);
    // si mode graph et mode auto activé, calcul du minimum et maximum moyen (basé sur les NB_Max_VALEURS_ENREGISTREES dernières itérations).
    if(group->normalized && group->display_mode == DISPLAY_MODE_GRAPH)
    {
      min = 0.0;
      max = -1.0;
      for (j = 0; j < group->rows; j++)
      {
        for (i = 0; i < group->columns * incrementation; i += incrementation)
        {
          values = group->tabValues[j][i/incrementation];
          if(group->indexDernier[j][i/incrementation] != -1)
            for(k=1; k<NB_Max_VALEURS_ENREGISTREES; k++)
            {
                tmp = values[sortie][k];
                if(max < min)   min = max = tmp;
                else if(tmp < min)   min = tmp;
                else if(max < tmp) max = tmp;
            }
        }
      }
      // si le minimum et le maximum sont incorrects, ils sont réinitialisés.
      if(max < min)
        {min = -0.5; max = 0.5;}
    }

    // construction du label du groupe sous la forme :   nom_groupe (en gras) - nom_fonction
    //                                                   [min | max] - fréquence moyenne
    snprintf(label_text, LABEL_MAX, "<b>%s</b> - %s \n[%.2f | %.2f] - %.3f Hz", group->name, group->function, min, max, frequence);
    gtk_label_set_markup(group->label, label_text);

	update_big_graph_data(group);
	if(group->display_mode == DISPLAY_MODE_BIG_GRAPH)
		draw_big_graph(group, cr, frequence);
	else
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

        ndg = niveauDeGris(val, min, max);
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
          // couleur rouge si la valeur est négative
          if(val < 0)
            cairo_set_source_rgb(cr, 1, 0.2, 0.25);
          y0 = coordonneeYZero(min, max, hauteurNeuron);
          yVal = CoordonneeYPoint(val, min, max, hauteurNeuron);
          if(yVal < y0)
          {
              tmp = yVal;
              yVal = y0;
              y0 = tmp;
          }
          yVal = yVal - y0;// hauteur du rectangle à afficher.
          y0 += j * hauteurNeuron + 1;
          cairo_rectangle(cr, i * largeurNeuron, y0, largeurNeuron - 1, yVal);
          cairo_fill(cr);
          if(val < 0)
            cairo_set_source_rgb(cr, 1, 1, 1);
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

        case DISPLAY_MODE_GRAPH :
        	values = group->tabValues[j][i/incrementation];
			indexDernier = group->indexDernier[j][i/incrementation];
			indexAncien = group->indexAncien[j][i/incrementation];

            indexTmp = indexDernier;
            tmp = 0;
            // permet de tracer le graphe dans la zone réservée au neurone (le graphe n'est pas relié).
            for(x = (i/incrementation + 1) * largeurNeuron - 2; x > i*largeurNeuron+3; x--)
            {
                // changement de la couleur du tracé si tmp devient positif ou négatif.
                if(tmp < 0 && !(values[sortie][indexTmp] < 0))
                {
                    cairo_fill(cr);
                    cairo_set_source_rgb(cr, 1, 1, 1);
                }
                else if(!(tmp < 0) && values[sortie][indexTmp] < 0)
                {
                    cairo_fill(cr);
                    cairo_set_source_rgb(cr, 1, 0.2, 0.25);
                }

                tmp = values[sortie][indexTmp];
                yVal = CoordonneeYPoint(tmp, min, max, hauteurNeuron);
                cairo_rectangle(cr, x, yVal + j * hauteurNeuron + 1, 1, 2);
                if(indexAncien == indexTmp)
                    break;
                indexTmp--;
                if(indexTmp < 0)
                    indexTmp = NB_Max_VALEURS_ENREGISTREES-1;
            }
            if(tmp < 0)
            {
                cairo_fill(cr);
                cairo_set_source_rgb(cr, 1, 1, 1);
            }
            /// permet de tracer les traits verticaux (séparation entre deux graphes situés sur la même ligne dans le groupe).
            if(i < (group->columns - 1) * incrementation)
            {
                cairo_rectangle(cr, (i/incrementation + 1) * largeurNeuron, j*hauteurNeuron, 0.5, hauteurNeuron);
            }
            /// permet de tracer la droite y=0.
            if(!(min > 0 || 0 > max))
            {
                cairo_rectangle(cr, (i/incrementation) * largeurNeuron, j*hauteurNeuron + coordonneeYZero(min, max, hauteurNeuron) + 0.5, largeurNeuron, 0.5);
            }
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

  if(lock_gdk_threads == TRUE)
  	  gdk_threads_leave();
}

void group_update_frequence_values(type_group *group)
{
	float time = g_timer_elapsed(group->timer, NULL);
	g_timer_start(group->timer);


	// calcul de la fréquence moyenne basée sur les FREQUENCE_MAX_VALUES_NUMBER dernières itérations.
	if(group->frequence_index_last == -1)
	{
		group->frequence_index_last = group->frequence_index_older = 0;
		group->frequence_values[0] = group->counter / time;
	}
	else if(group->frequence_index_older == group->frequence_index_last+1 || (group->frequence_index_last == FREQUENCE_MAX_VALUES_NUMBER-1 && group->frequence_index_older == 0))
	{
		if(group->frequence_index_last == FREQUENCE_MAX_VALUES_NUMBER-1)
		{
			group->frequence_index_last = 0;
			group->frequence_index_older = 1;
		}
		else if(group->frequence_index_older == FREQUENCE_MAX_VALUES_NUMBER-1)
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
	float hauteur = GTK_WIDGET(group->drawing_area)->allocation.height;
	float largeur = GTK_WIDGET(group->drawing_area)->allocation.width;
	float min = group->val_min, max = group->val_max, tmp, r, g, b, x, yVal;
	float **values;
	char label_text[LABEL_MAX];

	if(group->normalized)
	{
		min = 0.0;
		max = -1.0;
		for (i = 0; i < group->number_of_courbes; i++)
		{
			values = group->courbes[i].values;
			if(group->courbes[i].last_index != -1)
				for(k=0; k<NB_Max_VALEURS_ENREGISTREES; k++)
				{
					tmp = values[sortie][k];
					if(max < min)   min = max = tmp;
					else if(tmp < min)   min = tmp;
					else if(max < tmp) max = tmp;
				}
		}
	}

	if(min > max)
		min = max - 1;

    snprintf(label_text, LABEL_MAX, "<b>%s</b> - %s \n[%.2f | %.2f] - %.3f Hz", group->name, group->function, min, max, frequence);
    gtk_label_set_markup(group->label, label_text);

	for(i=0; i<group->number_of_courbes; i++)
	{
    	// trace la droite d'équation y=0.
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr, 1, coordonneeYZero(min, max, hauteur) + 0.5, largeur-2, 0.5);
        cairo_fill(cr);

        if(group->courbes[i].show == TRUE) // si il faut tracer la courbe pour ce neurone, l'exécution continue.
        {
			// application de la couleur associée à la courbe.
			graph_get_line_color(i, &r, &g, &b);
			cairo_set_source_rgb(cr, r, g, b);

	        indexAncien = group->courbes[i].old_index;
	        indexDernier = group->courbes[i].last_index;
	        values = group->courbes[i].values;

			// trace la courbe (les point sont reliés).
			indexTmp = indexDernier;
			tmp = 0;
			k = 0;
			for(x = largeur - 2.0; !(x < 1) ; x-=2.0, k=1)
			{
				tmp = values[sortie][indexTmp];
				yVal = CoordonneeYPoint(tmp, min, max, hauteur - 4.5);
				if(k == 0)
					cairo_move_to(cr, x, yVal + 1);
				else
					cairo_line_to(cr, x, yVal + 1);
				if(indexAncien == indexTmp)
					break;
				indexTmp--;
				if(indexTmp < 0)
					indexTmp = NB_Max_VALEURS_ENREGISTREES-1;
			}
			cairo_stroke(cr); // relie les points.
        }
	}
}

void update_graph_data(type_group *group)
{
	int indexDernier, indexAncien, i, j, k, incrementation = group->number_of_neurons / (group->columns * group->rows);
	float **values;

	if(stop == TRUE || group->rows > 10 || group->columns > 10)
		return;

    for (j = 0; j < group->rows; j++)
    {
      for (i = 0; i < group->columns * incrementation; i += incrementation)
      {
		values = group->tabValues[j][i/incrementation];
		indexDernier = group->indexDernier[j][i/incrementation];
		indexAncien = group->indexAncien[j][i/incrementation];

		// ajout de la dernière valeur au tableau des valeurs utilisé pour tracer le graphe.

		// si aucune valeur enregistrée, initialisation du tableau.
		if(indexDernier == -1)
		{
			indexDernier = indexAncien = 0;
			for(k = 0; k<NB_Max_VALEURS_ENREGISTREES; k++)
			{
				values[0][k] = group->neurons[i + j * group->columns * incrementation].s;
				values[1][k] = group->neurons[i + j * group->columns * incrementation].s1;
				values[2][k] = group->neurons[i + j * group->columns * incrementation].s2;
			}
		}
		// si le tableau est plein, remplacement de la valeur la plus ancienne par la valeur à ajouter. l'enregistrement des valeurs est similaire à une file circulaire.
		else if(indexAncien == indexDernier+1 || (indexDernier == NB_Max_VALEURS_ENREGISTREES-1 && indexAncien == 0))
		{
			if(indexDernier == NB_Max_VALEURS_ENREGISTREES-1)
			{
				indexDernier = 0;
				indexAncien = 1;
			}
			else if(indexAncien == NB_Max_VALEURS_ENREGISTREES-1)
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
		group->indexDernier[j][i/incrementation] = indexDernier;
		group->indexAncien[j][i/incrementation] = indexAncien;
      }
    }
}

void update_big_graph_data(type_group *group)
{
	int indexDernier, indexAncien, column, line, k, l, incrementation = group->number_of_neurons / (group->columns * group->rows);
	float **values;

	if(stop == TRUE)
		return;

	for(l=0; l < group->number_of_courbes; l++)
	{
		values = group->courbes[l].values;
		column = group->courbes[l].column;
		line = group->courbes[l].line;
		indexAncien = group->courbes[l].old_index;
		indexDernier = group->courbes[l].last_index;

		// si aucune valeur enregistrée, initialisation du tableau.
		if(indexDernier == -1)
		{
			indexDernier = indexAncien = 0;
			for(k = 0; k<NB_Max_VALEURS_ENREGISTREES; k++)
			{
				values[0][k] = group->neurons[column + line * group->columns * incrementation].s;
				values[1][k] = group->neurons[column + line * group->columns * incrementation].s1;
				values[2][k] = group->neurons[column + line * group->columns * incrementation].s2;
			}
		}
		// si le tableau est plein, remplacement de la valeur la plus ancienne par la valeur à ajouter. l'enregistrement des valeurs est similaire à une file circulaire.
		else if(indexAncien == indexDernier+1 || (indexDernier == NB_Max_VALEURS_ENREGISTREES-1 && indexAncien == 0))
		{
			if(indexDernier == NB_Max_VALEURS_ENREGISTREES-1)
			{
				indexDernier = 0;
				indexAncien = 1;
			}
			else if(indexAncien == NB_Max_VALEURS_ENREGISTREES-1)
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


// modifie la zone affichée. x et y représentent le point situé en haut a gauche de la zone à afficher.
void architecture_set_view_point(GtkWidget *scrollbars, double x, double y)
{
    GtkAdjustment *a_x, *a_y;
    a_x = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrollbars));
    a_y = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollbars));
    // modification des valeurs.
    gtk_adjustment_set_value(a_x, x);
    gtk_adjustment_set_value(a_y, y);
    // propagation de l'évènement. permet d'actualiser la zone à afficher.
    gtk_adjustment_value_changed(a_x);
    gtk_adjustment_value_changed(a_y);

    // l'evenement semble capté par un expose event.
}

// Met à jour l'affichage des groupes de neuronnes
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
          if (group->on_saving==1) cairo_set_line_width(cr, 4);
          if (group->on_saving==0) cairo_set_line_width(cr, 2);
          cairo_rectangle(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale, LARGEUR_GROUPE, HAUTEUR_GROUPE);


          /* Test selection */
          if (event != NULL)
          {
            if (cairo_in_fill(cr, event->x, event->y)) // Selection pour sauvegarde (touche control activée)
            	{
            	selected_group = group;
            	  if ((event->state & GDK_CONTROL_MASK) && !saving_press)
            	  {
            		  group->selected_for_save=!group->selected_for_save;

            		  //TODO : demande d'envoie des données à promethé pour ceux qui sont en selection type sauvegarde
            		  if (group->selected_for_save==1) pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_START, group->id, group->script->name);
            		  else pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_SEND_NEURONS_STOP, group->id, group->script->name);
            	  }
            	}

          }
          if (group == selected_group) cairo_set_source_rgb(cr, RED);
          else if(group->is_in_a_loop == TRUE) cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
          else color(cr, group);
          cairo_fill_preserve(cr);
          if(group->selected_for_save==1)
          {

              if (group->on_saving==1) cairo_set_source_rgb(cr, YELLOW);
              if (group->on_saving==0) cairo_set_source_rgb(cr, BLUE);


        	  cairo_stroke_preserve(cr);
          }


          /* Texts  */
          cairo_save(cr);
          if (group != selected_group) cairo_clip(cr);
          gdk_cairo_set_source_color(cr, &couleurs[noir]);
          cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + TEXT_LINE_HEIGHT);
          if(calculate_executions_times == TRUE)
            {
            snprintf(text, TEXT_MAX, "%s   | %sµs", group->name, group->stats.message);
            }
          else
          {
            snprintf(text, TEXT_MAX, "%s", group->name);
          }
          cairo_show_text(cr, text);
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
              else if(group->is_in_a_loop == TRUE && input_group->is_in_a_loop == TRUE)
              {
                cairo_set_source_rgb(cr, BLACK);
                cairo_set_line_width(cr, 4);
              }
              else
              {
            	color(cr, group);
            	cairo_set_line_width(cr, 1);
              }
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
        if ((input_group == selected_group ) || (group == selected_group))
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

void architecture_display_update_group(GtkWidget *architecture_display, type_group *group)
{
  cairo_t *cr;
  char text[TEXT_MAX];
  double x_offset = 0, y_offset = 0;
  type_script *script;

//Dessin des groupes : on dessine d'abord les scripts du plan z = 0 (s'il y en a), puis ceux du plan z = 1, etc., jusqu'à zMax inclus
  script = group->script;
  if(script->displayed)
  {
	  x_offset = graphic.zx_scale * script->z;
	  y_offset = graphic.zy_scale * script->z + script->y_offset * graphic.y_scale;

	  /* Box */
	  cr = gdk_cairo_create(architecture_display->window); //Crée un contexte Cairo associé à la drawing_area "zone"
	  cairo_rectangle(cr, x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale, LARGEUR_GROUPE, HAUTEUR_GROUPE);

	  if (group == selected_group) cairo_set_source_rgb(cr, RED);
	  else if(group->is_in_a_loop == TRUE) cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
	  else color(cr, group);
	  cairo_fill_preserve(cr);

	  /* Texts  */
	  cairo_save(cr);
	  if (group != selected_group) cairo_clip(cr);
	  gdk_cairo_set_source_color(cr, &couleurs[noir]);
	  cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + TEXT_LINE_HEIGHT);
	  if(group->stats.message[0] != '\0')
		  snprintf(text, TEXT_MAX, "%s   | %sµs", group->name, group->stats.message);
	  else
		  snprintf(text, TEXT_MAX, "%s", group->name);
	  cairo_show_text(cr, text);
	  cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + 2 * TEXT_LINE_HEIGHT);
	  cairo_show_text(cr, group->function);
	  snprintf(text, TEXT_MAX, "%d x %d", group->columns, group->rows);
	  cairo_move_to(cr, TEXT_OFFSET + x_offset + group->x * graphic.x_scale, y_offset + group->y * graphic.y_scale + 3 * TEXT_LINE_HEIGHT);
	  cairo_show_text(cr, text);
	  cairo_stroke(cr);
	  cairo_restore(cr); /* unclip */
	  cairo_destroy(cr);
  }
}

// permet de connaitre les coordonnées d'un groupe affiché dans la zone "architecture_display" (point en haut à gauche du groupe).
void architecture_get_group_position(type_group *group, float *x, float *y)
{
  int i, group_id = 0, z;
  double x_offset = 0, y_offset = 0;
  type_script *script;
  type_group *tmp_group;

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
          tmp_group = &script->groups[group_id];

          /* Test selection */
          if (tmp_group == group)
          {
              *x = x_offset + tmp_group->x * graphic.x_scale;
              *y = y_offset + tmp_group->y * graphic.y_scale;
              return;
          }
        }
      }
    }
  }
}
