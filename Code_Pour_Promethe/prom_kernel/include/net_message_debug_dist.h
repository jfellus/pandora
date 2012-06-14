/**
   \defgroup net_message_debug_dist net_message_debug_dist.h
   \ingroup generic_tools

   \details

   definition des messages echanges sur le reseau pour le debug a distance de promethe.
   dialogue entre prom_kernel et dist_prom. P. Gaussier Jan. 2008  

   \file
   \ingroup net_message_debug_dist

   Author: Arnaud Blanchard
   Created: 7/8/2009

   Fourni des defines, des macros et des fonctions de bases securisees. Utilisable aussi bien par hardhare que user.
*/

#ifndef NET_MESSAGE_DEBUG_DIST
#define NET_MESSAGE_DEBUG_DIST

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include <dlfcn.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <ivy.h>
#include <basic_tools.h>
#include <semaphore.h>


#define LISTENPORT       7500
#define BACKLOG          10

#define EXT         0
#define VIDEO_EXT   1
#define AUDIO_EXT   2
#define TACTILE_EXT 3

enum {
  DISTANT_DEBUG_START = 0,
  DISTANT_DEBUG_STOP
};


/** Describe status of promethe.  

The jump in the values (>10) : to separate when promethe is connected
from when no promethe communicates.
Needed when determining if telnet should reconnect or not. Cf. ivy_here_callback in themis_ivy_cb.c*/
enum prom_status{
/** From No_Not_Running to up to No_Undef : promethe is connected */
  No_Not_Running = 0, 
  No_Use_Only=1,
  No_Learn_and_Use=2,
  No_Continue_Learning=3,
  No_Continue_Using=4,

/** No_Undef and after : promethe is not connected */
  No_Undef=10,
  No_Quit=11,
  /** if terminal failed to launch promethe */
  No_Error=12, 

/**ATTENTION : Doit toujours etre superieur au max des valeurs possibles !! */
  NUMBER_OF_POSSIBLE_STATES=20
};


enum {
	ENET_PROMETHE_DESCRIPTION_CHANNEL = 0,
	ENET_DEF_GROUP_CHANNEL,
	ENET_GROUP_EVENT_CHANNEL,
	ENET_COMMAND_CHANNEL,
	ENET_DEF_LINK_CHANNEL,
	ENET_DEF_NEURON_CHANNEL,
	ENET_UPDATE_NEURON_CHANNEL,
	ENET_NUMBER_OF_CHANNELS /*Doit toujours etre en dernier */
};

enum {
	ENET_COMMAND_STOP_OSCILLO = 0,
	ENET_COMMAND_START_OSCILLO
};

#define ENET_UNLIMITED_BANDWITH 0

/* static const char *message_of_states[NUMBER_OF_POSSIBLE_STATES]; */


#define NB_MAX_ARG 20

/** Delai d'inactivation du thread_server en attente de rupture*/
#define DUREE_SLEEP_SURVEILLANCE_SECONDE 2
/* socket debug print */

#define SUCCESS 1
#define FAILURE 0

#define MAXIMUM_SIZE_OF_FILENAME 128
/*-----------------------------------------------------------------------------------------*/


#define KERNEL_SERVER_PORT   10000
#define CONSOLE_SERVER_PORT  30000
#define DEBUG_SERVER_PORT    20000

#define MAXBUF          1024
#define PARAM_MAX 			256
#define LOGICAL_NAME_MAX 				64



extern char debug_buffer[MAXBUF];
extern int socket_debug;
extern int client_debug;

extern char kernel_buffer[MAXBUF];
extern int socket_kernel;
extern int client_kernel;

extern char console_buffer[MAXBUF];
extern int socket_console;
extern int client_console;

/** Flag to specify that all the interactions are through telnet */
extern int distant_terminal; 
extern int oscillo_dist_activated;

extern char virtual_name[LOGICAL_NAME_MAX];

enum io_server_socket_state {ACTIVATED, CLOSED, SUSPENDED};

#define PRINT_SYSTEM_ERROR() print_warning(__FILE__, __FUNCTION__, __LINE__, strerror(errno))

#define ALLOCATION(type) (type*)secure_malloc(__FILE__, __FUNCTION__,__LINE__, sizeof(type))
#define MANY_ALLOCATIONS(numbers, type) (type*)secure_malloc(__FILE__,  __FUNCTION__, __LINE__, numbers*sizeof(type))
#define REALLOCATION(pointer, type) (type*)secure_realloc(__FILE__,  __FUNCTION__, __LINE__, pointer, sizeof(type))
#define MANY_REALLOCATIONS(pointer, numbers, type) (type*)secure_realloc(__FILE__,  __FUNCTION__, __LINE__, pointer, numbers*sizeof(type))

