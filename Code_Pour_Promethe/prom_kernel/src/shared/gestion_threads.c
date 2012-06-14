/* Fichier gestion_thread.c                                                                     */
/* P. Gaussier Aout 2005                                                                        */

/* lancement de tous les groupes en tant que threads.                                           */
/* Tous les threads passent alors dans un mode d'attente d'un semaphore pour s'executer         */
/* A la fin de leur execution ils remettent le semaphore a 0                                    */
/* La chaine de caractere message peut etre utilisee avant l'appel pour indiquer quel           */
/* genre d'execution est souhaitee.                                                             */

#include <stdlib.h>
#include <pthread.h>


/*
  #include <sys/ipc.h>
  #include <sys/shm.h>
*/

/* #define DEBUG 1  */

#include "public.h"
#include "rttoken.h"
#include "oscillo_kernel.h"

#include "net_message_debug_dist.h"

/*#define USE_THREADS 1 */

/*#define PROM_FS*/

/* verbose ne concerne que les messages affiches pendant le mode pas_a_pas.
   On peut donc laisser active cette option sans grand soucis d'affichage.
   Il suffit de ne pas se mettre en pas a pas. */
/*#define VERBOSE 1*/

extern int exec_cmd_break(int gpe);

extern void wg_trace_mode(void);
extern void wg_debug_pas_a_pas(int gpe, int gestion_STM, int local_flag_temps_dynamique);

#ifndef AVEUGLE
extern void affiche_debug_gpe(TxDonneesFenetre * fenetre, int gpe);
#endif

arg_thread_grp arg_global_thread_grp;

#ifdef PROM_FS
/*Mickael*/
/*pointeurs sur les zones de memoire partagees*/
type_neurone *shared_mem_n=NULL;
type_groupe *shared_mem_g=NULL;
#endif

/*PG: cette fonction devrait etre globalisee avec celle de gestion_sequ.c */
int launch_group_computation(int gpe, int local_learn)
{
  int ech;
  ech = def_groupe[gpe].ech_temps;

  UPDATE_OSCILLO_KERNEL(gpe,0);

  dprints("launch_group_computation group  %s (rt=%d)\n", def_groupe[gpe].no_name,def_groupe[gpe].rttoken);

  if (def_groupe[gpe].breakpoint > 0)
  {
    p_trace = 2;
    printf("\n\n===== BREAKPOINT ======= \n\n groupe %s (%d)\n", def_groupe[gpe].no_name, gpe);
  }

  if (p_trace == 2)
  {
    printf("launch_group_computation group  %s (rt=%d)\n", def_groupe[gpe].no_name, def_groupe[gpe].rttoken);
    wg_debug_pas_a_pas(gpe, 0, flag_temps_dynamique);
  }

  switch (def_groupe[gpe].type)
  {
  case No_Sub_Network:
    def_groupe[gpe].deja_active = 1;
    UPDATE_OSCILLO_KERNEL(gpe,1);
    return 1;
  case No_BREAK:
  	UPDATE_OSCILLO_KERNEL(gpe,1);
    return exec_cmd_break(gpe);
  case No_RTTOKEN:
    dprints("on ne fait rien pour rt_token gpe %s ici \n",def_groupe[gpe].no_name);
    return 1;
    /* return exec_cmd_rttoken_new(gpe); */
  case No_Fonction_Algo:
  case No_Fonction_Algo_mvt:
    def_groupe[gpe].deja_active = 1;
    (*def_groupe[gpe].appel_algo)(gpe /*,local_learn */);

#ifdef VERBOSE
    if (p_trace == 2)
    {
      printf("End of launch_group_computation %s \n", def_groupe[gpe].no_name);
    }
#endif

    UPDATE_OSCILLO_KERNEL(gpe,1);
    return 1;
    /*default: dprints("pas fini pour le gpe %s \n",def_groupe[gpe].no_name);*/
  }

#ifdef VERBOSE
  if (p_trace > 0) printf("lance l'execution du groupe neuronal %s \n", def_groupe[gpe].no_name);
#endif

  if (local_learn == 0)
  {
    (*def_groupe[gpe].appel_maj_STM_entrees)(gpe); /*modif PG sept 2005 */
    (*def_groupe[gpe].appel_algo)(gpe /*,local_learn */); /* ex mise_a_jour_normale */
    (*def_groupe[gpe].appel_maj_STM_sorties)(gpe); /*modif PG sept 2005 */

#ifdef VERBOSE
    if (p_trace > 0)
      printf("fin d'execution du groupe neuronal %s \n", def_groupe[gpe].no_name);
#endif
  }
  else
  {
#ifdef VERBOSE
    if (p_trace > 0)
      printf("--- apprentissage des poids du groupe %s, VIGILANCE = %f\n",def_groupe[gpe].no_name, vigilence);
#endif

    (*def_groupe[gpe].appel_apprend)(gpe);
  }

  dprints("gpe %s  deja_active=1 launch_group_computation\n",def_groupe[gpe].no_name);

#ifdef VERBOSE
  if (p_trace == 2)
  {
    printf("End of launch_group_computation %s \n", def_groupe[gpe].no_name);
  }
#endif

  dprints("End of launch_group_computation %s \n", def_groupe[gpe].no_name);
  UPDATE_OSCILLO_KERNEL(gpe,3);
  return 1;
}

