/*
 *      prom_send_to_japet.c
 *
 *      Brice Errandonea, Arnaud Blanchard (d'après Philippe Gaussier : prom_enet_debug.c)
 */

/* #define DEBUG 1 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <enet/enet.h>

#include <sys/timeb.h>
#include <signal.h>
#include <math.h>
#include <bits/local_lim.h>

#include "basic_tools.h"
#include "public.h"
#include "net_message_debug_dist.h"
#include "prom_enet_debug.h"
#include "oscillo_kernel.h"
#include "reseau.h"

#include "japet_connect.h"

int japet_activated = 0;

sem_t japet_lock;
ENetHost *japet_host;
ENetAddress japet_address;
ENetPeer *japet_peer;

void japet_enet_manager(int time)
{
	char ip[HOST_NAME_MAX];
	int running = 1;
	ENetEvent event;
	ENetPacket *name_packet, *groups_packet, *links_packet, *neurons_packet;
	size_t groupsPacketSize, linksPacketSize, neuronsPacketSize;
	
	type_com_groupe* def_com_group;
	type_liaison* def_link;
	type_neurone* def_neuron;

	if (enet_host_service(japet_host, &event, time) < 0) EXIT_ON_ERROR("Problem managing enet");	
	
	switch (event.type)
	{
	case ENET_EVENT_TYPE_CONNECT:
		printf("Prométhé reçoit la demande de Japet\n");
		enet_address_get_host_ip(&event.peer->address, ip, HOST_NAME_MAX);		
		kprints("A new client connected from ip %s:%i.\n", ip, event.peer->address.port);
		event.peer->data = NULL;
		japet_peer = event.peer;

		sem_wait(&(japet_lock));
		
		//Envoi du nom du script
		printf("Nom de script à envoyer : %s\n", virtual_name);		
		name_packet = enet_packet_create(virtual_name, strlen(virtual_name) * sizeof(char), ENET_PACKET_FLAG_RELIABLE & ENET_PACKET_FLAG_NO_ALLOCATE);
		if (name_packet == NULL) EXIT_ON_ERROR("The name packet has not been created.");
		if (enet_peer_send(event.peer, ENET_PROMETHE_DESCRIPTION_CHANNEL, name_packet) != 0) EXIT_ON_ERROR("The name packet has not been sent.");
		else printf("Le nom %s a été envoyé\n", virtual_name);		
		
		//Envoi des groupes
		printf("Nombre de groupes à envoyer : %d\n", nbre_groupe);		
		groupsPacketSize = sizeof(type_com_groupe) * nbre_groupe;
		def_com_group = MANY_ALLOCATIONS(nbre_groupe, type_com_groupe);
		init_com_def_groupe(def_com_group);
		groups_packet = enet_packet_create(def_com_group, groupsPacketSize, ENET_PACKET_FLAG_RELIABLE & ENET_PACKET_FLAG_NO_ALLOCATE);
		if (groups_packet == NULL) EXIT_ON_ERROR("The groups packet has not been created.");
		if (enet_peer_send(event.peer, ENET_DEF_GROUP_CHANNEL, groups_packet) != 0) EXIT_ON_ERROR("The groups packet has not been sent.");
		else printf("Les %d groupes ont été envoyés\n", nbre_groupe);		
				
		//Envoi des liaisons		
		printf("Nombre de laisons à envoyer : %d\n", nbre_liaison);		
		linksPacketSize = sizeof(type_liaison) * nbre_liaison;
		//def_link = MANY_ALLOCATIONS(nbre_liaison, type_liaison);		
		links_packet = enet_packet_create(liaison, linksPacketSize, ENET_PACKET_FLAG_RELIABLE & ENET_PACKET_FLAG_NO_ALLOCATE);
		if (links_packet == NULL) EXIT_ON_ERROR("The links packet has not been created.");
		if (enet_peer_send(event.peer, ENET_DEF_LINK_CHANNEL, links_packet) != 0) EXIT_ON_ERROR("The links packet has not been sent.");
		else printf("Les %d liaisons ont été envoyées\n", nbre_liaison);
		
		//Envoi des neurones				
		printf("Nombre de neurones à envoyer : %d\n", nbre_neurone);		
		neuronsPacketSize = sizeof(type_neurone) * nbre_neurone;
		//def_neuron = MANY_ALLOCATIONS(nbre_neurone, type_neurone);		
		neurons_packet = enet_packet_create(neurone, neuronsPacketSize, ENET_PACKET_FLAG_RELIABLE & ENET_PACKET_FLAG_NO_ALLOCATE);
		if (neurons_packet == NULL) EXIT_ON_ERROR("The neurons packet has not been created.");
		if (enet_peer_send(event.peer, ENET_DEF_NEURON_CHANNEL, neurons_packet) != 0) EXIT_ON_ERROR("The neurons packet has not been sent.");
		else printf("Les %d neurones ont été envoyés\n", nbre_groupe);
		
				
		enet_host_service(japet_host, &event, 1000);
		enet_host_service(japet_host, &event, 0);
		free(def_com_group);
		//free(def_link);
		//free(def_neuron);
		/*
		 connexion->packet = enet_packet_create(Tableau des neurones, packetSize, ENET_PACKET_FLAG_RELIABLE);
		 if (connexion->packet == NULL)  EXIT_ON_ERROR("The packet has not been created.");
		 if (enet_peer_send (connexion->peer, ENET_DEF_NEURONS_CHANNEL, connexion->packet) !=0) EXIT_ON_ERROR("The packet has not been sent.");

		 connexion->packet = enet_packet_create(Tableau des laisons, packetSize, ENET_PACKET_FLAG_RELIABLE);
		 if (connexion->packet == NULL)  EXIT_ON_ERROR("The packet has not been created.");
		 if (enet_peer_send (connexion->peer, ENET_DEF_LINKS_CHANNEL, connexion->packet) !=0) EXIT_ON_ERROR("The packet has not been sent.");*/

		sem_post(&(japet_lock));
		japet_activated = 1;
		//japet_host = enet_host_create(&event.peer->address, NB_SCRIPTS_MAX, JAPET_NUMBER_OF_CHANNELS, ENET_UNLIMITED_BANDWITH, ENET_UNLIMITED_BANDWITH);
		break;

	case ENET_EVENT_TYPE_RECEIVE:
		if (event.channelID == ENET_COMMAND_CHANNEL)
		{
			switch (*(int*) event.peer->data)
			{
			case ENET_COMMAND_STOP_OSCILLO:
				oscillo_dist_activated = 0;
				break;
			case ENET_COMMAND_START_OSCILLO:
				oscillo_dist_activated = 1;
				break;
			}
		}
		break;

	case ENET_EVENT_TYPE_DISCONNECT:
		kprints("%s disconected.\n", (char*) event.peer->data);
		/* Reset the peer's client information. */
		event.peer->data = NULL;
		break;

	case ENET_EVENT_TYPE_NONE:
		
		if (japet_activated == 1)
		{
		  //Envoyer les neurones qui ont changé sur le canal ENET_UPDATE_NEURON_CHANNEL 
		}
		//else kprints("ENET: none event \n");
		break;
	}
}


