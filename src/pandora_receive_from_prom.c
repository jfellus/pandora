/**  pandora_receive_from_prom.c
 *
 *  @Author Arnaud Blanchard et Brice Errandonea
 *
 */

#include "pandora.h"
#include <gtk/gtk.h>
#include <enet/enet.h>
#include "prom_tools/include/basic_tools.h"
#include "net_message_debug_dist.h"
#include "prom_kernel/include/pandora_connect.h"
#include "prom_user/include/Struct/prom_images_struct.h"
#include "prom_kernel/include/reseau.h"

ENetHost* enet_server = NULL;

const char *displayMode;

void enet_manager(ENetHost *server); /* Sinon il faut mettre la fonction avant les appels */
int freePeer = 0; //Utilisé pour numéroter les peers (donc les promethes, donc les scripts) (ENet)

/**
 *
 * Crée dans Pandora un serveur, qui va écouter le réseau et accepter les connexions des Prométhés
 *
 */
void server_for_promethes()
{
  char host_name[HOST_NAME_MAX];
  ENetAddress address;
  pthread_t enet_thread;

  enet_time_set(0);

  address.host = ENET_HOST_ANY;
  address.port = PANDORA_PORT;

  enet_server = enet_host_create(&address, NB_SCRIPTS_MAX, PANDORA_NUMBER_OF_CHANNELS, ENET_UNLIMITED_BANDWITH, ENET_UNLIMITED_BANDWITH);

  if (enet_server != NULL)
  {
    gethostname(host_name, HOST_NAME_MAX);
    pthread_create(&enet_thread, NULL, (void*(*)(void*)) enet_manager, enet_server);
  }
  else EXIT_ON_ERROR("Fail to create a enet server for pandora !\n\tCheck that there is no other pandora running.");
}

