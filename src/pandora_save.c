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
 *      Author: Nils Beaussé
 **/

#include "pandora.h"
#include "pandora_save.h"
#include "pandora_architecture.h"
#include "locale.h"
#include "pandora_graphic.h"
#include "common.h"

/* variables globales necessaires pour ce fichier */
extern GtkWidget *architecture_display;
extern char path_named[MAX_LENGHT_PATHNAME];





// Ferme tout les fichiers de tout les scripts.
void destroy_saving_ref(type_script *scripts_used[NB_SCRIPTS_MAX], gboolean is_link)
{
  int N = 0;
  int i, j;
  FILE* associated_file=NULL;
  FILE** associated_file_point=NULL;
  gboolean* pbool_on_saving;

  for (i = 0; i < number_of_scripts; i++)
  {
    N = scripts_used[i]->number_of_groups;
    for (j = 0; j < N; j++)
    {

      if (is_link)
        {
          associated_file=scripts_used[i]->groups[j].associated_file_link;
          associated_file_point=&(scripts_used[i]->groups[j].associated_file_link);
          pbool_on_saving=&(scripts_used[i]->groups[j].on_saving_link);
        }
      else
        {
          associated_file=scripts_used[i]->groups[j].associated_file;
          associated_file_point=&(scripts_used[i]->groups[j].associated_file);
          pbool_on_saving=&(scripts_used[i]->groups[j].on_saving);
        }

      if (associated_file != NULL )
      {
        fclose(associated_file);
        *associated_file_point = NULL;
       // *pbool_on_saving = 0; //par sécurité mais normalement c'est fait ailleurs.
      }
      *pbool_on_saving = 0; //par sécurité mais normalement c'est fait ailleurs.
    }
  }
}

//TODO : ces deux fonctions peuvent etre mergé en une avec les arguments adéquats.

// Ferme les fichiers associé à un script lors de sa cloture.
/*
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
      script_used->groups[indGroup].on_saving = 0; //par sécurité mais normalement c'est fait ailleurs.
    }

  }

}
*/
// Créé le fichier de sauvegarde. 1 dans le second argument si on sauvegarde des liens, 0 sinon.
void file_create(type_group *used_group, int is_for_link)
{
  char file_path_ori[MAX_LENGHT_PATHNAME] = "";
  char file_path[MAX_LENGHT_PATHNAME] = "";
  char num[4] = "";
  FILE* test = NULL;
  int i = 0;
  GtkTreeIter iter;
  gchar* pPath = NULL;
  GtkListStore* list = NULL;
  FILE* File_pt=NULL;

  if(is_for_link)
  {
    list = currently_saving_list_link;
  }
  else
  {
    list = currently_saving_list;
  }

  strcpy(file_path, path_named);

  //Creation du nom de fichier
  strcat(file_path, used_group->name);
  strcat(file_path, "_");
  strcat(file_path, used_group->script->name);

  if(is_for_link)
  {
    strcat(file_path, "_");
    strcat(file_path, "Links");
  }
  //on test si un fichier a cet emplacement existe déja
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

  File_pt=fopen(file_path, "a+");


  if(is_for_link)
    used_group->associated_file_link=File_pt;
  else
    used_group->associated_file = File_pt; /* Pourquoi a+ si fichier nouveau */



  pPath = file_path;

  gtk_list_store_append(list, &iter);
  gtk_list_store_set(list, &iter, 0, pPath, -1);

  if (File_pt != NULL )
  {
    if(is_for_link)
    {
      used_group->on_saving_link=1;
    }
    else
    {
      used_group->on_saving = 1;
    }

    if (format_mode == 0) fprintf(File_pt, "%d\n%d\n", used_group->rows, used_group->columns);

    gtk_widget_queue_draw(architecture_display);
    //architecture_display_update(architecture_display,NULL); // pour remettre �� jour l'affichage quand les groupes sont on saving

  }
  else
  {
    /* On affiche un message d'erreur*/
    if(is_for_link)
        {
      used_group->on_saving_link = 0;
        }
    else
    {
      used_group->on_saving = 0;
    }

    PRINT_WARNING("Impossible to open save file:  '%s'\n", file_path);
  }
}

