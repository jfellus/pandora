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
 * pandora_prompt.h
 *
 *  Created on: 19 juil. 2013
 *      Author: Nils Beaussé
 **/

#ifndef PANDORA_PROMPT_H_
#define PANDORA_PROMPT_H_

#include "common.h"
#include "pandora.h"

#define NB_LIGN_MAX 30

typedef struct prompt_lign {
  char text_lign[GROUP_NAME_MAX];
  double time_to_prompt;
  gboolean prompt;
  int group_id;
} prompt_lign;

/* "En-tête" de variables globales */
extern GtkTextBuffer * p_buf;
extern prompt_lign prompt_buf[];

gboolean send_info_to_top(gpointer *user_data);
void init_top(prompt_lign *buf, GtkTextBuffer *text_buf);
void insert_lign(prompt_lign lign, prompt_lign *prompt_buf);
void decal_tab(prompt_lign *prom_buf, int i);
void prompt(prompt_lign *prom_buf, GtkTextBuffer *text_buf);
void init_lign(prompt_lign *lign);
void delete_lign(prompt_lign *prom_buf, int i);
#endif /* PANDORA_PROMPT_H_ */