//Appelée par generic_create_and_manage() (dans gestion_threads.c)
//Transmet à Japet les nouvelles valeurs de sortie des neurones du script chaque fois qu'elles changent
void send_neurons_to_japet(int gpe)
{
  size_t neuronsPacketSize;
  type_neurone* update_neurons;
  ENetPacket* neurons_packet;
  ENetEvent event;
  
  int i;
  
  int nbRefreshedNeurons = def_groupe[gpe].nbre;
  
  //def_groupe[gpe].premier_ele    def_groupe[gpe].nbre
  
  if (japet_activated == 1)
  {  
    sem_wait(&(japet_lock));
  
    printf("Nombre de neurones à envoyer pour mise à jour : %d\n", nbRefreshedNeurons);		
    neuronsPacketSize = sizeof(type_neurone) * nbRefreshedNeurons;
    //update_neurons = MANY_ALLOCATIONS(nbRefreshedNeurons, type_neurone);		
    
    
    //On met le champ .groupe de tous ces neurones à gpe
    for (i = 0; i < nbRefreshedNeurons; i++)
    {
      neurone[def_groupe[gpe].premier_ele + i].groupe = gpe;
    }    
    
    neurons_packet = enet_packet_create(&neurone[def_groupe[gpe].premier_ele], neuronsPacketSize, ENET_PACKET_FLAG_RELIABLE & ENET_PACKET_FLAG_NO_ALLOCATE);
    if (neurons_packet == NULL) EXIT_ON_ERROR("The neurons packet (update) has not been created.");
    if (enet_peer_send(japet_peer, ENET_UPDATE_NEURON_CHANNEL, neurons_packet) != 0) EXIT_ON_ERROR("The neurons packet has not been sent.");
    else printf("Les %d neurones ont été envoyés\n", nbRefreshedNeurons);
    enet_host_service(japet_host, &event, 0);
    sem_post(&(japet_lock));    
  }
  
}

