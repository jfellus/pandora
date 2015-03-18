/*
 Copyright  ETIS — ENSEA, Université de Cergy-Pontoise, CNRS (1991-2014)
 promethe@ensea.fr

 Authors: P. Andry, J.C. Baccon, D. Bailly, A. Blanchard, S. Boucena, A. Chatty, N. Cuperlier, P. Delarboulas, P. Gaussier,
 C. Giovannangeli, C. Grand, L. Hafemeister, C. Hasson, S.K. Hasnain, S. Hanoune, J. Hirel, A. Jauffret, C. Joulain, A. Karaouzène,
 M. Lagarde, S. Leprêtre, M. Maillard, B. Miramond, S. Moga, G. Mostafaoui, A. Pitti, K. Prepin, M. Quoy, A. de Rengervé, A. Revel ...

 See more details and updates in the file AUTHORS

 This software is a computer program whose purpose is to simulate neural networks and control robots or simulations.
 This software is governed by the CeCILL v2.1 license under French law and abiding by the rules of distribution of free software.
 You can use, modify and/ or redistribute the software under the terms of the CeCILL v2.1 license as circulated by CEA, CNRS and INRIA at the following URL "http://www.cecill.info".
 As a counterpart to the access to the source code and  rights to copy, modify and redistribute granted by the license,
 users are provided only with a limited warranty and the software's author, the holder of the economic rights,  and the successive licensors have only limited liability.
 In this respect, the user's attention is drawn to the risks associated with loading, using, modifying and/or developing or reproducing the software by the user in light of its specific status of free software,
 that may mean  that it is complicated to manipulate, and that also therefore means that it is reserved for developers and experienced professionals having in-depth computer knowledge.
 Users are therefore encouraged to load and test the software's suitability as regards their requirements in conditions enabling the security of their systems and/or data to be ensured
 and, more generally, to use and operate it in the same conditions as regards security.
 The fact that you are presently reading this means that you have had knowledge of the CeCILL v2.1 license and that you accept its terms.
 */
/**  pandora_receive_from_prom.c
 *
 *  @Author Arnaud Blanchard et Brice Errandonea
 *
 **/

#include "pandora_receive_from_prom.h"
#include "pandora_graphic.h"
#include "pandora_save.h"
#include "pandora_prompt.h"
#include "pandora_architecture.h"

/* Variable Globales pour ce fichier*/
extern gboolean saving_press;
extern type_group *groups_to_display[];
pthread_t enet_thread;
//pthread_attr_t custom_sched_attr;

pthread_attr_t custom_sched_attr;
int fifo_max_prio, fifo_min_prio;
struct sched_param fifo_param;
enet_uint16 port_pando;

void recherche_vrai_nom(const char* nom, char* nom_gene, int taille)
{
  char PID[50];
  char* nom_machine;

  nom_machine=MANY_ALLOCATIONS(taille,char);
  sscanf(nom, "%[^':']:%[^':']:%[^':']", nom_machine, PID, nom_gene);
}

/**
 * Crée dans Pandora un serveur, qui va écouter le réseau et accepter les connexions des Prométhés
 */
//Créé le serveur pour enet à partir des informations donnée par Ivy, crée ensuite le thread qui va écouter.
void server_for_promethes()
{
  ENetHost* enet_server = NULL;
  char host_name[HOST_NAME_MAX];
  ENetAddress address;
  enet_time_set(0);
  int try=0;

  address.host = ENET_HOST_ANY;
 // address.port = PANDORA_PORT;


  while (enet_server == NULL && try<10)
  {
    address.port = PANDORA_PORT+try;
    enet_server = enet_host_create(&address, NB_SCRIPTS_MAX, ENET_NUMBER_OF_CHANNELS, ENET_UNLIMITED_BANDWITH, ENET_UNLIMITED_BANDWITH);

    gethostname(host_name, HOST_NAME_MAX);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compress_button))) enet_host_compress_with_range_coder(enet_server);
    // struct sched_param is used to store the scheduling priority

    // We'll set the priority to the maximum.

    try++;
   }
    pthread_attr_init(&custom_sched_attr);
    pthread_attr_setinheritsched(&custom_sched_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&custom_sched_attr, SCHED_BATCH);
    fifo_max_prio = sched_get_priority_max(SCHED_BATCH);
    fifo_min_prio = sched_get_priority_min(SCHED_BATCH);
    fifo_param.sched_priority = fifo_max_prio;
    pthread_attr_setschedparam(&custom_sched_attr, &fifo_param);
    //pthread_create(&(threads[i]), &custom_sched_attr, ....);

    //params.sched_priority = sched_get_priority_max(SCHED_FIFO);

    pthread_create(&enet_thread, &custom_sched_attr, (void*(*)(void*)) enet_manager, enet_server);
    pthread_setschedparam(enet_thread, SCHED_BATCH, &fifo_param);

  if(enet_server == NULL)
  {
    EXIT_ON_ERROR("Fail to create a enet server for pandora !\n\tCheck that there is no more than 10 pandora running.\n");
    return;
  }
  else
  {
    port_pando=address.port;
  }
}