/* destruction du thread associe au groupe et liberation des semaphores associes */
void destroy_group_thread(int gpe)
{
  int res;

  def_groupe[gpe].destroy(gpe); /* fonction pour la destruction des donnees / utilisateur */

  res = sem_destroy(&def_groupe[gpe].sem_wake_up);
  dprints("res destroy sem_wake_up gpe %s = %d \n", def_groupe[gpe].no_name, res);
  res = sem_destroy(&def_groupe[gpe].sem_ack);
  dprints("res destroy sem_ack gpe %s = %d \n", def_groupe[gpe].no_name, res);
  res = sem_destroy(&def_groupe[gpe].sem_lock_fields);
  dprints("res destroy sem_lock_field gpe %s = %d \n", def_groupe[gpe].no_name, res);

  dprints("Global: Arret du thread pour le gpe %s \n", def_groupe[gpe].no_name);
}

/* fonctions appelee par defaut pour gerer le thread associe a un gpe */
void *generic_create_and_manage(void *p_arg_gpe)
{
  int gpe, fin;
  int local_learn;
  int res;
  int ech;

  gestion_mask_signaux();

  gpe = ((arg_thread_grp *) p_arg_gpe)->gpe;
  local_learn = ((arg_thread_grp *) p_arg_gpe)->learn;
  ech = def_groupe[gpe].ech_temps;

  dprints("Global: Lancement du thread pour le gpe %s \n", def_groupe[gpe].no_name);

  res = sem_init(&def_groupe[gpe].sem_wake_up, 0, 0);
  if (res != 0)
  {
    printf("ERROR: %s res init sem_wake_up gpe %s = %d \n", __FUNCTION__, def_groupe[gpe].no_name, res);
    exit(EXIT_FAILURE);
  }
  res = sem_init(&def_groupe[gpe].sem_ack, 0, 0);
  if (res != 0)
  {
    printf("ERROR %s, res init sem_ack gpe %s = %d \n", __FUNCTION__, def_groupe[gpe].no_name, res);
    exit(EXIT_FAILURE);
  }
  res = sem_init(&def_groupe[gpe].sem_lock_fields, 0, 0);
  if (res != 0)
  {
    printf("ERROR %s res init sem_lock_field gpe %s = %d \n", __FUNCTION__, def_groupe[gpe].no_name, res);
    exit(EXIT_FAILURE);
  }
  def_groupe[gpe].new(gpe);

  /* boucle principale de la gestion du groupe */
  fin = 0;

  /*semaphore compteur signalant que le thread est bien cree*/
  sem_post(((arg_thread_grp *) p_arg_gpe)->p_is_launched);

  while (fin == 0)
  {
    do
    {
      res = sem_wait(&def_groupe[gpe].sem_wake_up); /* attend qu'on le reveille */
      if (res < 0)
      {
	dprints("ERROR: pb pour le reveil du gpe %s res semaphore wake_up = %d\n",def_groupe[gpe].no_name, res);
	dprints("ou est le pb, diff compilateur et options... \n");
	/*  exit(EXIT_FAILURE); */
      }
    } while (res < 0);

#ifdef PROM_FS
    /*Mickael*/
    /*recopie des valeurs des neurones par la copie de tout le groupe...*/
    memcpy(shared_mem_n + def_groupe[gpe].premier_ele , &(neurone[def_groupe[gpe].premier_ele]), def_groupe[gpe].nbre*sizeof(struct type_neurone) );
    /*faudrait que ca soit fait uniquement sur demande*/
#endif

    res = sem_post(&def_groupe[gpe].sem_lock_fields); /* reactive le thread en attente */

    dprints("---groupe %s locked \n",def_groupe[gpe].no_name);

    if (res != 0)
    {
      dprints("fealure on sem_post sem_lock_fields in generic_create_and_manage gpe %s (res=%d)\n",def_groupe[gpe].no_name,res);
      printf("ERROR: %s fealure on sem_post sem_lock_fields in generic_create_and_manage gpe %s (res=%d)\n", __FUNCTION__, def_groupe[gpe].no_name, res);
      exit(EXIT_FAILURE);
    }

#ifdef VERBOSE
    if (p_trace > 0)
    {
      printf("le gpe %s a ete reveille. Le message est %s. \n", def_groupe[gpe].no_name, def_groupe[gpe].message);
      printf("demande l'execution du groupe : %s ,  flag_temps_dynamique = %d \n", def_groupe[gpe].no_name, flag_temps_dynamique);
    }
#endif

    /* un message a ete envoye: choix de l'execution en fonction du message */
    /* message vide = execution normale */
    if (def_groupe[gpe].message[0] == 0)
    {
      dprints("execution normale groupe %s \n",def_groupe[gpe].no_name);
      def_groupe[gpe].return_value = launch_group_computation(gpe, local_learn);

#ifndef AVEUGLE
      if(def_groupe[gpe].debug != 0 && debug!=0 && ech >= echelle_temps_debug)
      {
	//def_groupe[gpe].premier_ele    def_groupe[gpe].nbre
	affiche_debug_gpe(&fenetre1, gpe);
	TxDisplay(&fenetre1);
	send_neurons_to_japet(gpe);
      }
#endif

    }
    else if (strcmp(def_groupe[gpe].message, "kill") == 0)
      fin = 1;
    else if (strcmp(def_groupe[gpe].message, "init") == 0)
    {
      dprints("init du thread du gpe %s \n", def_groupe[gpe].no_name);
      def_groupe[gpe].new(gpe);
    }

#ifdef VERBOSE
    if (p_trace > 0)
    {
      printf("envoie le sem_ack pour le gpe %s \n", def_groupe[gpe].no_name);
    }
#endif
    sem_post(&def_groupe[gpe].sem_ack); /* previent que l'execution s'est bien terminee */
    /* le thread general attend l'ack de tous les groupes lances pour passer a la suite */
  }

  destroy_group_thread(gpe);

  free(p_arg_gpe);
  /* set return value to exit */
  def_groupe[gpe].return_value=3;
  kprints("Thread grp %s has stopped\n",def_groupe[gpe].no_name);
  pthread_exit(NULL);
}

