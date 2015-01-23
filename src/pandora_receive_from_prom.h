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
extern sem_t enet_pandora_lock;
/* En-tête de fonctions */
gboolean queue_draw(gpointer data);
void server_for_promethes();
void enet_manager(ENetHost *server);
void verify_script(type_script *script);
void verify_group(type_group *group);
void sort_list_groups_by_rate(type_group **groups, int number_of_groups);
type_group* search_associated_group(int no_neuro, type_script* script);
void create_links(type_group *group, int no_neuro, enet_uint8 *current_data, int number_of_neuro_links, type_script* script);


#endif /* PANDORA_RECEIVE_FROM_PROM_H_ */