//Permet de gerer l'appel à gtk_widget_queue_draw en dehors d'enet_manager.
gboolean queue_draw(gpointer data)
{
  type_group* group = (type_group*) data;
  int i = group->idDisplay;
  gtk_widget_queue_draw(GTK_WIDGET(groups_to_display[i]->drawing_area));

  return FALSE;
}

int prom_getopt_float2(const char *args, const char *option, float *value)
{
  char const *current_addr;
  for (current_addr = strstr(args, option); current_addr != NULL; current_addr = strstr(current_addr, option))
  {
    current_addr += strlen(option);
    if (sscanf(current_addr, "%f", value) == 1)
    {
      return 2; // This is followed by a number nor a new argument.
    }
    if (current_addr[0] == '-') return 1;
  }
  return 0;
}

int enet_host_secure(ENetHost * host, ENetEvent * event, enet_uint32 timeout)
{
  int retour;
  sem_wait(&(enet_pandora_lock));
  retour = enet_host_service(host, event, timeout);
  sem_post(&(enet_pandora_lock));
  return retour;
}
// Thread créé par server_for_promethe, permet d'écouter constemment et de dispatché les informations reçu selon leur nature.
void enet_manager(ENetHost *server)
{
  static int first_call = 1;
  char ip[HOST_NAME_MAX];
  int running = 1;
  int i, j, k;
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
  int no_neuro;
  int number_of_neuro_links;
  long time;
  int phase;
  float min_default = 0.f, max_default = 200.f, step_default = 0.01f;
  char param_link[256];
  while (running)
  {
    first_call++;

    while (enet_host_secure(server, &event, 0) > 0)
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
        script->control_group = NULL;
        script->height = 0;
        script->displayed = 0;
        script->peer = event.peer; //enet_host_connect(server, &(event.peer->address), PANDORA_NUMBER_OF_CHANNELS, 0);
        script->groups = NULL;
        script->pWindow=NULL;
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
          current_data = &current_data[sizeof(size_t)];
          strcpy(script->name, (char*) current_data);
          recherche_vrai_nom(script->name, script->nom_gene, LOGICAL_NAME_MAX);

          current_data = &current_data[name_size];
          memcpy(&groups_size, current_data, sizeof(size_t));
          current_data = &current_data[sizeof(size_t)];

          //Création du script
          number_of_groups = groups_size / sizeof(type_com_groupe);
          received_groups_packet = (type_com_groupe*) current_data;

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
            group->param_neuro_pandora = MANY_ALLOCATIONS(group->number_of_neurons, type_para_neuro);
            for (j = 0; j < group->number_of_neurons; j++)
            {
              group->param_neuro_pandora[j].links_to_draw = NULL;
              group->param_neuro_pandora[j].selected = FALSE;
              group->param_neuro_pandora[j].links_ok = FALSE;
              group->param_neuro_pandora[j].center_x = 0;
              group->param_neuro_pandora[j].center_y = 0;
              group->param_neuro_pandora[j].coeff = NULL;
            }
            group->knownX = FALSE;
            group->knownY = FALSE;
            group->x = 1;
            group->calculate_x = FALSE;
            group->is_in_a_loop = FALSE;
            group->is_currently_in_a_loop = FALSE;
            group->y = 1;
            group->number_of_links = 0;
            group->number_of_links_second = 0;
            group->val_min = 0;
            group->val_max = 1;
            group->output_display = 1; /* s1 */
            if (group->number_of_neurons == 1) group->display_mode = DISPLAY_MODE_TEXT;
            else group->display_mode = DISPLAY_MODE_SQUARE;
            group->normalized = 0;
            group->previous = NULL;
            group->previous_second = NULL;
            group->widget = NULL;
            group->ext = NULL;
            group->image_selected_index = 0;

            group->tabValues = NULL;
            group->indexAncien = NULL;
            group->indexDernier = NULL;
            group->afficher = NULL;
            group->associated_file = NULL;
            group->selected_for_save = FALSE;
            group->on_saving = FALSE;

            group->courbes = NULL;

            group->stats.last_time_phase_0 = -1;
            group->stats.last_time_phase_1 = 0;
            group->stats.last_time_phase_3 = 0;
            group->stats.initialiser = TRUE;
            group->stats.message[0] = '\0';
            group->stats.nb_executions_tot = 0;
            group->stats.somme_tot = 0;
            group->stats.somme_temps_executions = 0;
            group->ok = TRUE;
            group->drawing_area = NULL;
            group->ok_display = FALSE;
            group->idDisplay = 0;
            group->is_watch = FALSE;
            group->from_file = FALSE;
            group->neurons_height = 20;
            group->neurons_width = 20;
            group->firstNeuron = received_groups_packet[i].premier_ele;
            group->neuro_select = -1;
            group->type_control = -1;
            group->borne_max = max_default;
            group->borne_min = min_default;
            group->step = step_default;
            group->init = 0;
          }

          current_data = &current_data[groups_size];
          memcpy(&links_size, current_data, sizeof(size_t));
          current_data = &current_data[sizeof(size_t)];
          number_of_links = links_size / sizeof(type_liaison);
          received_links_packet = (type_liaison*) current_data;

          //Comptage du nombre de liaisons vers chaque groupe
          for (link_id = 0; link_id < number_of_links; link_id++)
          {
            type = 0;
            if ((received_links_packet[link_id].depart == -1) || (received_links_packet[link_id].arrivee == -1)) continue;
            if ((received_links_packet[link_id].depart > script->number_of_groups) || (received_links_packet[link_id].arrivee > script->number_of_groups)) continue; //sécurité

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

            if (strcmp(group->function, liste_controle_associee[VUE_METRE]) == 0)
            {
              if (prom_getopt_float(received_links_packet[link_id].nom, "-m", &(group->borne_min)) != 2) group->borne_min = min_default;
              if (prom_getopt_float(received_links_packet[link_id].nom, "-M", &(group->borne_max)) != 2) group->borne_max = max_default;
              if (prom_getopt_float(received_links_packet[link_id].nom, "-s", &(group->step)) != 2) group->step = step_default;
              if (prom_getopt_float(received_links_packet[link_id].nom, "-i", &(group->init)) != 2) group->init = 0;
              else
              {
                for (k = 0; k < group->number_of_neurons; k++)
                {
                  group->neurons[k].s = group->init;
                  group->neurons[k].s1 = group->init;
                  group->neurons[k].s2 = group->init;
                }
              }
              if (prom_getopt(received_links_packet[link_id].nom, "-n", param_link) == 2) strcpy(group->name_n, param_link);
              pandora_bus_send_message(bus_id, "pandora(%d,%d,%d) %s", PANDORA_SEND_NEURONS_ONE, group->id,0, group->script->name + strlen(bus_id) + 1);

            }
            if (strcmp(group->function, liste_controle_associee[CHECKBOX]) == 0)
            {
              if (prom_getopt_float(received_links_packet[link_id].nom, "-i", &(group->init)) != 2) group->init = 0;
              else
              {
                for (k = 0; k < group->number_of_neurons; k++)
                {
                  group->neurons[k].s = group->init;
                  group->neurons[k].s1 = group->init;
                  group->neurons[k].s2 = group->init;
                }
              }
              if (prom_getopt(received_links_packet[link_id].nom, "-n", param_link) == 2) strcpy(group->name_n, param_link);
              pandora_bus_send_message(bus_id, "pandora(%d,%d,%d) %s", PANDORA_SEND_NEURONS_ONE, group->id,0, group->script->name + strlen(bus_id) + 1);
            }

            if (!received_links_packet[link_id].secondaire) //Si c'est une liaison principale
            {
              group->number_of_links++;
              MANY_REALLOCATIONS(&group->previous, group->number_of_links);
              group->previous[group->number_of_links - 1] = input_group;
            }
            else //Si c'est une liaison secondaire
            {
              group->number_of_links_second++;
              MANY_REALLOCATIONS(&group->previous_second, group->number_of_links_second);
              group->previous_second[group->number_of_links_second - 1] = input_group;
            }
          }

          script->y_offset = 0;
          verify_script(script);
          for (script_id = 0; script_id < number_of_scripts; script_id++)
          {
            if (scripts[script_id]->groups != NULL) script_update_positions(scripts[script_id]);
          }
          script_update_display(script);


          if (access(preferences_filename, R_OK) == 0)
            {
              pandora_file_load_script(preferences_filename, script);
            }
          else if((load_temporary_save == TRUE && (access("./pandora.pandora", R_OK) == 0)))
          {
            pandora_file_load_script("./pandora.pandora", script);
          }
          script_caracteristics(scripts[script->id], APPLY_SCRIPT_GROUPS_CARACTERISTICS);
          enet_packet_destroy(event.packet);

        //  pandora_load_preferences(,preferences_filename);
          break;

        case ENET_UPDATE_NEURON_CHANNEL:
          number_of_neurons = (event.packet->dataLength) / sizeof(type_neurone);
          neurons = (type_neurone*) event.packet->data;
          group_id = neurons->groupe;

          if (script == NULL) break;
          if (script->groups == NULL || script->groups == 0x0) break; // securité
          if (script->groups[group_id].ok != TRUE) break; //sécurité 2

          group = &script->groups[group_id];

          //Réception du paquet
          memcpy(group->neurons, event.packet->data, sizeof(type_neurone) * number_of_neurons);

          if (group->selected_for_save == 1 && saving_press == 1)
          {

            continuous_saving(group);

          }
          group->counter++;
          group->refresh_freq = TRUE;

          /*
           if ((refresh_mode == REFRESH_MODE_MANUAL) && (group->drawing_area != NULL) && (group->widget != NULL) && (group->ok == TRUE) && (group->ok_display == TRUE))
           {
           group->refresh_freq = TRUE;
           g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc) queue_draw, (gpointer) group, NULL);

           }
           */
          enet_packet_destroy(event.packet);
          break;

        case ENET_UPDATE_EXT_CHANNEL:

          group_id = *((int*) current_data);
          current_data = &current_data[sizeof(int)];

          if (script == NULL) break;
          if (script->groups == NULL || script->groups == 0x0) break; /* securité */
          group = &script->groups[group_id];
          if (group->ok != TRUE) break; /*sécurité*/

          if (group->ext == NULL) /*The first time we allocate the data to receive images*/
          {
            prom_images = ALLOCATION(prom_images_struct);
            memcpy(prom_images, current_data, sizeof(prom_images_struct));
            image_size = prom_images->sx * prom_images->sy * prom_images->nb_band * sizeof(unsigned char);
            for (i = 0; i < prom_images->image_number; i++)
            {
              prom_images->images_table[i] = MANY_ALLOCATIONS(image_size, unsigned char);
            }
            group->ext = prom_images;
          }

          else
          {
            prom_images = (prom_images_struct*) group->ext;
            image_size = ((size_t) prom_images->sx * prom_images->sy * prom_images->nb_band) * sizeof(unsigned char);
          }
          current_data = &current_data[sizeof(prom_images_struct)];

          for (i = 0; i < prom_images->image_number; i++)
          {
            memcpy(prom_images->images_table[i], current_data, image_size);
            current_data = &current_data[image_size];
          }

          group->counter++;
          group->refresh_freq = TRUE;

          /*
           if ((refresh_mode == REFRESH_MODE_MANUAL) && (group->drawing_area != NULL) && (group->widget != NULL) && (group->ok == TRUE) && (group->ok_display == TRUE))
           {
           group->refresh_freq = TRUE;

           g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc) queue_draw, (gpointer) group, NULL);
           }
           */

          enet_packet_destroy(event.packet);
          break;

        case ENET_UPDATE_PHASES_INFO_CHANNEL:
          current_data = event.packet->data;
          group_id = *((int *) current_data);

          //printf("ordre de reception\n");
          if (script == NULL) break;
          if (script->groups == NULL || script->groups == 0x0) break; // sécurité
          group = &script->groups[group_id];
          if (group->ok != TRUE) break; //sécurité 2

          //printf("passage des sécurités\n");
          current_data = &current_data[sizeof(int)];
          time = *((long *) current_data);
          current_data = &current_data[sizeof(long)];
          phase = *((int *) current_data);

          if (group->stats.initialiser == TRUE)
          {
            group->stats.first_time = time;
            group->stats.nb_executions = 0;
            group->stats.somme_temps_executions = 0;
            group->stats.initialiser = FALSE;
          }

          if (phase == 0)
          {
            group->stats.last_time_phase_0 = time;
          }
          if ((phase == 3 || phase == 1) && group->stats.last_time_phase_0 != -1)
          {
            group->stats.nb_executions++;
            group->stats.nb_executions_tot++;
            group->stats.last_time_phase_3 = time;
            group->stats.somme_temps_executions += diff(group->stats.last_time_phase_0, group->stats.last_time_phase_3);
            group->stats.somme_tot += diff(group->stats.last_time_phase_0, group->stats.last_time_phase_3);
            // printf("comptage !\n");
          }
          if (diff(group->stats.first_time, time) > 2000000)
          {
            // affichage
            group->stats.initialiser = TRUE;
            sprintf(group->stats.message, "%ld", (group->stats.nb_executions > 0 ? group->stats.somme_temps_executions / group->stats.nb_executions : 0));

            g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc) queue_draw_archi, NULL, NULL);
            g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc) send_info_to_top, (gpointer) (&(scripts[script->id]->groups[group_id])), NULL);
          }

          enet_packet_destroy(event.packet);

          break;
        case ENET_UPDATE_LINKS:
          current_data = event.packet->data;
          number_of_neuro_links = (event.packet->dataLength - sizeof(int)) / (sizeof(type_coeff));
          no_neuro = *((int*) current_data);
          current_data = &(current_data[sizeof(int)]);

          // coeffs = (type_coeff*)current_data;
          //coeffs=links->coeffs;

          if (script == NULL) break;
          if (script->groups == NULL || script->groups == 0x0) break; // securité

          group = search_associated_group(no_neuro, script);

          if (group == NULL) break;
          if (group->ok != TRUE) break;

          if (number_of_neuro_links) create_links(group, no_neuro, current_data, number_of_neuro_links, script);

          enet_packet_destroy(event.packet);
          break;
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
      sem_wait(&(enet_pandora_lock));
      enet_host_flush(server);
      enet_host_flush(server);
      enet_host_flush(server);
      enet_host_flush(server);
      enet_host_flush(server);
      enet_host_flush(server);
      sem_post(&(enet_pandora_lock));
    }
  }
}

