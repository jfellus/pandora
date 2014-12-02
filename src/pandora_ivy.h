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
 * pandora_ivy.h
 *
 *  Created on: Aug 23, 2012
 *      Author: arnablan
 **/

#ifndef PANDORA_IVY_H
#define PANDORA_IVY_H

#define BUS_ID_MAX 128
#define BROADCAST_MAX 32
#define SIZE_OF_IVY_PROM_NAME 64
#define MAX_SIZE_OF_PROM_BUS_MESSAGE 256

#include <ivy.h>
#include "common.h"

typedef struct ivyServer {
  char ip[30];
  char appName[64];
} ivyServer;

/* "En-tête" de variables globales */
extern sem_t ivy_semaphore;

/* En tete de fonctions */
void pandora_bus_send_message(char *id, const char *format, ...);
void ivyApplicationCallback(IvyClientPtr app, void *user_data, IvyApplicationEvent event);
void prom_bus_init(const char *ip);

#endif /* JAPET_IVY_H_ */
