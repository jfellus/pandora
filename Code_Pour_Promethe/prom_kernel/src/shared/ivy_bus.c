/* cc hello.c -o hello -I/usr/local/include/Ivy -L/usr/local/lib64 -lglibivy -lpcre*/
/* mettre la librairie a la fin des arguments de user */

#define DEBUG 1

static int current_state;

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdarg.h>
#include <ivy.h>

#ifdef AVEUGLE
#include <ivyloop.h>
#else
#include <ivyglibloop.h>
#endif

#include "string.h"
#include "net_message_debug_dist.h"
#include "public.h"
#include "prom_tools/include/outils_script.h"
#include "prom_tools/include/prom_bus.h"
#include "japet_connect.h"

#define TAILLE_CHAINE 256


char virtual_name[LOGICAL_NAME_MAX];
static sem_t ivy_semaphore;
char bus_id[BUS_ID_MAX];


type_commande liste_commande[]=
{
  {	"Hello", Hello_Callback, "provide the list of connected promethes"},
  {	"help", help_Callback, "this help \n [xxx] is the optional name of a NN (script name or virtual name if the prt is defined and -d VirtualName option is used."},
  {	"learn", run_online_Callback, "launch the online learning simulation"},
  {	"use", run_use_only_Callback, "launch the use only simulation"},
  {	"continue_learning", continue_learning_Callback, "continues the online learning simulation"},
  {	"continue_using", continue_using_Callback, "continues the use only learning simulation"},
  {	"save", save_NN_Callback, "save NN state"},
  {	"save_link\\((.*)\\)", save_NN_link_Callback, "(name,xxx,yyy)' : save links from group xxx to yyy (file named name.lres)"},
  {	"load_link\\((.*)\\)", load_NN_link_Callback, "(name,xxx,yyy)' : load links from group xxx to yyy (file named name.lres)"},
  {	"step", step_Callback, "start step by step mode"},
  {	"unstep", unstep_Callback, "leave step by step mode"},
  {	"cancel", cancel_Callback, "cancel (put in stanby)"},
  {	"autotest", autotest_Callback, "perform an autotest"},
  {	"status", status_Callback,"provide name of the scripts, config and res used + No of the telnet ports to connect the promethe(s)"},
  {	"quit", quit_Callback, "stop all the simulations (launch destroy_groups()) and close the different sockets)"},
  {	"profile\\((.*)\\)", profile_group_callback, "(group name, n)': send profile statistics about the group (group_name) each n iterations (n=0 to remove the profiling of this group). The return is: stat(name:function,number of samples,min,max,mean,sigma2)"},
  {	"distant_debug\\((.*)\\)", distant_debug_callback, "Send debug info."},
  {	"japet\\((.*)\\)", japet_callback, "Send japet command."},
  {	"\0", NULL, "\0"}
};

/* Pas de macro car necessite plusieurs appels de fonctions a la suite ce qui pose des probleme avec if and else. De toute facon le tmeps est negligeable et permet de separer prom_bus de IvyBus (pas besoin d'inclure IvyBus.h, prom_bus.h suffit)*/
void prom_bus_send_message(const char *format, ...)
{
  char *buffer;
  va_list arguments;

  va_start (arguments, format);
  if (vasprintf(&buffer, format, arguments) < 0) EXIT_ON_ERROR("Fail to allow the buffer"); /* Alloue le buffer de la bonne taille */
  sem_wait(&ivy_semaphore);
  IvySendMsg("%s:%s", bus_id, buffer);
  sem_post(&ivy_semaphore);
  free(buffer);
}

void prom_bus_change_state(int state)
{
  prom_bus_send_message("status(%d)\n", state);
  current_state = state;
}

char *prom_bus_get_name()
{
  return virtual_name;
}

/* callback associated to "Hello" messages */
void Hello_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  prom_bus_send_message("here(%d,%d,%d,%d)\n", current_state, io_server_kernel.port, io_server_console.port, io_server_debug.port);
}

