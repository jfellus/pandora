/*  japet_receive_from_prom.c
 *  
 *  Auteurs: Arnaud Blanchard et Brice Errandonea
 * 
 */

#include "japet.h"
#ifdef ETIS

#include <gtk/gtk.h>
#include <enet/enet.h>
#include "../prom_tools/include/basic_tools.h"
#include "net_message_debug_dist.h"
#include "../prom_kernel/include/japet_connect.h"
#include "prom_bus.h"

/*
#include "../prom_kernel/include/prom_enet_debug.h"
#include "../prom_tools/include/oscillo_kernel_display.h"
#include "reseau.h"
#include "enet_server.h"
#include "themis_ivy.h"
#include "themis.h"*/

//extern ENetHost* enet_server = NULL;


#define MAX_SIZE_OF_PROM_BUS_MESSAGE 256
#define BROADCAST_MAX 32
#define SIZE_OF_IVY_PROM_NAME 64

/*#define EXIT_ON_ERROR printf  Il faudrait se faire un vrai EXIT_ON_ERROR avec print_fatal_error comme fait dans themis et avec le define  de basic_tool.h. C'est maintenant dans japet.h */

ENetHost* enet_server = NULL;
sem_t ivy_semaphore;
char ivy_prom_name[SIZE_OF_IVY_PROM_NAME]; /* utiliser un define, peut être le mettre en commun a themis */


const char* id="japet"; /* Identifiant des promethes communiquants avec japet. Il faudra que ce soit passé en parametre sous la forme japet -i<identifiant>  en attendant c'est fixe il faut donc lancer promethe -ijapet ou themis -ijapet (voir main de themis.c)*/

void enet_manager(ENetHost *server); /* Sinon il faut mettre la fonction avant les appels */ 

ivyServer ivyServers[NB_SCRIPTS_MAX]; //Stocke l'ip de chaque promethe qui se connecte et le nom du script qu'il exécute
int ivyServerNb = 0; //Ce numéro sera affecté au prochain promethe qui se connectera

/**
 * 
 * Permet d'envoyer un message via le port ivy au format printf
 * 
 * @param *format: le message à afficher
 */
void japet_bus_send_message(const char *format, ...)
{
  char buffer[MAX_SIZE_OF_PROM_BUS_MESSAGE];
  va_list arguments;  
  va_start(arguments, format);
  vsprintf(buffer, format, arguments);    
  sem_wait(&ivy_semaphore);  
  IvySendMsg("%s:%s", id, buffer); /* Les message sont précédés de l'id pour que les promethes d'autres utilisateurs sur le même réseau ne reagissent pas.*/
  printf("Message envoyé : %s:%s\n", id, buffer);
  sem_post(&ivy_semaphore);  
}


void ivyApplicationCallback (IvyClientPtr app, void *user_data, IvyApplicationEvent event)
{
  char *appname;
  char *host;

  (void)user_data;

  appname = IvyGetApplicationName (app);
  host = IvyGetApplicationHost (app);

  switch  (event)  {
  case IvyApplicationConnected:
    printf("Connexion ivy de %s d'address %s\n", appname, host);
    strcpy(ivyServers[ivyServerNb].appName, appname);
    strcpy(ivyServers[ivyServerNb].ip, host);
    printf("Connexion ivy de %s d'adresse %s\n", ivyServers[ivyServerNb].appName, ivyServers[ivyServerNb].ip);
    ivyServerNb++;
    break;

  case IvyApplicationDisconnected:
    printf("%s disconnected (ivy) from %s\n", appname,  host);
    break;

  default:
    printf("%s: unkown ivy event %d\n", appname, event);
    break;
  }
}

void prom_bus_init(char* brodcast_ip)
{
  char broadcast[BROADCAST_MAX];
  char computer_name[HOST_NAME_MAX];
  pid_t my_pid;

  snprintf(broadcast,  BROADCAST_MAX, "%s:2010", brodcast_ip);

  gethostname(computer_name, HOST_NAME_MAX);
  my_pid = getpid();

  snprintf(ivy_prom_name, SIZE_OF_IVY_PROM_NAME, "japet:%s:%ld", computer_name, (long int)my_pid); /* Le nom unique du programme sur le bus ivy sera japet:<hostanem>:<pid du processus */

  sem_init(&ivy_semaphore, 0, 1);   /* semaphore necessaire pour faire des appels ivy thread safes.*/
  IvyInit(ivy_prom_name, "Japet started", ivyApplicationCallback, NULL, NULL, NULL);
  IvyStart(broadcast);

}