/* init network for udp connexion using enet : used for distant oscillo_kernel */
/* ip_adr= 127.0.0.1 par ex ou nom_adr=localhost... */

int japet_connect(char *ip_adr, int port)
{
	int error;
	size_t packetSize;
	ENetEvent event;
/*	pthread_t enet_thread;*/

	if (sem_init(&(japet_lock), 0, 1) != 0) EXIT_ON_SYSTEM_ERROR("Fail to init semaphore.");
	/* attend que le message precedent ait ete envoye */

	/* network initialization: */
	if (enet_initialize() != 0) EXIT_ON_ERROR("An error occurred while initializing ENet.\nAn error occurred while initializing ENet.\n");

	atexit(enet_deinitialize); /* PG: A verifier si pas de conflit au niveau global de promethe */
	enet_time_set(0);

	japet_host = enet_host_create(NULL, 1, JAPET_NUMBER_OF_CHANNELS, ENET_UNLIMITED_BANDWITH, ENET_UNLIMITED_BANDWITH);

	if (!japet_host) EXIT_ON_ERROR("An error occurred while trying to create an ENet client host.\n");
	else printf("Hôte japet créé\n");

	if (enet_address_set_host(&(japet_address), ip_adr) != 0) EXIT_ON_ERROR("Error with address %s", ip_adr);
	japet_address.port = port;
	japet_peer = enet_host_connect(japet_host, &(japet_address), JAPET_NUMBER_OF_CHANNELS, 0);

	japet_enet_manager(5000);
	/*	if (pthread_create(&enet_thread, NULL, (void*(*)(void*)) japet_enet_manager, NULL) != 0) EXIT_ON_ERROR("Unable to create thread.");*/
	return 0;
}

/*
 void quit_japet(type_connexion_udp *connexion)
 {
 int event_return;


 if (connexion->data != NULL)
 {
 free(connexion->data);
 connexion->data = NULL;
 }
 if (connexion->peer)
 {
 enet_peer_disconnect(connexion->peer, 0);
 event_return = enet_host_service(connexion->host, &connexion->event, connexion->waitTime);
 if (event_return < 0) printf("error in event_host_service\n");

 printf("resetting peer...");
 fflush(stdout);
 enet_peer_reset(connexion->peer);
 printf(" done.\n");
 }

 printf("closing down...\n");
 fflush(stdout);
 enet_host_destroy(connexion->host);
 printf(" done.\n");
 enet_deinitialize();
 exit(1);
 }

 */

/*void send_token_oscillo_kernel(int gpe, int phase, long temps)
 {
 type_nn_message data_network;
 int res;

 data_network.type_trame = GROUP_DEBUG;
 no_message++;
 data_network.val_nn_message_id = NN_message_magic_card;
 data_network.no_message = no_message;
 data_network.time_stamp = temps;
 data_network.gpe = gpe;
 data_network.type_message = phase;
 res = sem_wait(&(network_debug_link.lock));
 send_debug_data(&network_debug_link, &data_network, sizeof(type_nn_message), ENET_GROUP_EVENT_CHANNEL);
 sem_post(&(network_debug_link.lock));
 }*/