void help_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  int i;
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  prom_bus_send_message("Name of promethe:%s responding to 'help':\n By default all the commands are sent to all promethes of the network. Otherwise, use: '<command> <name of promethe>'.\n", virtual_name);
  for(i=0; liste_commande[i].name[0]!=0; i++)
  {
    prom_bus_send_message("'%s' : %s\n", liste_commande[i].name, liste_commande[i].help);
  }
}

void status_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  prom_bus_send_message ("%s status:\n", virtual_name);
  prom_bus_send_message("script = %s \n",fscript);
  prom_bus_send_message("res = %s \n", freseau);
  prom_bus_send_message("config = %s \n", fconfig);
  prom_bus_send_message("port client_kernel = %d  \n", io_server_kernel.port);
  prom_bus_send_message("port client_debug = %d  \n", io_server_debug.port);
  prom_bus_send_message("port client_console_user = %d  \n", io_server_console.port);
}

extern int autotest();
void autotest_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  int res;
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  res=autotest();
  if(res==1) prom_bus_send_message ("autotest %s = %d [OK] \n", virtual_name, res);
  else prom_bus_send_message ("autotest %s = %d [PROBLEM!!!!] \n", virtual_name, res);
  printf("callback pour autotest active res=%d\n",res);
}

void run_online_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;
  prom_bus_send_message ("running %s\n", fscript);
#ifndef AVEUGLE
  gdk_threads_enter();
#endif
  set_learn_and_use_mode(NULL, NULL);
#ifndef AVEUGLE
  gdk_threads_leave();
#endif
  printf("callback pour run active \n");
}

void run_use_only_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  prom_bus_send_message ("running in use only mode %s\n", fscript);
#ifndef AVEUGLE
  gdk_threads_enter();
#endif
  set_use_only_mode(NULL, NULL);
#ifndef AVEUGLE
  gdk_threads_leave();
#endif
  printf("callback pour run in use only mode active \n");
}

void continue_learning_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  prom_bus_send_message ("running %s (continue learning)\n", fscript);
#ifndef AVEUGLE
  gdk_threads_enter();
#endif
  set_continue_learning_mode(NULL, NULL);
#ifndef AVEUGLE
  gdk_threads_leave();
#endif
  printf("callback pour continue_learning active \n");
}

void continue_using_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  prom_bus_send_message ("running %s (continue using)\n", fscript);
#ifndef AVEUGLE
  gdk_threads_enter();
#endif
  set_continue_only_mode(NULL, NULL);
#ifndef AVEUGLE
  gdk_threads_leave();
#endif
  printf("callback pour continue_using active \n");
}

void step_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  prom_bus_send_message ("%s entering step by step mode\n", fscript);
  step_by_step_pressed(NULL, NULL);
  printf("callback pour step by step active \n");
}

void unstep_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  prom_bus_send_message ("%s leaving step by step mode\n", fscript);
  continue_pressed(NULL, NULL);
  printf("callback pour unstep active \n");
}

void save_NN_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  ecriture_reseaub(NULL, NULL);
  prom_bus_send_message ("saved %s\n", fscript);
  printf("callback pour save_NN active \n");
}

void save_NN_link_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  char link_file_name[MAXBUF], input_group_name[MAXBUF], output_group_name[MAXBUF];
  int success;
  (void)app;
  (void)data;

  if(argc!=1)
  {
    prom_bus_send_message ("ERROR cannot read save_link arguments (argc= %i !=1) for script %s\n", argc, fscript);
    return;
  }
  prom_bus_send_message ("save link for script %s with arg %s\n", fscript,argv[0]);
  printf("callback pour save_NN_link active \n");

  if (sscanf(argv[0], "%[^,],%[^,],%[^)]",link_file_name, input_group_name, output_group_name)!=3) PRINT_WARNING("Wrong format: %s", argv[0]);

  strcat(link_file_name,".lres");
  printf("%s :%s ( %s , %s )\n", fscript, link_file_name, input_group_name, output_group_name);
  success=ecriture_voie_liaison(fscript, link_file_name, input_group_name, output_group_name);

  if(success==1) prom_bus_send_message ("save link for script %s with arg completed %s\n", fscript,argv[0]);
  else prom_bus_send_message ("FAILED to save link for script %s with arg %s\n", fscript,argv[0]);
}