/*-----------------------------------------------------------------------*/
/* Fonction appelee par le sequenceur.                                   */
/* Nouvelle fonction appelee pour communiquer avec un thread deja lance. */

int execute_en_parallele2(int *gpes_en_parallele, int nbre_parallele, int local_learn, int gestion_STM, int mvt)
{
  int i, gpe, res;
  int val_retour = 0;
  /* int valeur; */

#ifdef VERBOSE
  if (p_trace > 0)
    printf("demande l'execution en parallele de %d groupes \n",nbre_parallele);
#endif

  for (i = 0; i < nbre_parallele; i++) /* pour tous les groupes meme le premier gpe est gere par thread contrairement a avant */
  {
#ifdef VERBOSE
    if (p_trace > 0)
      printf("Appel thread %d  pour le groupe %s\n", i,def_groupe[gpes_en_parallele[i]].no_name);
#endif
    /*      arg[i].no_thread = i; */
    gpe = gpes_en_parallele[i];
    arg_global_thread_grp.gestion_STM = gestion_STM;
    arg_global_thread_grp.learn = local_learn;
    arg_global_thread_grp.mvt = mvt;
    def_groupe[gpe].return_value = 0; /* on suppose l'echec avant le lancement */
    def_groupe[gpe].message[0] = 0; /* message vide correspond a un execution normale */

    res = sem_post(&def_groupe[gpe].sem_wake_up); /* reactive le thread en attente */
    if (res != 0)
    {
      dprints("fealure on sem_post in execute_en_parallele2 gpe %s \n",def_groupe[gpe].no_name);
      exit(EXIT_FAILURE);
    }
  }

#ifdef VERBOSE
  if (p_trace > 0)
    printf("En attente de terminaison de tous les threads declenches en parallele par le semaphore ... \n");
#endif
  for (i = 0; i < nbre_parallele; i++)
  {
    gpe = gpes_en_parallele[i];
    /* res = sem_getvalue(&def_groupe[gpe].sem_ack,&valeur);
       dprints("valeur semaphore gpe %s avant retour = %d, res= %d\n",def_groupe[gpe].no_name,valeur,res); */
    do
    {
      res = sem_wait(&def_groupe[gpe].sem_ack);
      if (res < 0)
      {
	dprints("~~~~~~~~~ ERROR de sem_wait %d pour le thread %d, gpe=%s (execute_en_parallele2)\n",
		res, i, def_groupe[gpe].no_name);
	dprints("......... Je recommence...\n");
      }
    } while (res < 0);
    /*      res = sem_getvalue(&def_groupe[gpe].sem_ack,&valeur);
	    dprints("valeur semaphore gpe %s apres retour = %d, res= %d\n",def_groupe[gpe].no_name,valeur,res); */

#ifdef VERBOSE
    if(p_trace > 0)
      printf("i=%d retour du groupe %s = %d \n", i, def_groupe[gpe].no_name, def_groupe[gpe].return_value);
#endif

    if (res == 0)
    {
#ifdef VERBOSE
      if(p_trace > 0)
	printf("thread %d/%d recueilli \n", i + 1, nbre_parallele);
#endif
    }
    else /* impossible maintenant avec le do while qui precede... */
    {
      dprints("echec de sem_wait %d pour le thread %d, gpe=%s\n", res, i,def_groupe[gpe].no_name);
      exit(EXIT_FAILURE);
    }

    dprints("gpe %s  deja_active=1 execute_en_parallele2\n",def_groupe[gpe].no_name);
    def_groupe[gpe].deja_active = 1;

    res = sem_wait(&def_groupe[gpe]. sem_lock_fields); /* reprend le jeton (on peut maintenant modifier les champs du gpe) */

    dprints("---groupe %s unlocked \n",def_groupe[gpe].no_name);
    if (res < 0)
    {
      dprints("ERROR: pb  sem_lock_fields gpe %s res = %d\n", def_groupe[gpe].no_name, res);
      dprints("ou est le pb, diff compilateur et options... \n");
      /*  exit(EXIT_FAILURE); */
    }

    if (def_groupe[gpe].return_value > val_retour) val_retour = def_groupe[gpe].return_value; /* le break retour=2 est prioritaire */

    if (def_groupe[gpe].return_value == 0)
    {
      dprints("Problem in the execution of the thread linked to the group %s \n", def_groupe[gpe].no_name);
      dprints("Return value after running is 0. This should not happend... \n");
      exit(EXIT_FAILURE);
    }
  } /* fin boucle for groupes en parallele */

#ifdef VERBOSE
  if (p_trace > 0)
    printf("activation de la vague de THREADS paralleles terminee (tous les ack recus). \n");
#endif

  return val_retour;
}