/**
 * 
 * Crée dans Japet un serveur, qui va écouter le réseau et accepter les connexions des Prométhés
 * 
 */
void server_for_promethes()
{
	char host_name[HOST_NAME_MAX];
	ENetAddress address;
	int error;
	pthread_t enet_thread;

	enet_time_set(0);

	address.host = ENET_HOST_ANY;
	address.port = JAPET_PORT;

	enet_server = enet_host_create(&address, NB_SCRIPTS_MAX, JAPET_NUMBER_OF_CHANNELS, ENET_UNLIMITED_BANDWITH, ENET_UNLIMITED_BANDWITH);

	if (enet_server != NULL)
	{
		gethostname(host_name, HOST_NAME_MAX);
		/* prom_bus_send_message("connect_profiler(%s:%d)", host_name, port);*/
		error = pthread_create(&enet_thread, NULL, (void*(*)(void*)) enet_manager, enet_server);
	}
	else EXIT_ON_ERROR("Fail to create a enet server for oscillo kernel !\n\tCheck that there is no other japet running.");
}


/*
void on_oscillo_kernel_start_button_clicked(GtkWidget *widget, void *data)
{
	(void) widget;
	(void) data;void prom_bus_send_message(const char *format, ...)
{
  char buffer[MAX_SIZE_OF_PROM_BUS_MESSAGE];
  va_list arguments;

  va_start(arguments, format);
  vsprintf(buffer, format, arguments);
  sem_wait(&ivy_semaphore);
  IvySendMsg("%s:%s", themis.id, buffer)
	if (enet_server== NULL)
	{
		init_oscillo_kernel(1235); 
		oscillo_kernel_create_window();
	}

	prom_bus_send_message("distant_debug(%d)", DISTANT_DEBUG_START);
	oscillo_kernel_activated=1;
}*/