// Sauvegarde le groupe en cours : TODO : pour l'instant la valeur s1 uniquement est sauvegarder, il reste à faire une fenetre de configuration de la sauvegarde pour savoir quelle valeur de neuronne utiliser.
void continuous_saving(type_group *group)
{

  double time;
  int i, j, incrementation;
  struct timeval time_stamp;
  float to_save;

  incrementation = group->number_of_neurons / (group->columns * group->rows); /* Microneurones */
  setlocale(LC_NUMERIC, "C");


  if(group->on_saving == TRUE)
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
          switch(group->thing_to_save)
          {
          case SAVE_S:
            to_save=group->neurons[i * group->columns * incrementation + j].s;
            break;
          case SAVE_S1:
            to_save=group->neurons[i * group->columns * incrementation + j].s1;
            break;
          case SAVE_S2:
            to_save=group->neurons[i * group->columns * incrementation + j].s2;
            break;
          case SAVE_D:
            to_save=group->neurons[i * group->columns * incrementation + j].d;
            break;
          default:
            PRINT_WARNING("Attention, dans continuous_saving, pas de prise en charge de l'option de sauvegarde en cours \n");
            to_save=-999999999;
            break;
          }


          fprintf(group->associated_file, " %f", to_save);
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
          switch(group->thing_to_save)
                    {
                    case SAVE_S:
                      to_save=group->neurons[i * group->columns * incrementation + j].s;
                      break;
                    case SAVE_S1:
                      to_save=group->neurons[i * group->columns * incrementation + j].s1;
                      break;
                    case SAVE_S2:
                      to_save=group->neurons[i * group->columns * incrementation + j].s2;
                      break;
                    case SAVE_D:
                      to_save=group->neurons[i * group->columns * incrementation + j].d;
                      break;
                    default:
                      PRINT_WARNING("Attention, dans continuous_saving, pas de prise en charge de l'option de sauvegarde en cours \n");
                      to_save=-999999999;
                      break;
                    }

          fprintf(group->associated_file, "%f ", to_save);
        }
        fprintf(group->associated_file, "\n");
      }
      fprintf(group->associated_file, "\n");
    }
  }
  else  // Si le groupe n'est pas déja en train de se sauvegarder alors on doit creer le nom de fichier
   {
     file_create(group,0);
   }

}

// Sauvegarde les liens du neurone en cours.
void continuous_saving_link(type_group *group)
{

  /*
  if (group->on_saving_link == 0)
  {
  group->on_saving_link = 1; // pour test
  gtk_widget_queue_draw(architecture_display);
  }
  */

  double time;
  int i, j,k, incrementation;
  struct timeval time_stamp;

  incrementation = group->number_of_neurons / (group->columns * group->rows); // Microneurones
  setlocale(LC_NUMERIC, "C");


  if(group->on_saving_link == TRUE)
  {
    gettimeofday(&time_stamp, NULL );
    time = time_stamp.tv_sec + (double)time_stamp.tv_usec / 1000000. - start_time;
    // if vector


    if (format_mode == 1)
    {
      PRINT_WARNING("FORMAT PROMETHEE NON EXISTANT POUR LA SAUVEGARDE DE LIENS !!! VEUILLEZ CHANGER DE FORMAT \n ");
      /*
      fprintf(group->associated_file, "%lf", time);
      for (i = 0; i < group->rows; i++)
        for (j = 0; j < group->columns * incrementation; j += incrementation)
        {
          fprintf(group->associated_file, " %f", group->neurons[i * group->columns * incrementation + j].s1);
        }
      fprintf(group->associated_file, "\n");
      */

    }
    else if(format_mode == 0)// Sauvegarde 2D
    {
      // Pour python
      fprintf(group->associated_file_link, "#Time: %lf\n", time);
      for (i = 0; i < group->rows; i++)
      {
        for (j = 0; j < group->columns * incrementation; j += incrementation)
        {
//          fprintf(group->associated_file_link, "%d %d\n",i,j);
          //printf("i=%d, j=%d, incre=%d, no_neurone = %d, nbre_links = %d\n",i,j,incrementation,i * group->columns * incrementation + j,group->param_neuro_pandora[i * group->columns * incrementation + j].nbre_links);
          for(k = 0; k < group->param_neuro_pandora[i * group->columns * incrementation + j].nbre_links; k++)
          {
       //     printf("k=%d, val = %f\n",k,group->param_neuro_pandora[i * group->columns * incrementation + j].coeff[k].val);
            fprintf(group->associated_file_link, "%f/", group->param_neuro_pandora[i * group->columns * incrementation + j].coeff[k].val);
          }
          fprintf(group->associated_file_link, " ");
       //   fprintf(group->associated_file, "%f ", group->neurons[i * group->columns * incrementation + j].s1);
        }
        fprintf(group->associated_file_link, "\n");
      }
      fprintf(group->associated_file_link, "\n");
    }
  }
  else  // Si le groupe n'est pas déja en train de se sauvegarder alors on doit creer le nom de fichier
  {
    file_create(group, 1);
  }

}
