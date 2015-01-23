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
 * common.h
 *
 *  Created on: 15 juil. 2013
 *      Author: Nils Beaussé
 */
/**
 * Ce fichier contient les include standard necessaire au fonctionnement de Pandora. Simplifiant le probleme des ordres d'inclusion.
 */
#ifndef PANDORA_COMMON_H_
#define PANDORA_COMMON_H_

/** Partie Biblioteques standards **/
/* En-têtes classiques */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>

/* Systeme de semaphore */
#include <semaphore.h>

/** Partie biblioteques non-standards **/
/* Fichiers communs pour l'IHM */
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <cairo.h>

/** Partie Promethe **/
/* fichiers communs à promethe */
#include "colors.h"
#include "prom_tools/include/xml_tools.h"
#include "enet/include/enet/enet.h"
#include "prom_kernel/include/pandora_connect.h"
#include "prom_kernel/include/reseau.h"
#include "prom_user/include/Struct/prom_images_struct.h"
#include <net_message_debug_dist.h>
#include "basic_tools.h"

#endif

