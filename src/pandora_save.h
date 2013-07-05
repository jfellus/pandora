/*
 * pandora_save.h
 *
 *  Created on: 2 juil. 2013
 *      Author: nilsbeau
 */



#ifndef PANDORA_SAVE_N
#define PANDORA_SAVE_N


#include "pandora.h"
#include "locale.h"
#include "graphic.h"


//#define MAX_LENGHT_GROUPNAME 20
#define MAX_LENGHT_PATHNAME  100
#define PATH_NAME "save/"

extern gboolean saving_press;
extern GtkWidget *architecture_display;

void continuous_saving(type_group *used_group);

void destroy_saving_ref(type_script *scripts_used[NB_SCRIPTS_MAX]);

void destroy_saving_ref_one(type_script *scripts_used);

void file_create(type_group *used_group);

//void tabula_rasa_selected(type_script *scripts_to_deselect[NB_SCRIPTS_MAX]);

//gboolean compare_list(char* string_table, char compared);


#endif