void load_NN_link_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  char link_file_name[MAXBUF], input_group_name[MAXBUF], output_group_name[MAXBUF];
  int success;
  (void)app;
  (void)data;

  if(argc!=1)
  {
    prom_bus_send_message ("ERROR cannot read save_link arguments (argc= %i !=1) for script %s\n", argc, fscript);
    return;
  }
  prom_bus_send_message ("load link for script %s with arg %s\n", fscript,argv[0]);
  printf("callback pour load_NN_link active \n");

  if (sscanf(argv[0], "%[^,],%[^,],%[^)]",link_file_name, input_group_name, output_group_name)!=3) PRINT_WARNING("Wrong format: %s", argv[0]);

  strcat(link_file_name,".lres");

  printf("%s :%s ( %s , %s )\n", fscript, link_file_name, input_group_name, output_group_name);
  success=lecture_voie_liaison(link_file_name, input_group_name, output_group_name);

  if(success==1) prom_bus_send_message ("SUCCESS load link for script %s with arg %s %s %s\n", fscript,link_file_name, input_group_name, output_group_name);
  else prom_bus_send_message ("FAILED to load link for script %s with arg %s %s %s\n", fscript,link_file_name, input_group_name, output_group_name);
}

/* callback associated to "Bye" messages */
void quit_Callback (IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  IvyStop ();
  kprints("Arret demande par ivy_bus ByeCallbac \n");
  exit(0); /* promethe_quit() execute avant exit grace a atexit */

}

/*** Begin profile ***/
void cancel_Callback(IvyClientPtr app, void *data, int argc, char **argv)
{
  (void)app;
  (void)data;
  (void)argc;
  (void)argv;

  cancel_pressed(NULL, NULL);
  prom_bus_send_message ("cancelling %s\n", fscript);
  printf("callback pour cancel simul active \n");
}

void profile_group_callback(IvyClientPtr app, void *data, int argc, char **argv)
{
  char group_name[SIZE_NO_NAME];
  int index_of_group, number_of_iterations;
  (void)app;
  (void)data;
  (void)argc;

  sscanf(argv[0], "%[^,],%d", group_name, &number_of_iterations);
  if (strcmp("*", group_name) == 0)
  {
    for (index_of_group = 0; index_of_group < nbre_groupe; index_of_group++)
    {
      if (def_groupe[index_of_group].type == No_Fonction_Algo || def_groupe[index_of_group].type == No_Fonction_Algo_mvt)
      {
	if (number_of_iterations == 0) destroy_statistical_debug_group(index_of_group);
	else new_statistical_debug_group(index_of_group, number_of_iterations);
      }
    }
  }
  else
  {
    index_of_group = find_no_associated_to_symbolic_name(group_name, NULL);
    if (index_of_group < 0) prom_bus_send_message("Name of group: '%s' unknown.");
    else
    {
      if (number_of_iterations == 0) destroy_statistical_debug_group(index_of_group);
      else new_statistical_debug_group(index_of_group, number_of_iterations);
    }
  }
}

void distant_debug_callback(IvyClientPtr app, void *data, int argc, char **argv)
{

  (void)data;
  (void)argc;

  {
#if (USE_ENET)

    switch (atoi(argv[0]))
    {
    case DISTANT_DEBUG_START:dprints("Connect distant debug from: %s.", IvyGetApplicationName(app));
      init_enet_token_oscillo_kernel(IvyGetApplicationHost(app), 1234);
      break;
    }
#endif
  }
}

