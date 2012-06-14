/**  japet_receive_from_prom.c
 *  
 *  @Author Arnaud Blanchard et Brice Errandonea
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

#include "../prom_kernel/include/reseau.h"


/*
#include "../prom_kernel/include/prom_enet_debug.h"
#include "../prom_tools/include/oscillo_kernel_display.h"

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


int freePeer = 0; //Utilisé pour numéroter les peers (donc les promethes, donc les scripts) (ENet)

/**
 * 
 * Permet d'envoyer un message via le port ivy au format printf
 * 
 * @param *format: le message à afficher
**/
void japet_bus_send_message(const char *format, ...)
{
  char buffer[MAX_SIZE_OF_PROM_BUS_MESSAGE];
  va_list arguments;  
  va_start(arguments, format);
  vsprintf(buffer, format, arguments);    
  sem_wait(&ivy_semaphore);  
  /// Les message sont précédés de l'id pour que les promethes d'autres utilisateurs sur le même réseau ne reagissent pas.
  IvySendMsg("%s:%s", id, buffer); 
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
    if (strncmp(appname, "themis:", 7) == 0) break; //On ne veut pas référencer Thémis comme étant un script
    strcpy(ivyServers[ivyServerNb].appName, appname);
    strcpy(ivyServers[ivyServerNb].ip, host);
    if (strcmp(ivyServers[ivyServerNb].ip, "localhost") == 0) strcpy(ivyServers[ivyServerNb].ip, "127.0.0.1");
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
	int i, j, k, l;
	int id_of_script;
	int number_of_groups, number_of_links, number_of_neurons;
	ENetEvent event;
	int *addr_i;	
		
	//paquets reçus
	char* received_name_packet;
	type_com_groupe* received_groups_packet;
	type_liaison* received_links_packet;		
	type_neurone* received_neurons_packet;					
	
	//variables temporaires, pour analyser chaque groupe reçu
	char groupName[SIZE_NO_NAME];	
	int groupY;
	int groupFirstNeuron;
	char groupType[TAILLE_CHAINE];
	int groupRows;
	int groupColumns;
	int groupNbNeurons;
	float groupLearningSpeed;
	int groupId; //numéro de ce groupe dans le tableau envoyé par prométhé

	
	//variables temporaires, pour analyser chaque liaison reçue
	int linkSecondary;
	char previousName[SIZE_NO_NAME];
	char nextName[SIZE_NO_NAME];
	int linkCreated;
	
	//variables temporaires, pour analyser chaque neurone reçu
	int neuronGroupId;
	int s;
	int s1;
	int s2;
	int neuronX;
	int neuronY;		
		
	//int request; //Requête envoyée à un Prométhé
	//ENetPacket* answer_packet;
	
	while (running)
	{
		gint script_id;
		/* Wait up to 2000 milliseconds for an event. */
		while (enet_host_service(server, &event, 2000) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				
				enet_address_get_host_ip(&event.peer->address, ip, HOST_NAME_MAX);
				printf("A new client connected (ENet) from ip %s:%i.\n", ip, event.peer->address.port);				
				
				event.peer->data = &Index[freePeer];
				freePeer++;
				
				
				/*
				//Recherche du numéro qu'ivy a attribué à la machine située à cette ip
				for (i = 0; i < NB_SCRIPTS_MAX; i++) 
				{				  
				  if (strcmp(ivyServers[i].ip, ip) == 0)
				  {
				    addr_i = ALLOCATION(int); 
				    *addr_i = i;
				    event.peer->data = addr_i;//&Index[i];
				    //printf("Le script %s envoyé par le client à l'adresse %s a reçu le numéro %d.\n", ivyServers[i].appName, ivyServers[i].ip, i);
				    break;
				  }
				  
				}*/				  				  									
				
				//request = JAPET_WANTS_GROUPS;
				//answer_packet = enet_packet_create(&request, sizeof(int), ENET_PACKET_FLAG_RELIABLE & ENET_PACKET_FLAG_NO_ALLOCATE);
				
				//if (enet_peer_send (event.peer, ENET_DEF_GROUP_CHANNEL, answer_packet) !=0) EXIT_ON_ERROR("Les groupes n'ont pas été demandés.\n");				
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				//printf("Japet reçoit quelque chose.\n");
				script_id = *((gint*)(event.peer->data)); //Numéro du peer, et donc du script, qui lui a été attribué lors de sa connexion (voir cas précédent)
				switch (event.channelID) 
				{
				      /*case ENET_DEF_SCRIPTNAME_CHANNEL: //Initialisation de ce script
				//		printf("Connexion promethe %s\n", (char*)event.packet->data);				
				
						//Ouverture du script
						newScript(&scr[newScriptNumber], Nom du script, ip, newScriptNumber, Nombre de groupes dans le script);
						number_of_groups = (event.packet->dataLength) / sizeof(type_com_groupe);
						event.peer->data = oscillo_kernel_add_promethe((type_com_groupe*) event.packet->data, number_of_groups, (const char*)event.packet->data);
						break;						
						*/
				      
				      case ENET_PROMETHE_DESCRIPTION_CHANNEL:
 						received_name_packet = (gchar*) event.packet->data;						
						//printf("nom du script pour promethe : %s\n", received_name_packet); 
						//printf("longueur : %d\n", (int) event.packet->dataLength);
						strncpy(scriptsNames[script_id], received_name_packet, event.packet->dataLength);
						//printf("nom du script pour japet : %s\n", scriptsNames[script_id]); 
						break;					
					
				  
				      case ENET_DEF_GROUP_CHANNEL: //Réception de nouvelles données concernant certains groupes
				//		printf("Connexion promethe %s\n", (char*)event.packet->data);	
						if(scr[script_id].z == -4) //Si ce script n'a pas encore été créé
						{
						  //Création du script		
						  number_of_groups = (event.packet->dataLength) / sizeof(type_com_groupe);
						  //printf("Japet reçoit %d groupes\n", number_of_groups);
						  newScript(&scr[script_id], scriptsNames[script_id], ip, script_id, number_of_groups);																  
						  //received_groups_packet = malloc(number_of_groups * sizeof(type_com_group));
						  received_groups_packet = (type_com_groupe*) event.packet->data;
						  printf("Réception des %d groupes du script %s\n", number_of_groups, scr[script_id].name);
						  
						  //Création des groupes
						  for (i = 0; i < number_of_groups; i++)
						  {
						    strncpy(groupName, received_groups_packet[i].no_name, SIZE_NO_NAME);
						    //printf("%s\n", groupName);						    
						    groupY = received_groups_packet[i].posy;
						    //printf("y = %d\n", groupY);						    
						    groupFirstNeuron = received_groups_packet[i].premier_ele;
						    //printf("premier neurone = %d\n", groupFirstNeuron);						    
						    strncpy(groupType, received_groups_packet[i].nom, TAILLE_CHAINE);						    
						    //printf("type = %d\n", groupType);						    
						    groupRows = received_groups_packet[i].tailley;
						    //printf("nb de lignes = %d\n", groupRows);
						    groupColumns = received_groups_packet[i].taillex;
						    //printf("nb de colonnes = %d\n", groupColumns);
						    groupNbNeurons = received_groups_packet[i].nbre;
						    //printf("nb de neurones = %d\n", groupNbNeurons);
						    groupLearningSpeed = received_groups_packet[i].learning_rate;
						    //printf("taux d'apprentissage = %f\n", groupLearningSpeed);
						    groupId = received_groups_packet[i].no;
						    
						    newGroup(&scr[script_id].groups[groupId],
							     &scr[script_id], 
							     groupName, 
							     groupType, 
							     groupLearningSpeed, 
							     groupNbNeurons, 
							     groupRows, 
							     groupColumns, 
							     1, //Plus tard, remplacer par groupY
							     0, //On ne connait pas encore de liaison vers ce groupe				    
							     groupFirstNeuron); 
						  }  
						}				
						break;

					case ENET_DEF_LINK_CHANNEL:								
						
						printf("Réception des liaisons du script %s\n", scr[script_id].name);
						number_of_links = (event.packet->dataLength) / sizeof(type_liaison);						
						
						received_links_packet = (type_liaison*) event.packet->data;
						  
						//Comptage du nombre de liaisons vers chaque groupe
						for (i = 0; i < number_of_links; i++)
						{
						  linkSecondary = received_links_packet[i].secondaire;				
						  strncpy(previousName, received_links_packet[i].depart_name, SIZE_NO_NAME);
						  strncpy(nextName, received_links_packet[i].arrivee_name, SIZE_NO_NAME);
						    
						  //Si c'est une liaison principale (pour l'instant, on ne tient pas compte des liaisons secondaires)
						  if (linkSecondary == 0)
						  {  
						    for (j = 0; j < scr[script_id].nbGroups; j++)
						    {
						      //printf("%s vs %s\n", scr[script_id].groups[j].name, nextName);
						      for (k = 0; k < scr[script_id].nbGroups; k++)
						      if (strcmp(scr[script_id].groups[j].name, nextName) == 0
							  && strcmp(scr[script_id].groups[k].name, previousName) == 0) //Vérifie que le groupe au début de la liaison est dans le groupe script_id
						      {							
							scr[script_id].groups[j].nbLinksTo++; //Il y a une liaison de plus qu'avant vers ce groupe						      
							//printf("%s a %d prédécesseurs\n", scr[script_id].groups[j].name, scr[script_id].groups[j].nbLinksTo);
						      }
						    }  
						  }
						}  
						
						//Allocation mémoire et initialisation des group.previous[] maintenant qu'on sait combien chaque groupe a de prédécesseurs
						printf("Allocation mémoire des liaisons\n");
						for (i = 0; i < scr[script_id].nbGroups; i++)
						{
						  scr[script_id].groups[i].previous = malloc(scr[script_id].groups[i].nbLinksTo * sizeof(group*));
						  for (j = 0; j < scr[script_id].groups[i].nbLinksTo; j++)
						    scr[script_id].groups[i].previous[j] = NULL;
						}
						
						//Création des liaisons
						printf("On va créer les liaisons\n");
						for (i = 0; i < number_of_links; i++)
						{
						  linkSecondary = received_links_packet[i].secondaire;						    
						  strncpy(previousName, received_links_packet[i].depart_name, SIZE_NO_NAME);
						  strncpy(nextName, received_links_packet[i].arrivee_name, SIZE_NO_NAME);
						  //printf("%s -> %s : %d\n", previousName, nextName, linkSecondary);
						  //printf("%d\n", received_links_packet[i].type);
						    
						  //Si c'est une liaison principale (pour l'instant, on ne tient pas compte des liaisons secondaires)
						  if (linkSecondary == 0)
						  {						    
						    for (j = 0; j < scr[script_id].nbGroups; j++)
						    {  
						      if (strcmp(scr[script_id].groups[j].name, nextName) == 0)
						      {
							//printf("j = %d    name = %s    nextName = %s\n", j, scr[script_id].groups[j].name, nextName);
							for (k = 0; k < scr[script_id].nbGroups; k++)
							  if (strcmp(scr[script_id].groups[k].name, previousName) == 0)
							  {
							    //Liaison de groups[k] vers groups[j]
							    linkCreated = 0;
							    l = 0;
							    //printf("j : %d, name : %s nbLinksTo : %d\n", j, scr[script_id].groups[j].name, scr[script_id].groups[j].nbLinksTo);
							    do
							    {
							      //printf("l = %d\n", l);
							      if (scr[script_id].groups[j].previous[l] == NULL) //Si cette case du tableau previous est libre
							      {
								scr[script_id].groups[j].previous[l] = &scr[script_id].groups[k];
								linkCreated = 1;
							      }
							      l++;
							    } while (linkCreated == 0 && l <= scr[script_id].groups[j].nbLinksTo);
							    //printf("Création d'une liaison de %s vers %s\n", scr[script_id].groups[k].name, scr[script_id].groups[j].name);
							  }
						      }
						    }  
						 }				  
						}  										
						break;

					case ENET_DEF_NEURON_CHANNEL:								
						printf("Réception des neurones du script %s\n", scr[script_id].name);
						number_of_neurons = (event.packet->dataLength) / sizeof(type_neurone);						
						
						received_neurons_packet = (type_neurone*) event.packet->data;
						//printf("Japet a reçu %d neurones.\n", number_of_neurons);
						
						int allocatedNeurons;
						
						//printf("Création des neurones du groupe %d du script %d\n", received_neurons_packet[0].groupe, script_id);
						//Création des neurones
						
						neuronGroupId = 0;
						
						for (i = 0; i < number_of_neurons; i++)
						{
						    //neuronGroupId = received_neurons_packet[i].groupe;
						    if (i > 0) 
						      if (received_neurons_packet[i].groupe != received_neurons_packet[i-1].groupe)
							neuronGroupId++;
						      
						    s = received_neurons_packet[i].s;
						    s1 = received_neurons_packet[i].s1;
						    s2 = received_neurons_packet[i].s2;	
						    neuronX = received_neurons_packet[i].posx;
						    neuronY = received_neurons_packet[i].posy;
						    
						    //if (received_neurons_packet[i].groupe != received_neurons_packet[i-1].groupe)
						      //printf("Création des neurones du groupe %d du script %d\n", received_neurons_packet[i].groupe, script_id);
						    /*
						    //Recherche du groupe dont ce neurone fait partie
						    for (j = 0; j < scr[script_id].nbGroups; j++)
						    {
						      if ((i >= scr[script_id].groups[j].firstNeuron) 
							&& (i < (scr[script_id].groups[j].firstNeuron + scr[script_id].groups[j].nbNeurons)))
							neuronGroupId = j;
						    }*/					    
						    
						  //  printf("groupe : %d %d\n", neuronGroupId, received_neurons_packet[i].groupe);
						    allocatedNeurons = scr[script_id].
							    groups[neuronGroupId].
								  allocatedNeurons;
						    
						    newNeuron(&scr[script_id].groups[neuronGroupId].neurons[allocatedNeurons],
							     &scr[script_id].groups[neuronGroupId], 
							     s, 
							     s1, 
							     s2, 
							     0, 
							     neuronX, 
							     neuronY);	
						    
						    //if (received_neurons_packet[i].groupe != received_neurons_packet[i-1].groupe)
						      //printf("Les neurones du groupe %d du script %d sont créés\n", received_neurons_packet[i-1].groupe, script_id);
							     
						  } 
						  //printf("Les neurones du groupe %d du script %d sont créés\n", neuronGroupId, script_id);
						  break;
						  
					case ENET_UPDATE_NEURON_CHANNEL:
					  
						//exit(0);
						
						//On commence par remettre à FALSE le champ justRefreshed de tous les groupes
						for (i = 0; i < nbScripts; i++)
						    for (j = 0; j < scr[i].nbGroups; j++)
							scr[i].groups[j].justRefreshed = FALSE;
						
						//Réception du paquet
						//printf("Réactualisation de neurones du script %s\n", scr[script_id].name);
						number_of_neurons = (event.packet->dataLength) / sizeof(type_neurone);						
						
						received_neurons_packet = (type_neurone*) event.packet->data;
						//printf("Japet a reçu les mises à jour pour %d neurones du groupe %d.\n", number_of_neurons, received_neurons_packet[0].groupe);
						
						//Mise à jour de chaque neurone renouvelé
						for (i = 0; i < scr[script_id].groups[neuronGroupId].nbNeurons; i++)
						{
						    neuronGroupId = received_neurons_packet[i].groupe;
						    s = received_neurons_packet[i].s * 100;
						    s1 = received_neurons_packet[i].s1 * 100;
						    s2 = received_neurons_packet[i].s2 * 100;	
						    //neuronX = received_neurons_packet[i].posy;
						    //neuronY = received_neurons_packet[i].posx;					    
						    
						    //updateNeuron(&scr[script_id].groups[neuronGroupId].neurons[i], s * 100, s1 * 100, s2 * 100, 0);							
						    
						    scr[script_id].groups[neuronGroupId].neurons[i].s[0] = s;
						    scr[script_id].groups[neuronGroupId].neurons[i].s[1] = s1;
						    scr[script_id].groups[neuronGroupId].neurons[i].s[2] = s2;
						    	    
						    
						    /*
						    //Mise à jour du neurone, en fonction de ses coordonnées
						    for (j = 0; j < scr[script_id].groups[neuronGroupId].nbNeurons; j++)
						      if (scr[script_id].groups[neuronGroupId].neurons[k].x == neuronX && scr[script_id].groups[neuronGroupId].neurons[j].y == neuronY) 
						      {
							updateNeuron(&scr[script_id].groups[neuronGroupId].neurons[j], s, s1, s2, 0);							
							break;
						      }*/
						      
						    //On indique que le groupe dont ce neurone fait partie vient d'être mis à jour
						    scr[script_id].groups[neuronGroupId].justRefreshed == TRUE;
						}			
						/*if (number_of_neurons < 10) printf("%f  %f  %f  %f  %f %d\n", 
						  received_neurons_packet[0].s,
						  received_neurons_packet[1].s,
						  received_neurons_packet[2].s,
						  received_neurons_packet[3].s,
						  received_neurons_packet[4].s,
						  received_neurons_packet[0].groupe);
						else
						 // printf("groupe de 100 neurones %d\n", received_neurons_packet[0].groupe);*/
						
						if (displayMode[1] == 'a' && nbSnapshots < NB_BUFFERED_MAX) nbSnapshots++; //Si on est en mode échantillonné, et si les tableaux "buffer" ne sont pas pleins,
															   //on note qu'on a un instantané de plus en mémoire.
						
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