void sort_list_groups_by_rate(type_group **groups, int number_of_groups) // utilisation de l'algorithme de tri rapide. // TODO A supprimer ou a adapter dans la fonction top de pandora_prompt.
{
  int i = 1, j = number_of_groups - 1;
  type_group *tmp;
  if (number_of_groups < 2) return;

  while (i < j)
  {
    for (; i < j && !(groups[i]->stats.somme_temps_executions > groups[0]->stats.somme_temps_executions); i++)
      ;
    for (; i < j && !(groups[i]->stats.somme_temps_executions < groups[0]->stats.somme_temps_executions); j--)
      ;
    if (i < j)
    permut(groups[i], groups[j], tmp);
  }
  if (i == j && groups[i]->stats.somme_temps_executions > groups[0]->stats.somme_temps_executions) i--;
  if (i > 0)
  permut(groups[0], groups[i], tmp);

  if (i > 0) sort_list_groups_by_rate(groups, i);
  if (i < number_of_groups - 1) sort_list_groups_by_rate(&groups[i + 1], number_of_groups - i - 1);
}

void verify_script(type_script *script)
{
  int i;

  for (i = 0; i < script->number_of_groups; i++)
    if (script->groups[i].knownX == FALSE) verify_group(&script->groups[i]);
  for (i = 0; i < script->number_of_groups; i++)
    script->groups[i].knownX = FALSE;
}

