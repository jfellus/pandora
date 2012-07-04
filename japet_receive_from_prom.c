/**  japet_receive_from_prom.c
 *  
 *  @Author Arnaud Blanchard et Brice Errandonea
 * 
 */

#include "japet.h"
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

const char* id = "japet"; /* Identifiant des promethes communiquants avec japet. Il faudra que ce soit passé en parametre sous la forme japet -i<identifiant>  en attendant c'est fixe il faut donc lancer promethe -ijapet ou themis -ijapet (voir main de themis.c)*/

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
	sem_post(&ivy_semaphore);
}

void ivyApplicationCallback(IvyClientPtr app, void *user_data, IvyApplicationEvent event)
{
	char *appname;
	char *host;

	(void) user_data;

	appname = IvyGetApplicationName(app);
	host = IvyGetApplicationHost(app);

	switch (event)
	{
	case IvyApplicationConnected:
		if (strncmp(appname, "themis:", 7) == 0) break; //On ne veut pas référencer Thémis comme étant un script
		strcpy(ivyServers[ivyServerNb].appName, appname);
		strcpy(ivyServers[ivyServerNb].ip, host);
		if (strcmp(ivyServers[ivyServerNb].ip, "localhost") == 0) strcpy(ivyServers[ivyServerNb].ip, "127.0.0.1");
		printf("Connexion ivy de %s d'adresse %s\n", ivyServers[ivyServerNb].appName, ivyServers[ivyServerNb].ip);
		ivyServerNb++;
		break;

	case IvyApplicationDisconnected:
		printf("%s disconnected (ivy) from %s\n", appname, host);
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

	snprintf(broadcast, BROADCAST_MAX, "%s:2010", brodcast_ip);

	gethostname(computer_name, HOST_NAME_MAX);
	my_pid = getpid();

	snprintf(ivy_prom_name, SIZE_OF_IVY_PROM_NAME, "japet:%s:%ld", computer_name, (long int) my_pid); /* Le nom unique du programme sur le bus ivy sera japet:<hostanem>:<pid du processus */

	sem_init(&ivy_semaphore, 0, 1); /* semaphore necessaire pour faire des appels ivy thread safes.*/
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
		error = pthread_create(&enet_thread, NULL, (void*(*)(void*)) enet_manager, enet_server);
	}
	else EXIT_ON_ERROR("Fail to create a enet server for japet !\n\tCheck that there is no other japet running.");
}

