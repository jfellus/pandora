#ifndef JAPET_CONNECT_H
#define JAPET_CONNECT_H

#include <ivy.h>
#include <Ivy/ivyglibloop.h>

#define JAPET_NUMBER_OF_CHANNELS 8 //Nombre de canaux de communication entre Prométhé et Japet

#define NB_SCRIPTS_MAX 30 //Nombre maximal de scripts qu'on peut détecter (et donc afficher) simultanément

enum{
 JAPET_START=0,
 JAPET_WANTS_GROUPS=1,
 JAPET_WANTS_NEURONS=2,
 JAPET_WANTS_LINKS=3
};

//Implémentée dans ../prom_kernel/src/prom_send_to_japet.c
int japet_connect(char* ip_adr, int port);

//void quit_japet(type_connexion_udp* connexion);

//Implémentée dans ../prom_kernel/src/shared/ivy_bus.c
void japet_callback(IvyClientPtr app, void* data, int argc, char** argv);

#endif