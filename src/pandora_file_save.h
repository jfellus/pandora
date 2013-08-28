/*
 * pandora_file_save.h
 *
 *  Created on: 5 août 2013
 *      Author: Nils Beaussé
 */

#ifndef PANDORA_FILE_SAVE_H_
#define PANDORA_FILE_SAVE_H_
#include "common.h"
#include "pandora.h"

const char* whitespace_callback_pandora(mxml_node_t *node, int where);
void pandora_file_save(const char *filename);
void pandora_file_load(const char *filename);
void pandora_file_load_script(const char *filename, type_script *script);
void save_preferences(GtkWidget *pWidget, gpointer pData);
void save_preferences_as(GtkWidget *pWidget, gpointer pData);

#endif /* PANDORA_FILE_SAVE_H_ */