void enet_manager(ENetHost *server)
{
	gchar ip[HOST_NAME_MAX];
	int running = 1;
	int i, j, k, l;

	int number_of_groups, number_of_links, number_of_neurons;
	ENetEvent event;

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
	//char previousName[SIZE_NO_NAME];
	//char nextName[SIZE_NO_NAME];
	int previousNb;
	int nextNb;
	int linkCreated;
	int nbCreatedLinksForThisScript;

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
				event.peer->data = ALLOCATION(int);
				memcpy(event.peer->data, &nbScripts, sizeof(int));
				scr[nbScripts].peer = event.peer;
				scr[nbScripts].autorize_neurons_update = 0;
				nbScripts++;
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				script_id = *((int*) (event.peer->data));//Numéro du peer, et donc du script, qui lui a été attribué lors de sa connexion (voir cas précédent)
				switch (event.channelID)
				{
				case ENET_PROMETHE_DESCRIPTION_CHANNEL: //Réception du nom du script
					received_name_packet = (char*) event.packet->data;
					strncpy(scriptsNames[script_id], received_name_packet, event.packet->dataLength);
					scriptsNames[script_id][event.packet->dataLength] = '\0';
					break;

				case ENET_DEF_GROUP_CHANNEL: //Réception du tableau des groupes
					if (scr[script_id].z == -4) //Si ce script n'a pas encore été créé
					{
						//Création du script
						number_of_groups = (event.packet->dataLength) / sizeof(type_com_groupe);
						newScript(&scr[script_id], scriptsNames[script_id], ip, script_id, number_of_groups);
						received_groups_packet = (type_com_groupe*) event.packet->data;
						printf("Réception des %d groupes du script %s\n", number_of_groups, scr[script_id].name);

						//Création des groupes
						for (i = 0; i < number_of_groups; i++)
						{
							strncpy(groupName, received_groups_packet[i].no_name, SIZE_NO_NAME);
							groupY = received_groups_packet[i].posy;
							groupFirstNeuron = received_groups_packet[i].premier_ele;
							strncpy(groupType, received_groups_packet[i].nom, TAILLE_CHAINE);
							groupRows = received_groups_packet[i].tailley;
							groupColumns = received_groups_packet[i].taillex;
							groupNbNeurons = received_groups_packet[i].nbre;
							groupLearningSpeed = received_groups_packet[i].learning_rate;
							groupId = received_groups_packet[i].no;

							newGroup(&scr[script_id].groups[groupId], &scr[script_id], groupName, groupType, groupLearningSpeed, groupNbNeurons, groupRows, groupColumns, 1, //Remplacer par groupY si on veut utiliser dans Japet le y qu'avait ce groupe dans Coeos
							0, //On ne connait pas encore de liaison vers ce groupe
							groupFirstNeuron);
						}
					}
					break;

				case ENET_DEF_LINK_CHANNEL:
					number_of_links = (event.packet->dataLength) / sizeof(type_liaison);
					printf("Réception des %d liaisons du script %s\n", number_of_links, scr[script_id].name);
					received_links_packet = (type_liaison*) event.packet->data;

					nbCreatedLinksForThisScript = 0;

					//Comptage du nombre de liaisons vers chaque groupe
					for (i = 0; i < number_of_links; i++)
					{
						linkSecondary = received_links_packet[i].secondaire;
						//strncpy(previousName, received_links_packet[i].depart_name, SIZE_NO_NAME);
						//strncpy(nextName, received_links_packet[i].arrivee_name, SIZE_NO_NAME);
						previousNb = received_links_packet[i].depart;
						nextNb = received_links_packet[i].arrivee;
					//	if (linkSecondary == 0) scr[script_id].groups[received_links_packet[i].depart].nbLinksTo++;

						if (linkSecondary == 0) //Si c'est une liaison principale (pour l'instant, on ne tient pas compte des liaisons secondaires)
						{
							//On vérifie que les groupes au début et à la fin de la liaison sont bien dans le script n°script_id
							for (j = 0; j < scr[script_id].nbGroups; j++)
							{
								//printf("%s vs %s\n", scr[script_id].groups[j].name, nextName);
								for (k = 0; k < scr[script_id].nbGroups; k++)
									//if (strcmp(scr[script_id].groups[j].name, nextName) == 0 && strcmp(scr[script_id].groups[k].name, previousName) == 0)
									if(j == nextNb  && k == previousNb)
									{
										scr[script_id].groups[j].nbLinksTo++; //Il y a une liaison de plus qu'avant vers ce groupe
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
						//strncpy(previousName, received_links_packet[i].depart_name, SIZE_NO_NAME);
						//strncpy(nextName, received_links_packet[i].arrivee_name, SIZE_NO_NAME);
						previousNb = received_links_packet[i].depart;
						nextNb = received_links_packet[i].arrivee;

						//Si c'est une liaison principale (pour l'instant, on ne tient pas compte des liaisons secondaires)
						if (linkSecondary == 0)
						{
							linkCreated = 0;
							for (j = 0; j < scr[script_id].nbGroups; j++)
							{
								//if (strcmp(scr[script_id].groups[j].name, nextName) == 0)
								if(j == nextNb)
								{
									//printf("j = %d    name = %s    nextName = %s\n", j, scr[script_id].groups[j].name, nextName);
									for (k = 0; k < scr[script_id].nbGroups; k++)
										//if (strcmp(scr[script_id].groups[k].name, previousName) == 0)
										if(k == previousNb)
										{
											//Liaison de groups[k] vers groups[j]
											l = 0;
											//printf("j : %d, name : %s nbLinksTo : %d\n", j, scr[script_id].groups[j].name, scr[script_id].groups[j].nbLinksTo);
											do
											{
												//printf("l = %d\n", l);
												if (scr[script_id].groups[j].previous[l] == NULL) //Si cette case du tableau previous est libre
												{
													scr[script_id].groups[j].previous[l] = &scr[script_id].groups[k];
													linkCreated = 1;
													nbCreatedLinksForThisScript++;
												}
												l++;
											} while (linkCreated == 0 && l <= scr[script_id].groups[j].nbLinksTo);
											//printf("Création d'une liaison de %s vers %s\n", scr[script_id].groups[k].name, scr[script_id].groups[j].name);
										}
								}
							}
							//if (linkCreated == 0) printf("La liaison de %s vers %s a été ignorée car l'un des deux n'existe pas\n", previousName, nextName);
							if (linkCreated == 0) printf("La liaison de %d vers %d a été ignorée car l'un des deux n'existe pas\n", previousNb, nextNb);
						}
						//else printf("La liaison de %s vers %s a été ignorée car secondaire\n", previousName, nextName);
						else printf("La liaison de %d vers %d a été ignorée car secondaire\n", previousNb, nextNb);
					}
					printf("%d liaisons ont été créées pour le script %s\n", nbCreatedLinksForThisScript, scr[script_id].name);

					break;

				case ENET_DEF_NEURON_CHANNEL:
					printf("Réception des neurones du script %s\n", scr[script_id].name);
					number_of_neurons = (event.packet->dataLength) / sizeof(type_neurone);
					received_neurons_packet = (type_neurone*) event.packet->data;

					int allocatedNeurons;

					neuronGroupId = 0;

					for (i = 0; i < number_of_neurons; i++)
					{
						if (i > 0)
						{
							if (received_neurons_packet[i].groupe != received_neurons_packet[i - 1].groupe) neuronGroupId++;
						}

						s = received_neurons_packet[i].s;
						s1 = received_neurons_packet[i].s1;
						s2 = received_neurons_packet[i].s2;
						neuronX = received_neurons_packet[i].posx;
						neuronY = received_neurons_packet[i].posy;

						allocatedNeurons = scr[script_id].groups[neuronGroupId].allocatedNeurons;
						newNeuron(&scr[script_id].groups[neuronGroupId].neurons[allocatedNeurons], &scr[script_id].groups[neuronGroupId], s, s1, s2, 0, neuronX, neuronY);
					}
					update_script_display(script_id);
					scr[script_id].autorize_neurons_update = 1;
					break;

				case ENET_UPDATE_NEURON_CHANNEL:

					if(scr[script_id].autorize_neurons_update == 1)
					{
						//if(nb_update == gtk_spin_button_get_value(GTK_SPIN_BUTTON(refreshScale)))
						{
							//On commence par remettre à FALSE le champ justRefreshed de tous les groupes

							for (j = 0; j < scr[script_id].nbGroups; j++)
								scr[script_id].groups[j].justRefreshed = FALSE;

							//Réception du paquet
							number_of_neurons = (event.packet->dataLength) / sizeof(type_neurone);
							received_neurons_packet = (type_neurone*) event.packet->data;

							//Mise à jour de chaque neurone renouvelé
							for (j = 0; j < scr[script_id].groups[neuronGroupId].nbNeurons; j++)
							{
								neuronGroupId = received_neurons_packet[j].groupe;
								s = received_neurons_packet[j].s * 100;
								s1 = received_neurons_packet[j].s1 * 100;
								s2 = received_neurons_packet[j].s2 * 100;

								scr[script_id].groups[neuronGroupId].neurons[j].s[0] = s;
								scr[script_id].groups[neuronGroupId].neurons[j].s[1] = s1;
								scr[script_id].groups[neuronGroupId].neurons[j].s[2] = s2;

								//On indique que le groupe dont ce neurone fait partie vient d'être mis à jour
								scr[script_id].groups[neuronGroupId].justRefreshed = TRUE;


							}

							if (displayMode[1] == 'a' && nbSnapshots < NB_BUFFERED_MAX) nbSnapshots++; //Si on est en mode échantillonné, et si les tableaux "buffer" ne sont pas pleins,
							//on note qu'on a un instantané de plus en mémoire.
							//update_neurons_display(script_id, neuronGroupId);
							//nb_update = 0;
						}
						//nb_update++;
					}
				}
				/* Clean up the packet now that we're done using it. */
				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				script_id = *((int*) (event.peer->data));
				printf("%s disconnected.\n", scr[script_id].name);
				/* Reset the peer's client information. */
				free(event.peer->data);
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
