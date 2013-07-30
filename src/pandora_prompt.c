/**
 * pandora_prompt.c
 *
 *  Created on: 19 juil. 2013
 *      Author: Nils Beaussé
 **/

#include "pandora_prompt.h"
#include <glib/gprintf.h>

GtkTextBuffer * p_buf;
prompt_lign prompt_buf[NB_LIGN_MAX];

extern pthread_cond_t cond_copy_arg_top;

gboolean send_info_to_top(gpointer *user_data)
{
  long int time = 0;
  prompt_lign lign;
  type_group group;
  group = *(type_group*) user_data;

  //la copie est réalisé on envoie le signal de libération d'enet

  time = (long int) ((double) (group.stats.somme_tot) / (double) (group.stats.nb_executions_tot));

  strcpy(&(lign.text_lign[0]), ""); //sécurité
  strncat(&(lign.text_lign[0]), &(group.name[0]), GROUP_NAME_MAX - 1);
  lign.time_to_prompt = time;
  lign.prompt = TRUE;
  lign.group_id = group.id;

  insert_lign(lign, (prompt_lign*) prompt_buf); //TODO remonter le concept objet jusqu'au main (promp_buf ici)
  pthread_cond_signal(&cond_copy_arg_top);
  prompt(prompt_buf, p_buf);

  return FALSE;
}

void init_top(prompt_lign *buf, GtkTextBuffer* text_buf)
{
  int i = 0;
  //printf("on passe dans l'init");
  for (i = 0; i < NB_LIGN_MAX; i++)
  {
    init_lign(&(buf[i]));
  }
  gtk_text_buffer_set_text(text_buf, "", -1);

}

void insert_lign(prompt_lign lign, prompt_lign *prom_buf)
{
  int i = 0;

  for (i = 0; i < NB_LIGN_MAX; i++)
  {
    if (lign.group_id == prom_buf[i].group_id)
    {
      delete_lign(prom_buf, i);
      break;
    }
  }

  if (lign.prompt)
  {
    for (i = NB_LIGN_MAX - 1; i >= 0; i--)
    {
      if (prom_buf[i].prompt) // La ligne concurente est-elle à afficher ?
      { //Oui
        if (lign.time_to_prompt > prom_buf[i].time_to_prompt) //doit-on aller plus haut?
        { //oui
          if (i != 0) // est-il possible de remonter?
          {
            //dans ce cas on remonte, donc on ne fais rien
          }
          else
          {
            // on est tout en haut avec la premiere ligne qui est à déplacer
            decal_tab(prom_buf, 0); // Alors on décale tout depuis 0;
            prom_buf[0] = lign; //et on insere notre ligne
            break;
          }
        }
        else
        { //non on ne doit pas aller plus haut, notre ligne est égale ou inférieure

          if (i != NB_LIGN_MAX - 1) //si on est pas tout en bas
          {
            if (prom_buf[i + 1].prompt) //si la ligne suivante était à afficher
            {
              decal_tab(prom_buf, i + 1); // on décale tout depuis la ligne d'après
              prom_buf[i + 1] = lign; // on insere notre ligne en dessous
              break;
            }
            else //sinon on suppose qu'aucune ligne du dessous n'était à afficher donc inutile de tout décaler
            {
              prom_buf[i + 1] = lign; // on insere notre ligne en dessous
              break;
            }
          }
          else //si on est tout en bas... la ligne ne peut pas etre inséré
          {
            break;
          }
        }
      }
      else
      { //Non la ligne concurente n'est pas à afficher, inutile de tout verifier, on doit remonter

        if (i != 0) // est-il possible de remonter?
        {
          //dans ce cas on remonte, donc on ne fais rien
        }
        else
        {
          // on est tout en haut avec la premiere ligne qui est vide
          prom_buf[0] = lign; //On insere notre ligne
          break;
        }
      }
    }
  }
  else
  {
    printf("Mauvaise utilisation de insert_lign !");
  }

}

void decal_tab(prompt_lign *prom_buf, int i)
{
  int k;
  // prompt_lign prom_lign_temp;

  for (k = NB_LIGN_MAX - 1; k > i; k--)
  {
    if (prom_buf[k - 1].prompt) prom_buf[k] = prom_buf[k - 1]; // on décale le tableau depuis l'indice i jusqu'a la fin, et on vire le dernier element
  }

}

void prompt(prompt_lign *prom_buf, GtkTextBuffer *text_buf)
{
  gint len = 0;
  gchar* lign[NB_LIGN_MAX];
  gchar* text;
  gchar* UTF8text;
  int i = 0;

  for (i = 0; i < NB_LIGN_MAX; i++)
  {
    lign[i] = MANY_ALLOCATIONS(GROUP_NAME_MAX+10,gchar);
  }

  for (i = 0; i < NB_LIGN_MAX; i++)
  {
    if (prom_buf[i].prompt) g_sprintf(lign[i], "%s\t\t\t\t%5d\n", prom_buf[i].text_lign, (int) prom_buf[i].time_to_prompt);
    else g_sprintf(lign[i], " ");
  }

  text = MANY_ALLOCATIONS((GROUP_NAME_MAX+10)*(NB_LIGN_MAX+2),gchar);
  g_stpcpy(text, "TOP\nBox name\t\tExec time(µs)\n");
  len = sizeof(gchar) * (GROUP_NAME_MAX + 10) * NB_LIGN_MAX;

  for (i = 0; i < NB_LIGN_MAX; i++)
  {
    g_strlcat(text, lign[i], len);
  }

  UTF8text = g_locale_to_utf8(text, -1, NULL, NULL, NULL);
  gtk_text_buffer_set_text(text_buf, UTF8text, -1);

  g_free(UTF8text);
  free(text);
  for (i = 0; i < NB_LIGN_MAX; i++)
  {
    free(lign[i]);
  }

}

void delete_lign(prompt_lign *prom_buf, int i)
{
  prompt_lign blank_lign;
  int k;

  //creation ligne vierge
  init_lign(&blank_lign);

  for (k = i; k < NB_LIGN_MAX - 1; k++)
  {
    if (prom_buf[k].prompt)
    {
      prom_buf[k] = prom_buf[k + 1]; // on décale le tableau
    }
    else
    {
      break;
    }
  }
  prom_buf[NB_LIGN_MAX - 1] = blank_lign;

}

void init_lign(prompt_lign *lign)
{
  lign->prompt = FALSE;
  strcpy(lign->text_lign, "");
  lign->time_to_prompt = 0;
  lign->group_id = 0;
}