/* le cas des groupes rien ou debut n'est pas traite pour l'instant
   (perte de temps d'execution...) */

void launch_each_group_as_a_thread(void)
{
  int gpe, res;
  arg_thread_grp *local_arg;
  pthread_attr_t thread_attr;
#ifdef PROM_FS
  /*Mickael*/
  int id_n;
  int id_g;
#endif

  /*semaphore s'assurant que tous les threads sont bien lances avant de continuer. M.M. 07/02/07*/
  sem_t is_launched;
  sem_init(&is_launched, 0, 0);

  pthread_attr_init(&thread_attr);
  dprints("Set Real Time policy ? \n");
  /*SCHED_OTHER ,  SCHED_FIFO SCHED_RR temps reel ne marche que pour root !!! */
  res = pthread_attr_setschedpolicy(&thread_attr, SCHED_OTHER);
  if (res != 0)
  {
    dprints("WARNING problem to set scheduling policy in launch_each_group_as_a_thread() - kernel:gestion_threads.c \n");
    dprints("Error number is %d \n", res);
    /* if(res==ENOTSUP) */
    dprints("You must be superuser/root to use the real time mode or use suid mode...\n");
  }

  dprints("launch_each_group_as_a_thread();\n");

  for (gpe = 0; gpe < nbre_groupe; gpe++)
  {
    dprints("Lancement du thread pour le gpe %s \n", def_groupe[gpe].no_name);
    /*   res = pthread_create( & def_groupe[i].pthread, NULL, def_groupe[i].create_and_manage,(void*) local_arg); */
    local_arg = (arg_thread_grp *) malloc(sizeof(arg_thread_grp));
    local_arg->learn = 0; /* il faut esperer que le contenu de local_arg change plus
			     lentement que l'execution du thread associe... */
    local_arg->gpe = gpe;
    local_arg->p_is_launched = &is_launched;

    /*      res = pthread_create( & def_groupe[gpe].pthread, NULL, generic_create_and_manage,(void *) local_arg); */
    strcpy(def_groupe[gpe].message, "start");
    if (def_groupe[gpe].type != No_RTTOKEN)
    {
      res = pthread_create(&def_groupe[gpe].pthread, &thread_attr, generic_create_and_manage, (void *) local_arg);
    }
    else
      res = pthread_create(&def_groupe[gpe].pthread, &thread_attr, rt_token_create_and_manage, (void *) local_arg);

    if (res != 0)	EXIT_ON_ERROR("fealure on thread creation for group %s \n", def_groupe[gpe].no_name);
  }

  /* On quitte la zone critique de la boucle principale de gdk pour
     laisser la main si on veut utiliser gtk dans les new des
     groupes */
#ifndef AVEUGLE
  gdk_threads_leave();
#endif
  /*on attend que tous les threads aient ete crees: le semaphore sert de compteur*/
  for (gpe = 0; gpe < nbre_groupe; gpe++)
  {
    sem_wait(&is_launched);
  }
#ifndef AVEUGLE
  gdk_threads_enter();
#endif

  dprints("Tous les threads des groupes ont ete lances \n");

#ifdef PROM_FS
  /*Mickael*/

  /*faudrait pas en creer de nouvelles mais reutiliser celle exsitentes pour etre efficace*/

  /*zone de memoire pour les neurones*/
  id_n=shmget(3,(nbre_neurone+1) * sizeof(struct type_neurone),IPC_CREAT | 0740 );
  if(id_n==-1)
  {
    perror("shmget");
    exit(0);
  }
  shared_mem_n = shmat(id_n, NULL, 0);
  if(shared_mem_n==(void*)-1)
  {
    perror("shmget");
    exit(0);
  }
  shared_mem_n[0].s1 = (float) nbre_neurone;
  shared_mem_n = shared_mem_n + 1;
  memcpy(shared_mem_n , neurone, nbre_neurone*sizeof(struct type_neurone) );

  /*zone de memoire pour les groupes*/
  id_g=shmget(4,(nbre_groupe+1) * sizeof(struct type_groupe),IPC_CREAT | 0740 );
  if(id_g==-1)
  {
    perror("shmget");
    exit(0);
  }
  shared_mem_g = shmat(id_g, NULL, 0);
  if(shared_mem_g==(void*)-1)
  {
    perror("shmget");
    exit(0);
  }
  shared_mem_g[0].no = nbre_groupe;
  shared_mem_g = shared_mem_g + 1;
  memcpy(shared_mem_g , def_groupe, nbre_groupe*sizeof(struct type_groupe) );

#endif

  /* mise en route effective des groupes temps reels qui sont bloques en attente du wake_up */
  for (gpe = 0; gpe < nbre_groupe; gpe++)
  {
    if (def_groupe[gpe].type == No_RTTOKEN)
    {
      res = sem_post(&def_groupe[gpe].sem_wake_up); /* on active le thread en attente */
      if (res != 0) EXIT_ON_ERROR("fealure on sem_post in start rt_token gpe %s \n",def_groupe[gpe].no_name);

    }
  }
}

