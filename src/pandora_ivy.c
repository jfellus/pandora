/*
 * ivy.c
 *
 *  Created on: Aug 23, 2012
 *      Author: arnablan
 */

/**
 *
 * Permet d'envoyer un message via le port ivy au format printf
 *
 * @param *format: le message à afficher
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strcmp ... */
#include <stdarg.h> /* va_arg va_list*/
#include <semaphore.h> /* sem_t */
#include <limits.h> /* HOST_NAME_MAX */
#include <unistd.h> /* gethostname */
#include <ivy.h>

#include "pandora.h"
#include "pandora_ivy.h"

#include "prom_bus.h"
#include "prom_kernel/include/pandora_connect.h" /* NB_SCRIPTS_MAX */

#define BROADCAST_MAX 32
#define SIZE_OF_IVY_PROM_NAME 64
#define MAX_SIZE_OF_PROM_BUS_MESSAGE 256


typedef struct ivyServer {
  char ip[18];
  char appName[30];
} ivyServer;

sem_t ivy_semaphore;
char ivy_prom_name[SIZE_OF_IVY_PROM_NAME]; /* utiliser un define, peut être le mettre en commun a themis */
ivyServer ivyServers[NB_SCRIPTS_MAX]; //Stocke l'ip de chaque promethe qui se connecte et le nom du script qu'il exécute
int ivyServerNb = 0; //Ce numéro sera affecté au prochain promethe qui se connectera

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
  char *appname;
  char *host;
  int i;
  type_script *script;

  (void) user_data;

  appname = IvyGetApplicationName(app);
  host = IvyGetApplicationHost(app);

  switch (event)
  {
  case IvyApplicationConnected:
    if (strncmp(appname, "themis:", 7) == 0) break; //On ne veut pas référencer Thémis comme étant un script
    strcpy(ivyServers[ivyServerNb].appName, appname);
    strcpy(ivyServers[ivyServerNb].ip, host);
    if (strcmp(ivyServers[ivyServerNb].ip, "localhost") == 0) strcpy(ivyServers[ivyServerNb].ip, "127.0.0.1");
    printf("Connexion ivy de %s d'adresse %s\n", ivyServers[ivyServerNb].appName, ivyServers[ivyServerNb].ip);
    ivyServerNb++;
    pandora_bus_send_message(bus_id, "pandora(%d,0) %s", PANDORA_START, appname);
    break;

  case IvyApplicationDisconnected:
    printf("%s disconnected (ivy) from %s\n", appname, host);
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
  char broadcast[BROADCAST_MAX];
  char computer_name[HOST_NAME_MAX];
  pid_t my_pid;

  gethostname(computer_name, HOST_NAME_MAX);
  my_pid = getpid();

  sprintf(broadcast, "%s:2010", ip);
  snprintf(ivy_prom_name, SIZE_OF_IVY_PROM_NAME, "pandora:%s:%ld", computer_name, (long int) my_pid); /* Le nom unique du programme sur le bus ivy sera pandora:<hostanem>:<pid du processus */

  sem_init(&ivy_semaphore, 0, 1); /* semaphore necessaire pour faire des appels ivy thread safes.*/
  IvyInit(ivy_prom_name, "Pandora started", ivyApplicationCallback, NULL, NULL, NULL);
  IvyStart(broadcast);
}
