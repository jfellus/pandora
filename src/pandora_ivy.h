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
  char ip[18];
  char appName[30];
} ivyServer;

extern sem_t ivy_semaphore;
extern int ivyServerNb;

void pandora_bus_send_message(char *id, const char *format, ...);
void ivyApplicationCallback(IvyClientPtr app, void *user_data, IvyApplicationEvent event);
void prom_bus_init(const char *ip);

#endif /* JAPET_IVY_H_ */