void enet_manager(ENetHost *server)
{
  static int first_call = 1;
  char ip[HOST_NAME_MAX];
  int running = 1;
  int i;
  int script_id, group_id, link_id, net_link_id;

  int number_of_groups, number_of_links, number_of_neurons;
  ENetEvent event;

 // float time;

  //paquets reçus
  type_com_groupe* received_groups_packet;
  type_liaison* received_links_packet;

  //variables temporaires, pour analyser chaque liaison reçue
  int type;
  char *link_name, *option;

  prom_images_struct *prom_images;
  size_t image_size;
  enet_uint8 *current_data;

  size_t name_size, groups_size, links_size;
  type_script *script = NULL;
  type_group *group, *input_group;
  type_neurone *neurons;


  while (running)
  {
  first_call++;
    /* Wait up to 2000 milliseconds for an event. */
    while (enet_host_service(server, &event, 2000) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        enet_address_get_host_ip(&event.peer->address, ip, HOST_NAME_MAX);
        printf("A new client connected (ENet) from ip %s:%i.\n", ip, event.peer->address.port);

        script = ALLOCATION(type_script);
        script->id = number_of_scripts;
        script->color = script->id % COLOR_MAX;
        script->z = 0;
        script->y_offset = 0;
        script->height = 0;
        script->displayed = 0;
        script->peer = event.peer;
        script->groups = NULL;
        sem_init(&script->sem_groups_defined, 0, 0);
        scripts[number_of_scripts] = script;
        number_of_scripts++;
        event.peer->data = script;
        break;

      case ENET_EVENT_TYPE_RECEIVE:
        script = event.peer->data;
        current_data = event.packet->data;

        switch (event.channelID)
        {
        case ENET_PROMETHE_DESCRIPTION_CHANNEL: //Réception du nom du script
          memcpy(&name_size, current_data, sizeof(size_t));
          current_data=&current_data[sizeof(size_t)];
          strcpy(script->name, (char*) current_data);

          current_data=&current_data[name_size];
          memcpy(&groups_size, current_data, sizeof(size_t));
          current_data=&current_data[sizeof(size_t)];

          //Création du script
          number_of_groups = groups_size / sizeof(type_com_groupe);
          received_groups_packet = (type_com_groupe*)current_data;

          script->number_of_groups = number_of_groups;
          script->groups = MANY_ALLOCATIONS(number_of_groups, type_group);

          //Création des groupes
          for (i = 0; i < number_of_groups; i++)
          {
            group = &script->groups[i];
            group->id = i;
            group->script = script;
            strcpy(group->name, received_groups_packet[i].no_name);
            strcpy(group->function, received_groups_packet[i].nom);
            group->number_of_neurons = received_groups_packet[i].nbre;
            group->columns = received_groups_packet[i].taillex;
            group->rows = received_groups_packet[i].tailley;
            group->neurons = MANY_ALLOCATIONS(group->number_of_neurons, type_neurone);
            group->knownX = FALSE;
            group->knownY = FALSE;
            group->x = 1;
            group->y = 1;
            group->number_of_links = 0;
            group->val_min = 0;
            group->val_max = 1;
            group->output_display = 1; /* s1 */
            if (group->number_of_neurons == 1 ) group->display_mode = DISPLAY_MODE_TEXT;
            else  group->display_mode = DISPLAY_MODE_SQUARE;
            group->normalized = 0;
            group->previous = NULL;
            group->widget = NULL;
            group->ext = NULL;

            group->tabValues = NULL;
            group->indexAncien = NULL;
            group->indexDernier = NULL;
            group->afficher = NULL;
          }

          current_data=&current_data[groups_size];
          memcpy(&links_size, current_data, sizeof(size_t));
          current_data=&current_data[sizeof(size_t)];
          number_of_links = links_size / sizeof(type_liaison);
          received_links_packet = (type_liaison*)current_data;

          //Comptage du nombre de liaisons vers chaque groupe
          for (link_id = 0; link_id < number_of_links; link_id++)
          {
            type = 0;
            if ((received_links_packet[link_id].depart == -1) || (received_links_packet[link_id].arrivee == -1)) continue;

            input_group = &script->groups[received_links_packet[link_id].depart];
            group = &script->groups[received_links_packet[link_id].arrivee];

            if (strcmp(group->function, "f_send") == 0)
            {
              link_name = strtok(received_links_packet[link_id].nom, "-");
              option = strtok(NULL, "-");
              if (option != NULL)
              {
                if (strcmp(option, "ack") == 0) type = NET_LINK_ACK;
              }

              for (net_link_id = 0; net_link_id < number_of_net_links; net_link_id++)
              {
                if (strcmp(link_name, net_links[net_link_id].name) == 0)
                {
                  net_links[net_link_id].previous = group;
                  net_links[net_link_id].type |= type;

                  break;
                }
              }

              if (net_link_id == number_of_net_links) /* Ce nom de lien n'a pas ete trouvé */
              {
                strcpy(net_links[net_link_id].name, link_name);
                net_links[net_link_id].previous = group;
                net_links[net_link_id].next = NULL;
                net_links[net_link_id].type = type;

                number_of_net_links++;
              }
            }
            else if (strcmp(group->function, "f_recv") == 0)
            {
              link_name = strtok(received_links_packet[link_id].nom, "-");
              option = strtok(NULL, "-");
              if (option != NULL)
              {
                if (strcmp(option, "block") == 0) type = NET_LINK_BLOCK;
              }

              for (net_link_id = 0; net_link_id < number_of_net_links; net_link_id++)
              {
                if (strcmp(link_name, net_links[net_link_id].name) == 0)
                {
                  net_links[net_link_id].next = group;
                  net_links[net_link_id].type |= type;

                  break;
                }
              }
              if (net_link_id == number_of_net_links)
              {
                strcpy(net_links[net_link_id].name, link_name);
                net_links[net_link_id].previous = NULL;
                net_links[net_link_id].next = group;
                net_links[net_link_id].type = type;

                number_of_net_links++; /* Ce nom de lien n'a pas ete trouvé */

              }

            }

            if (!received_links_packet[link_id].secondaire) //Si c'est une liaison principale
            {
              group->number_of_links++;
              group->previous = MANY_REALLOCATIONS(group->previous, group->number_of_links, type_group*);
              group->previous[group->number_of_links - 1] = input_group;
            }
          }

          script->y_offset= 0;
          for (script_id = 0; script_id < number_of_scripts; script_id++)
          {
            if (scripts[script_id]->groups != NULL) script_update_positions(scripts[script_id]);
          }
          script_update_display(script);

          pthread_mutex_lock(&mutex_script_caracteristics);
          if((first_call<3) && load_temporary_save == TRUE && (access("./pandora.pandora", R_OK) == 0))
            pandora_file_load_script("./pandora.pandora", script);
          else if((first_call<3) && (access(preferences_filename, R_OK) == 0))
            pandora_file_load_script(preferences_filename, script);
          script_caracteristics(script, APPLY_SCRIPT_GROUPS_CARACTERISTICS);
          pthread_mutex_unlock(&mutex_script_caracteristics);
          break;

        case ENET_UPDATE_NEURON_CHANNEL:
          number_of_neurons = (event.packet->dataLength) / sizeof(type_neurone);
          neurons = (type_neurone*) event.packet->data;
          group_id = neurons->groupe;
          group = &script->groups[group_id];

          /* printf("RTT: %i\n", event.peer->lastRoundTripTime); */
          //Réception du paquet
          memcpy(group->neurons, event.packet->data, sizeof(type_neurone) * number_of_neurons);
          group->counter++;
          enet_packet_destroy(event.packet);

          pthread_mutex_lock(&mutex_script_caracteristics);
          if((refresh_mode == REFRESH_MODE_SEMI_AUTO || refresh_mode == REFRESH_MODE_MANUAL) && group->drawing_area != NULL && group->widget != NULL)
        	  group_expose_neurons(group, TRUE);
          pthread_mutex_unlock(&mutex_script_caracteristics);
/*
          time = g_timer_elapsed(group->timer, NULL);
          g_timer_start(group->timer);

          // calcul de la fréquence moyenne basée sur les FREQUENCE_MAX_VALUES_NUMBER dernières itérations.
            if(group->frequence_index_last == -1)
            {
                group->frequence_index_last = group->frequence_index_older = 0;
                group->frequence_values[0] = 1/time;
            }
            else if(group->frequence_index_older == group->frequence_index_last+1 || (group->frequence_index_last == FREQUENCE_MAX_VALUES_NUMBER-1 && group->frequence_index_older == 0))
            {
                if(group->frequence_index_last == FREQUENCE_MAX_VALUES_NUMBER-1)
                {
                    group->frequence_index_last = 0;
                    group->frequence_index_older = 1;
                }
                else
                {
                    group->frequence_index_last++;
                    group->frequence_index_older++;
                }
                group->frequence_values[group->frequence_index_last] = 1/time;
            }
            else
            {
                group->frequence_index_last++;
                group->frequence_values[group->frequence_index_last] = 1/time;
            }
*/
          break;

        case ENET_UPDATE_EXT_CHANNEL:
          group_id = *((int*)current_data);
          current_data = &current_data[sizeof(int)];

          group = &script->groups[group_id];

          if (group->ext == NULL) /*Th first time we allocate the data to receive images */
          {
            prom_images = ALLOCATION(prom_images_struct);
            memcpy(prom_images, current_data, sizeof(prom_images_struct));
            image_size = prom_images->sx * prom_images->sy * prom_images->nb_band * sizeof(unsigned char);
            for (i = 0; (unsigned int) i < prom_images->image_number; i++)
            {
              prom_images->images_table[i] = MANY_ALLOCATIONS(image_size, unsigned char);
            }
            group->ext = prom_images;
          }
          else
          {
            prom_images = (prom_images_struct*) group->ext;
            image_size = ((size_t)prom_images->sx * prom_images->sy * prom_images->nb_band) * sizeof(unsigned char);
          }
          current_data = &current_data[sizeof(prom_images_struct)];

          for (i = 0; (unsigned int) i < prom_images->image_number; i++)
          {
            memcpy(prom_images->images_table[i], current_data, image_size);
            current_data = &current_data[image_size];
          }

          /* Clean up the packet now that we're done using it. */
          group->counter++;
          enet_packet_destroy(event.packet);

          pthread_mutex_lock(&mutex_script_caracteristics);
          if((refresh_mode == REFRESH_MODE_SEMI_AUTO || refresh_mode == REFRESH_MODE_MANUAL) && group->drawing_area != NULL && group->widget != NULL)
        	  group_expose_neurons(group, TRUE);
          pthread_mutex_unlock(&mutex_script_caracteristics);
/*
          time = g_timer_elapsed(group->timer, NULL);
          g_timer_start(group->timer);

            if(group->frequence_index_last == -1)
            {
                group->frequence_index_last = group->frequence_index_older = 0;
                group->frequence_values[0] = 1/time;
            }
            else if(group->frequence_index_older == group->frequence_index_last+1 || (group->frequence_index_last == FREQUENCE_MAX_VALUES_NUMBER-1 && group->frequence_index_older == 0))
            {
                if(group->frequence_index_last == FREQUENCE_MAX_VALUES_NUMBER-1)
                {
                    group->frequence_index_last = 0;
                    group->frequence_index_older = 1;
                }
                else
                {
                    group->frequence_index_last++;
                    group->frequence_index_older++;
                }
                group->frequence_values[group->frequence_index_last] = 1/time;
            }
            else
            {
                group->frequence_index_last++;
                group->frequence_values[group->frequence_index_last] = 1/time;
            }
*/          break;
        }
        break;

      case ENET_EVENT_TYPE_DISCONNECT:
        script = event.peer->data;
        event.peer->data = NULL;
        break;

      case ENET_EVENT_TYPE_NONE:
        printf("ENET: none event \n");
        break;
      }
    }
  }
}