void verify_group(type_group *group)
{
  static type_group *first_in_loop[50];
  static int loop_number = 0;
  int i;

  if (group->calculate_x == TRUE)
  {
    if (loop_number == 0 || first_in_loop[loop_number - 1] != group)
    {
      first_in_loop[loop_number] = group;
      loop_number++;
    }
    group->is_in_a_loop = TRUE;
    group->is_currently_in_a_loop = TRUE;
    return;
  }
  group->calculate_x = TRUE;
  if (group->previous == NULL)
  {
    group->x = 1;
  }
  else
  {
    for (i = 0; i < group->number_of_links; i++)
    {
      if (group->previous[i]->knownX == FALSE)
      {
        verify_group(group->previous[i]);
        if (group->previous[i]->is_currently_in_a_loop)
        {
          group->is_currently_in_a_loop = group->is_in_a_loop = TRUE;
          group->previous[i]->is_currently_in_a_loop = FALSE;
          if (loop_number > 0 && first_in_loop[loop_number - 1] == group)
          {
            group->is_currently_in_a_loop = FALSE;
            loop_number--;
          }
        }
      }
      if (group->previous[i]->x >= group->x) group->x = group->previous[i]->x + 1;
    }

    for (i = 0; i < number_of_net_links; i++)
    {
      if (net_links[i].next == group && net_links[i].previous != NULL)
      {
        if (net_links[i].type & NET_LINK_BLOCK)
        {
          if (!net_links[i].previous->knownX)
          {
            verify_group(net_links[i].previous);
            if (net_links[i].previous->is_currently_in_a_loop)
            {
              group->is_currently_in_a_loop = group->is_in_a_loop = TRUE;
              net_links[i].previous->is_currently_in_a_loop = FALSE;
              if (loop_number > 0 && first_in_loop[loop_number - 1] == group)
              {
                group->is_currently_in_a_loop = FALSE;
                loop_number--;
              }
            }
          }
          if (net_links[i].next->x < net_links[i].previous->x) net_links[i].next->x = net_links[i].previous->x + 1;
        }
      }
    }
  }
  group->calculate_x = FALSE;

  group->knownX = TRUE;
}

