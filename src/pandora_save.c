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
 * pandora_save.h
 *
 *  Created on: 2 juil. 2013
 *      Author: Nils Beauss��
 **/

#include "pandora_save.h"
#include "locale.h"
#include "pandora_graphic.h"
#include "common.h"

/* variables globales necessaires pour ce fichier */
extern GtkWidget *architecture_display;
extern char path_named[MAX_LENGHT_PATHNAME];
extern GtkListStore* currently_saving_list;

// Ferme tout les fichiers de tout les scripts.
void destroy_saving_ref(type_script *scripts_used[NB_SCRIPTS_MAX])
{
  int N = 0;
  int indScript, indGroup;

  for (indScript = 0; indScript < number_of_scripts; indScript++)
  {
    N = scripts_used[indScript]->number_of_groups;
    for (indGroup = 0; indGroup < N; indGroup++)
    {
      if (scripts_used[indScript]->groups[indGroup].associated_file != NULL )
      {
        fclose(scripts_used[indScript]->groups[indGroup].associated_file);
        scripts_used[indScript]->groups[indGroup].associated_file = NULL;
        scripts_used[indScript]->groups[indGroup].on_saving = 0; //par s��curit�� mais normalement c'est fait ailleurs.
      }
    }
  }
}

//TODO : ces deux fonctions peuvent etre merg�� en une avec les arguments ad��quats.

// Ferme les fichiers associ�� �� un script lors de sa cloture.
void destroy_saving_ref_one(type_script* script_used)
{
  int N = 0;
  int indGroup;

  N = script_used->number_of_groups;
  for (indGroup = 0; indGroup < N; indGroup++)
  {
    if (script_used->groups[indGroup].on_saving == 1)
    {
      fclose(script_used->groups[indGroup].associated_file);
      printf("\n\nCloture du fichier associé %s\n\n", script_used->groups[indGroup].name);
      script_used->groups[indGroup].on_saving = 0; //par s��curit�� mais normalement c'est fait ailleurs.
    }

  }

}

// Cr���� le fichier de sauvegarde
void file_create(type_group *used_group)
{
  char file_path_ori[MAX_LENGHT_PATHNAME] = "";
  char file_path[MAX_LENGHT_PATHNAME] = "";
  char num[4] = "";
  FILE* test = NULL;
  int i = 0;
  GtkTreeIter iter;
  gchar* pPath = NULL;

  strcpy(file_path, path_named);

  //Creation du nom de fichier
  strcat(file_path, used_group->name);
  strcat(file_path, "_");
  strcat(file_path, used_group->script->name);

  //on test si un fichier a cet emplacement existe d��ja
  strcpy(file_path_ori, file_path);
  do // fonctionne mais peut bugger si il fait trop de fois la boucle
  {
    i++;
    test = fopen(file_path, "r");

    if (test != NULL )
    {
      strcpy(file_path, file_path_ori);
      fclose(test);
      strcat(file_path, "_");
      sprintf(num, "%d", i);
      strcat(file_path, num);
    }

  } while (test != NULL );

  //strcat(file_path, ".csv");

  used_group->associated_file = fopen(file_path, "a+"); /* Pourquoi a+ si fichier nouveau */

  pPath = file_path;
  gtk_list_store_append(currently_saving_list, &iter);
  gtk_list_store_set(currently_saving_list, &iter, 0, pPath, -1);

  if (used_group->associated_file != NULL )
  {
    used_group->on_saving = 1;
    if (format_mode == 0) fprintf(used_group->associated_file, "%d\n%d\n", used_group->rows, used_group->columns);

    gtk_widget_queue_draw(architecture_display);
    //architecture_display_update(architecture_display,NULL); // pour remettre �� jour l'affichage quand les groupes sont on saving

  }
  else
  {
    /* On affiche un message d'erreur*/
    used_group->on_saving = 0;
    printf("Impossible to open save file:  '%s'\n", file_path);
  }
}

// Sauvegarde le groupe en cours : TODO : pour l'instant la valeur s1 uniquement est sauvegarder, il reste �� faire une fenetre de configuration de la sauvegarde pour savoir quelle valeur de neuronne utiliser.
void continuous_saving(type_group *group)
{

  double time;
  int i, j, incrementation;
  struct timeval time_stamp;

  incrementation = group->number_of_neurons / (group->columns * group->rows); /* Microneurones */
  setlocale(LC_NUMERIC, "C");

  if (group->on_saving == 0) // Si le groupe n'est pas d��ja en train de se sauvegarder alors on doit creer le nom de fichier
  {
    file_create(group);
  }

  if (group->on_saving == 1)
  {
    gettimeofday(&time_stamp, NULL );
    time = time_stamp.tv_sec + (double)time_stamp.tv_usec / 1000000. - start_time;
    /* if vector */

    if (format_mode == 1)
    {
      fprintf(group->associated_file, "%lf", time);
      for (i = 0; i < group->rows; i++)
        for (j = 0; j < group->columns * incrementation; j += incrementation)
        {
          fprintf(group->associated_file, " %f", group->neurons[i * group->columns * incrementation + j].s1);
        }
      fprintf(group->associated_file, "\n");

    }
    else if(format_mode == 0)/* Sauvegarde 2D */
    {
      /* Pour python */
      fprintf(group->associated_file, "#Time: %lf\n", time);
      for (i = 0; i < group->rows; i++)
      {
        for (j = 0; j < group->columns * incrementation; j += incrementation)
        {
          fprintf(group->associated_file, "%f ", group->neurons[i * group->columns * incrementation + j].s1);
        }
        fprintf(group->associated_file, "\n");
      }
      fprintf(group->associated_file, "\n");
    }
  }

}