/**
* 
* Detecte la connection de Japet avec le protocole IVY et appele la fonction japet_connect
*
* @see japet_connect()
*/
void japet_callback(IvyClientPtr app, void *data, int argc, char **argv)
{

  (void)data;
  (void)argc;

  printf("Japet s'est connecté\n");
  
#if (USE_ENET)
    switch (atoi(argv[0]))
    {
    case JAPET_START:
      dprints("Connect distant japet from: %s at address %s.\n", IvyGetApplicationName(app), IvyGetApplicationHost(app));
      japet_connect( IvyGetApplicationHost(app), 1235);
      break;
    }
#endif
  
}




/*** End profile ***/

void bind_ivy_callbacks(type_commande liste_commande[], char *prom_name, void *user_data)
{

  int i;
  static char message[MAXBUF];

  for(i=0; liste_commande[i].name[0]!=(char) 0; i++)
  {
    sprintf(message,"^%s:%s$", bus_id, liste_commande[i].name);
    IvyBindMsg(liste_commande[i].function, user_data, message);

    sprintf(message,"^%s:%s %s$", bus_id, liste_commande[i].name, prom_name); /*utiliser IvySendDirectMsg ? */
    IvyBindMsg(liste_commande[i].function, user_data, message);
  }
}

void ApplicationCallback (IvyClientPtr app, void *user_data, IvyApplicationEvent event)
{
  char *appname;
  /*char *host;*/

  appname = IvyGetApplicationName (app);
/*  host = IvyGetApplicationHost (app); */

  switch (event)
  {
  case IvyApplicationConnected:
    dprints("Connexion de %s\n", appname);
    fflush(stdout);
    fflush(stderr);

    Hello_Callback(app, user_data, 0, NULL);
    break;
  case IvyApplicationDisconnected:
    dprints("%s disconnected\n", appname);
    break;

  default:
    dprints("%s: unkown event %d\n", appname, event);
    break;
  }
}

void main_loop_ivy(int argc, char *argv[])
{
  /* handling of -b option */
  const char* bus = 0;
  char computer_name[LOGICAL_NAME_MAX];
  long int my_pid;
  /*  char c;*/
  int i;

#ifdef AVEUGLE
  int res;
  pthread_t un_thread;
#endif
  (void)argc;

  if(argv[No_arg_bus_ivy][0]!='\0')
  {
    kprints("Argument pour le bus ivy = %s \n",argv[ No_arg_bus_ivy]);
    bus=argv[No_arg_bus_ivy];
  }
  else
  {
    bus= getenv("IVYBUS");
  }

  kprints("Ivy bus: %s \n", bus);

  if(argv[No_arg_VirtualNetwork][0]!=(char)0) strncpy(virtual_name, argv[No_arg_VirtualNetwork], LOGICAL_NAME_MAX);
  else
  {
    gethostname(computer_name, LOGICAL_NAME_MAX);
    my_pid = getpid();
    snprintf(virtual_name, LOGICAL_NAME_MAX, "%s:%ld", computer_name, my_pid);
  }

  if(argv[No_arg_StartMode][0]!='\0') sscanf(argv[No_arg_StartMode], "%d", &i);

  sem_init(&ivy_semaphore, 0, 1); /* semaphore necessaire pour faire des appels thread safes... Prom_bus_send_message n'est pas suffisant ??? */
  IvyInit (virtual_name, NULL, ApplicationCallback , NULL, NULL, NULL);
  IvyStart (bus);
  bind_ivy_callbacks(liste_commande, virtual_name, NULL); /* Permet de passer un pointeur sur une structure perso (user_data) */

#ifdef AVEUGLE  /* et si pas gtk ...*/

  /* ivy main loop version AVEUGLE*/
  res = pthread_create(&un_thread, NULL,(void*(*)(void *))IvyMainLoop, NULL);
  res = pthread_detach(un_thread);

  if (res != 0) kprints("Echec detach IvyMainLoop\n");
#endif

}
