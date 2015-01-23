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
 * ivy.c
 *
 *  Created on: Aug 23, 2012
 *      Author: arnablan
 **/

/**
 *
 * Permet d'envoyer un message via le port ivy au format printf
 *
 * @param *format: le message à afficher
 **/

#include "pandora_ivy.h"
#include "pandora.h"
#include "prom_bus.h"
#include <unistd.h> /* gethostname */
#include <limits.h> /* HOST_NAME_MAX */

char broadcast[BROADCAST_MAX];
char computer_name[HOST_NAME_MAX];
char ivy_prom_name[SIZE_OF_IVY_PROM_NAME]; /* utiliser un define, peut être le mettre en commun a themis */

/* Variables Globales pour ce fichier */
extern char bus_id[BUS_ID_MAX];
sem_t ivy_semaphore;
typedef struct ivy_serveurs ivy_serveurs;

typedef struct ivy_serveurs {
  ivyServer serveur;
  ivy_serveurs* s;
} ivy_serveurs;

int ivyServerNb = 0;
ivy_serveurs* liste_serveurs = NULL;
ivy_serveurs* last_serveur = NULL;

void new_ivy_serveur(char* const appname, char* const host)
{
  ivy_serveurs* nouveau_serveur = NULL;
  nouveau_serveur = malloc(sizeof(ivy_serveurs));

  strncpy((nouveau_serveur->serveur).appName, appname, 63);
  (nouveau_serveur->serveur).appName[63] = '\0';
  strcpy((nouveau_serveur->serveur).ip, host);
  nouveau_serveur->s = NULL;
  if (strcmp((nouveau_serveur->serveur).ip, "localhost") == 0) strcpy((nouveau_serveur->serveur).ip, "127.0.0.1");

  if (liste_serveurs == NULL)
  {
    // premier element
    liste_serveurs = nouveau_serveur;
    last_serveur = nouveau_serveur;
  }
  else
  {
    last_serveur->s = nouveau_serveur;
    last_serveur = nouveau_serveur;
  }

  ivyServerNb++;
}

void destroy_ivy_serveur(char* const appname)
{
  ivy_serveurs* element_parcouru = liste_serveurs;
  ivy_serveurs* pointeur_apres = NULL;
  ivy_serveurs* a_effacer = NULL;

  if (element_parcouru != NULL)
  {
    if (strcmp((element_parcouru->serveur).appName, appname) == 0)
    {
      liste_serveurs=element_parcouru->s;
      free(element_parcouru);
    }
    else
    {
      while (element_parcouru->s != NULL)
      {
        if (strcmp(((element_parcouru->s)->serveur).appName, appname) == 0)
        {
          pointeur_apres=(element_parcouru->s)->s;
          a_effacer= element_parcouru->s;
          element_parcouru->s=pointeur_apres;
          free(a_effacer);
          break;
        }
        element_parcouru = element_parcouru->s;
      }
    }
  }
}
void pandora_bus_send_message(char *id, const char *format, ...)
{
  char buffer[MAX_SIZE_OF_PROM_BUS_MESSAGE];
  va_list arguments;
  va_start(arguments, format);
  vsprintf(buffer, format, arguments);
  sem_wait(&ivy_semaphore);
  /// Les message sont précédés de l'id pour que les promethes d'autres utilisateurs sur le même réseau ne reagissent pas.
  IvySendMsg("%s:%s", id, buffer);
  sem_post(&ivy_semaphore);
}

void ivyApplicationCallback(IvyClientPtr app, void *user_data, IvyApplicationEvent event)
{
  char *appname = NULL;
  char *host = NULL;
  int i;
  type_script *script;
  //static ivyServer ivyServers[NB_SCRIPTS_MAX]; //Stocke l'ip de chaque promethe qui se connecte et le nom du script qu'il exécute
  //static int ivyServerNb = 0; //Ce numéro sera affecté au prochain promethe qui se connectera

  (void) user_data;

  appname = IvyGetApplicationName(app);
  // host = IvyGetApplicationHost(app);
  host = IvyGetApplicationIp(app);

  switch (event)
  {
  case IvyApplicationConnected:
    if (strncmp(appname, "themis:", 7) == 0) break; //On ne veut pas référencer Thémis comme étant un script

    new_ivy_serveur(appname, host);

    printf("Connexion ivy de %s d'adresse %s\n", (last_serveur->serveur).appName, (last_serveur->serveur).ip);
    pandora_bus_send_message(bus_id, "pandora(%d,%d) %s", PANDORA_START, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compress_button)), appname+strlen(bus_id)+1);
    break;

  case IvyApplicationDisconnected:
    printf("%s disconnected (ivy) from %s\n", appname, host);

    destroy_ivy_serveur(appname);

    for (i = 0; i < number_of_scripts; i++)
    {
      script = scripts[i];
      if (strcmp(script->name, appname) == 0)
      {
        script_destroy(script);
        break;
      }
    }
    break;

  default:
    printf("%s: unkown ivy event %d\n", appname, event);
    break;
  }
}

void prom_bus_init(const char *ip)
{

  pid_t my_pid;

  gethostname(computer_name, HOST_NAME_MAX);
  my_pid = getpid();

  sprintf(broadcast, "%s:2010", ip);
  snprintf(ivy_prom_name, SIZE_OF_IVY_PROM_NAME, "pandora:%s:%ld", computer_name, (long int) my_pid); /* Le nom unique du programme sur le bus ivy sera pandora:<hostanem>:<pid du processus */

  sem_init(&ivy_semaphore, 0, 1); /* semaphore necessaire pour faire des appels ivy thread safes.*/
  IvyInit(ivy_prom_name, "Pandora started", ivyApplicationCallback, NULL, NULL, NULL);
  IvyStart(broadcast);
}