void enet_manager(ENetHost *server)
{
	gchar ip[HOST_NAME_MAX];
	int running = 1;
	int i, id_of_script;
	int number_of_groups;
	ENetEvent event;
	
	//variables temporaires, pour analyser chaque groupe reçu
	char groupName[TAILLE_CHAINE];
	char strGroupY[sizeof(int)];
	int groupY;
	char strGroupFirstNeuron[sizeof(int)];
	int groupFirstNeuron;
	char strGroupType[sizeof(int)];
	int groupType;
	char strGroupRows[sizeof(int)];
	int groupRows;
	char strGroupColumns[sizeof(int)];
	int groupColumns;
	char strGroupNbNeurons[sizeof(int)];
	int groupNbNeurons;
	char strGroupLearningSpeed[sizeof(float)];
	float groupLearningSpeed;
	
	while (running)
	{
		/* Wait up to 1000 milliseconds for an event. */
		while (enet_host_service(server, &event, 2000) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				
				enet_address_get_host_ip(&event.peer->address, ip, HOST_NAME_MAX);
				printf("A new client connected (ENet) from ip %s:%i.\n", ip, event.peer->address.port);				
					
				for (i = 0; i < NB_SCRIPTS_MAX; i++) 
				{
				  if (strcmp(ivyServers[i].ip, "localhost") == 0) strcpy(ivyServers[i].ip, "127.0.0.1");
				  printf("%d : %s %s\n", i, ivyServers[i].ip, ip);
				  if (strcmp(ivyServers[i].ip, ip) == 0)
				  {
				    event.peer->data = &Index[i];
				    printf("Le script %s envoyé par le client à l'adresse %s a reçu le numéro %d.\n", ivyServers[i].appName, ivyServers[i].ip, i);
				    break;
				  }
				}				  				  										
				
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				printf("Japet reçoit quelque chose.\n");
				switch (event.channelID) 
				{
				      /*case ENET_DEF_SCRIPTNAME_CHANNEL: //Initialisation de ce script
				//		printf("Connexion promethe %s\n", (char*)event.packet->data);				
				
						//Ouverture du script
						newScript(&scr[newScriptNumber], Nom du script, ip, newScriptNumber, Nombre de groupes dans le script);
						number_of_groups = (event.packet->dataLength) / sizeof(type_com_groupe);
						event.peer->data = oscillo_kernel_add_promethe((type_com_groupe*) event.packet->data, number_of_groups, (const char*)event.packet->data);
						break;				  */
				  
				      case ENET_DEF_GROUP_CHANNEL: //Réception de nouvelles données concernant certains groupes
				//		printf("Connexion promethe %s\n", (char*)event.packet->data);																

						//Création du script																		
						number_of_groups = (event.packet->dataLength) / sizeof(type_com_groupe);
						printf("Japet reçoit %d groupes\n", number_of_groups);
						newScript(&scr[*((gint*)(event.peer->data))], "Script generique", ip, *((gint*)(event.peer->data)), number_of_groups);										
				
				/*
				//Création des groupes

				for (i = 0; i < number_of_groups; i++) 
				{
				  strncpy(groupName, event.packet->data + i * sizeof(type_com_groupe) + SIZE_NO_NAME, TAILLE_CHAINE);
				  printf("%s\n", groupName);
				  strncpy(strGroupY, event.packet->data + i * sizeof(type_com_groupe) + SIZE_NO_NAME + TAILLE_CHAINE + 8, sizeof(int));
				  groupY = 2^24 * strGroupY[0] + 2^16 * strGroupY[1] + 2^8 * strGroupY[2] + strGroupY[3];
				  printf("y = %d\n", groupY);
				  strncpy(strGroupFirstNeuron, event.packet->data + i * sizeof(type_com_groupe) + SIZE_NO_NAME + TAILLE_CHAINE + 44, sizeof(int));
				  printf("premier neurone = %d\n", groupFirstNeuron);
				  strncpy(strGroupType, event.packet->data + i * sizeof(type_com_groupe) + SIZE_NO_NAME + TAILLE_CHAINE + 56, sizeof(int));
				  printf("type = %d\n", groupType);
				  strncpy(strGroupRows, event.packet->data + i * sizeof(type_com_groupe) + SIZE_NO_NAME + TAILLE_CHAINE + 64, sizeof(int));
				  printf("nb de lignes = %d\n", groupRows);
				  strncpy(strGroupColumns, event.packet->data + i * sizeof(type_com_groupe) + SIZE_NO_NAME + TAILLE_CHAINE + 68, sizeof(int));
				  printf("nb de colonnes = %d\n", groupColumns);
				  strncpy(strGroupNbNeurons, event.packet->data + i * sizeof(type_com_groupe) + SIZE_NO_NAME + TAILLE_CHAINE + 72, sizeof(int));
				  printf("nb de neurones = %d\n", groupNbNeurons);
				  strncpy(strGroupLearningSpeed, event.packet->data + i * sizeof(type_com_groupe) + SIZE_NO_NAME + TAILLE_CHAINE + 104, sizeof(float));
				  printf("taux d'apprentissage = %f\n", groupLearningSpeed);
						  
				  newGroup(&scr[atoi(event.peer->data)].groups[i],
					   &scr[atoi(event.peer->data)], 
					   groupName, 
					   groupType, 
					   groupLearningSpeed, 
					   groupNbNeurons, 
					   groupRows, 
					   groupColumns, 
					   groupY, 
					   1 //pour l'instant
					   );
				  }*/
				
						/*event.peer->data = oscillo_kernel_add_promethe((type_com_groupe*) event.packet->data, number_of_groups, (const char*)event.packet->data);*/
						break;

						/*					case ENET_GROUP_EVENT_CHANNEL:
						if (event.peer->data != NULL)
						{
						  id_of_script  =  (int)event.peer->data;
						  network_data = (type_nn_message*)event.packet->data;
						  group_profiler_update_info(event.peer->data, last_pos_oscillo_kernel, network_data->gpe, network_data->type_message, network_data->time_stamp);
						}
						break;*/
				}
				/* Clean up the packet now that we're done using it. */
				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				printf("%s disconnected.\n", (char*) event.peer->data);
				/* Reset the peer's client information. */
				event.peer->data = NULL;
				break;

			case ENET_EVENT_TYPE_NONE:
				printf("ENET: none event \n");
				break;
			}
		}
	}
}

void stop_oscillo_kernel()
{
	enet_host_destroy(enet_server);
	enet_deinitialize();
}

#endif
