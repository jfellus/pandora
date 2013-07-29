/**
 * pandora_receive_from_prom.h
 *
 *  Created on: 15 juil. 2013
 *      Author: Nils Beaussé
 **/

#ifndef PANDORA_RECEIVE_FROM_PROM_H_
#define PANDORA_RECEIVE_FROM_PROM_H_

#include "common.h"
#include "pandora.h" // pour les type_script group etc...

#define diff(a,b) (a > b ? a-b : b-a)
#define permut(a,b,tmp) {tmp = a; a = b; b = tmp;} // TODO : probablement à supprimer
/* "En-tête" de variables globales */
extern pthread_t enet_thread;

/* En-tête de fonctions */
gboolean queue_draw(gpointer data);
void server_for_promethes();
void enet_manager(ENetHost *server);
void verify_script(type_script *script);
void verify_group(type_group *group);
void sort_list_groups_by_rate(type_group **groups, int number_of_groups);

#endif /* PANDORA_RECEIVE_FROM_PROM_H_ */
