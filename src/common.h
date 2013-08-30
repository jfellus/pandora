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