/* envoie un message a chaque groupe pour leur dire de s'arreter */
/* Cette fonction force l'arret des groupes qui ne se terminent pas.*/
/* Par ex., attente dans un scanf, socket...                        */
/* Le thread associe a la fonction est cancelled.                   */
/* L'appel au destroy du groupe se fait lui jusqu'a la fin.         */

void terminate_each_group_as_a_thread_hard(void)
{
  int gpe, res;
  pthread_t self_pthread=pthread_self();
  
  kprints("HARD TERMINATION... \n");
  dprints("terminate_each_group_as_a_thread_hard();\n");

  /* type pthread_t is opaque and depends on architecture - %p may not
   * be optimal */
  dprints("Calling thread is %p\n",self_pthread);  

  for (gpe = 0; gpe < nbre_groupe; gpe++)
  {
    dprints("Going to terminate thread %p (gpe %s) ?\n",def_groupe[gpe].pthread, def_groupe[gpe].no_name);
    /* cancelling threads that are not the calling thread and that
     * were not already stopped */
    if(self_pthread!=def_groupe[gpe].pthread && def_groupe[gpe].return_value!=3) {
      destroy_group_thread(gpe);
      res = pthread_cancel(def_groupe[gpe].pthread);
      kprints("Cancel thread grp %s, res = %d \n", def_groupe[gpe].no_name, res);
    }
  }
  dprints("terminate_each_group_as_a_thread();\n");
}

