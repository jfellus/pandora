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

extern GtkTextBuffer * p_buf;

typedef struct prompt_lign {
  char text_lign[GROUP_NAME_MAX];
  double time_to_prompt;
  gboolean prompt;
  int group_id;
} prompt_lign;

gboolean send_info_to_top(gpointer *user_data);
void init_top(prompt_lign *buf, GtkTextBuffer *text_buf);
void insert_lign(prompt_lign lign, prompt_lign *prompt_buf);
void decal_tab(prompt_lign *prom_buf, int i);
void prompt(prompt_lign *prom_buf, GtkTextBuffer *text_buf);
void init_lign(prompt_lign *lign);
void delete_lign(prompt_lign *prom_buf, int i);
#endif /* PANDORA_PROMPT_H_ */