// Déduit d'une neuronne envoyée et des groupes en présence le groupe auquel cette neuronne appartient dans pandora.
type_group* search_associated_group(int no_neuro, type_script* script)
{
  int i = 0;
  for (i = 0; i < script->number_of_groups; i++)
  {
    if (no_neuro >= (script->groups[i].firstNeuron) && no_neuro < (script->groups[i].firstNeuron + script->groups[i].number_of_neurons))
    {
      return &(script->groups[i]);
    }
  }
  return NULL;

}

// Crée les liens neuronnes à neuronnes quand necessaire.
void create_links(type_group *group, int no_neuro, enet_uint8 *current_data, int number_of_neuro_links, type_script* script)
{
  int no_neuro_rel;
  int i;
  type_coeff* coeffs = NULL;

  no_neuro_rel = no_neuro - (group->firstNeuron);
  if (!(group->param_neuro_pandora[no_neuro_rel].links_ok))
  {
    MANY_REALLOCATIONS(&group->param_neuro_pandora[no_neuro_rel].coeff, number_of_neuro_links);
    // group->neurons[no_neuro_rel].coeff=MANY_REALLOCATIONS(group->neurons[no_neuro_rel].coeff,number_of_neuro_links,type_coeff);
    MANY_REALLOCATIONS(&group->param_neuro_pandora[no_neuro_rel].links_to_draw, number_of_neuro_links);
    group->param_neuro_pandora[no_neuro_rel].links_ok = TRUE;
  }

  memcpy(group->param_neuro_pandora[no_neuro_rel].coeff, current_data, number_of_neuro_links * sizeof(type_coeff));
  coeffs = group->param_neuro_pandora[no_neuro_rel].coeff;

  for (i = 0; i < number_of_neuro_links; i++)
  {
    group->param_neuro_pandora[no_neuro_rel].links_to_draw[i].group_pointed = search_associated_group(coeffs[i].entree, script);
    group->param_neuro_pandora[no_neuro_rel].links_to_draw[i].no_neuro_rel_pointed = (coeffs[i].entree - group->param_neuro_pandora[no_neuro_rel].links_to_draw[i].group_pointed->firstNeuron);
  }

}