/* Fonction appelee a la fin d'une simulation ou lorsque l'on quitte le simulateur*/
/* On envoie un message aux threads pour leur demander de s'arreter a la fin de l'excution
 * de la fonction qui leur est associee.
 * Un pthread_join permet de verfier que tous les threads se sont bien arretes.
 * Si un thread ne se termine pas (scanf, attente..., on reste coince...
 * Pour etre plus propre, il faudrait rajouter un flag sur chaque groupe afin de connaitre
 * son etat et notamment eviter de le detruire 2 fois... */

void terminate_one_group_as_a_thread(int gpe)
{
  void *local_resultat;
  int res;
   
  strcpy(def_groupe[gpe].message, "kill");
  res = sem_post(&def_groupe[gpe].sem_wake_up); /* reactive le thread en attente */
  if (res != 0)
  {
    dprints("failure on sem_post in terminate_each_group_as_a_thread gpe %s \n",def_groupe[gpe].no_name);
    exit(EXIT_FAILURE);
  }
  res = pthread_join(def_groupe[gpe].pthread, &local_resultat);
  if (res == 0)
  {
    if (p_trace > 0)
      dprints("thread du groupe %s recueilli \n", def_groupe[gpe].no_name);
  }
  else
  {
    EXIT_ON_ERROR("echec de pthread_join %d pour le thread/groupe %s\n", res, def_groupe[gpe].no_name);
  }
}

/*important de faire la difference entre rttoken et les autres car la fonction de reset
  des rt_token peut relancer l'apprentissage d'autres boites qui pourraient avoir deja ete detruites
  (appel de leur fonction destroy lors du kill) */

void terminate_each_rttoken_group_as_a_thread(void)
{
  int gpe;

  dprints("terminate_each_rttoken_group_as_a_thread();\n");

  for (gpe = 0; gpe < nbre_groupe; gpe++)
  {
    if (def_groupe[gpe].type == No_RTTOKEN) terminate_one_group_as_a_thread(gpe);
  }
}

void terminate_each_normal_group_as_a_thread(void)
{
  int gpe;

  dprints("terminate_each_normal_group_as_a_thread();\n");

  for (gpe = 0; gpe < nbre_groupe; gpe++)
  {
    if (def_groupe[gpe].type != No_RTTOKEN) terminate_one_group_as_a_thread(gpe);
  }
}

void terminate_each_group_as_a_thread(void)
{

  dprints("terminate_each_group_as_a_thread();\n");

  terminate_each_rttoken_group_as_a_thread();

  terminate_each_normal_group_as_a_thread();

}

