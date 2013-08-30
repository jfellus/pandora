/**
 * pandora_save.h
 *
 *  Created on: 2 juil. 2013
 *      Author: Nils Beaussé
 **/

#ifndef PANDORA_SAVE_N
#define PANDORA_SAVE_N

#include "pandora.h" // Pour les types script/group etc...
#define MAX_LENGHT_PATHNAME  100
#define MAX_LENGHT_FILENAME  50

/* En-tête de fonctions */

void continuous_saving(type_group *used_group);

void destroy_saving_ref(type_script *scripts_used[NB_SCRIPTS_MAX]);

void destroy_saving_ref_one(type_script *scripts_used);

void file_create(type_group *used_group);

#endif