/** Implemented in basic_tools (->prom_tools ?)
    Envoie un message d'erreur avec name_of_file, name_of_function, number_of_line et affiche le message formate avec les parametres variables.
    Puis exit le programme avec le parametre EXIT_FAILURE.*/
extern  void fatal_error(const char *name_of_file, const char* name_of_function,  int numero_of_line, const char *message, ...);

/**
   Envoie un message de warning avec name_of_file, name_of_function, number_of_line et affiche le message formate avec les parametres variables.
*/
extern  void print_warning(const char *name_of_file, const char* name_of_function,  int numero_of_line, const char *message, ...);

typedef struct statistic
{
  unsigned int size;
  float mean, min, max, sigma2;
}Statistic;

extern void* load_libraryf(const char *format, ...);


static inline void __attribute__((always_inline)) link_function_with_library(void *pointer_to_function, void *lib_handle, const char* name_of_function)
{
  void *symbol;

  symbol = dlsym(lib_handle, name_of_function);

  if (symbol == NULL) pointer_to_function = NULL;
  else memcpy(pointer_to_function, &symbol, sizeof(void*));

}

/** Result in ms */
static inline float __attribute__((always_inline)) diff_timespec_in_ms(struct timeval *start, struct timeval *end)
{
  return 1000.0 * (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec)*0.001;
}

static inline void __attribute__((always_inline)) statistic_add_item(Statistic *stat, float value)
{
  if (stat->size <= 0)
  {
    stat->mean = value;
    stat->max = value;
    stat->min = value;
    stat->sigma2 = 0;
    stat->size = 1;
  }
  else
  {
    stat->size++;
    if (value > stat->max) stat->max = value;
    if (value < stat->min) stat->min = value;
    stat->mean += (value - stat->mean)/(float)stat->size;
    stat->sigma2 += (pow(value-stat->mean, 2))/(float)stat->size;
  }
}

static inline void __attribute__((always_inline)) close_library(void *lib_handle)
{
  dlclose(lib_handle);
}

typedef struct type_io_server_socket
{
  char name[MAXBUF];
  char comment[MAXBUF];
  enum io_server_socket_state state;
  int port;
/** pointe vers la variable globale reellement utilisee pour aller plus vite */
  int *socket_file;  
/** pointe vers la variable globale reellement utilisee pour aller plus vite */
  int *client;  
  struct sockaddr_in self;
  struct sockaddr_in client_addr;
  void (*gestionnaire_de_signaux)();
  pthread_t pthread;
} type_io_server_socket;

extern type_io_server_socket io_server_kernel;
extern type_io_server_socket io_server_debug;
extern type_io_server_socket io_server_console;


/** defined in iostrem_over_network.c */
extern void vkprints(const char *fmt, va_list ap);
extern void kprints(const char *fmt, ...);
extern void kscans(const char *fmt, ...);
extern void cscans(const char *fmt, ...);
extern void cprints(const char *fmt, ...);


#ifndef DEBUG
#define dprints(...) do{}while(0)  /* on ne fait rien et evite pbs si dprints suit un if par ex. (evite les pbs de ; )*/
#else
/** extern de basic_tools/iostream_over_network.c */
extern void true_dprints(const char *fmt, ...); 
#define dprints true_dprints
#endif

extern void cscans(const char *fmt, ...);

extern void open_io_server(type_io_server_socket *io_server, char *nom, int server_port, int *socket_file, int *client, const char *comment,void (*gestionnaire_de_signaux)());
extern void close_io_server(type_io_server_socket *io_server);
extern void global_close_io_server();

#ifndef DIAGNOSTIC
/** Fonctions possibles via ivy */
void Hello_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void help_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void status_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void autotest_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void run_online_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void run_use_only_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void continue_learning_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void continue_using_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void step_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void unstep_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void save_NN_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void save_NN_link_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void load_NN_link_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void quit_Callback (IvyClientPtr app, void *data, int argc, char **argv);
void cancel_Callback(IvyClientPtr app, void *data, int argc, char **argv);
void profile_group_callback(IvyClientPtr app, void *data, int argc, char **argv);
void connect_profiler_callback(IvyClientPtr app, void *data, int argc, char **argv);
void distant_debug_callback(IvyClientPtr app, void *data, int argc, char **argv);
void japet_callback(IvyClientPtr app, void *data, int argc, char **argv);

typedef struct type_commande
{
  char name[MAXBUF];
  void (*function)(IvyClientPtr app, void *data, int argc, char **argv);
  char help[MAXBUF];
} type_commande;

extern type_commande liste_commande[];
#endif

#endif



