/* japet.c
 *
 * Auteurs : Brice Errandonea et Manuel De Palma
 * 
 * Pour compiler : make
 * 
 * 
 */

#include "japet.h"


//--------------------------------------------------VARIABLES GLOBALES----------------------------------------------------

GtkWidget* pWindow; //La fenêtre de l'application Japet
GtkWidget* pPane; //Panneau latéral
GtkWidget* pVBoxScripts; //Panneau des scripts
GtkWidget* zone3D; //La grande zone de dessin des liaisons entre groupes
GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale, *neuronHeightScale, *digitsScale; //Échelles

//Indiquent quel est le mode d'affichage en cours (Off-line, Sampled ou Snapshot)
gchar* displayMode;
GtkWidget* modeLabel; 

gint Index[NB_SCRIPTS_MAX]; //Tableau des indices : Index[0] = 0, Index[1] = 1, ..., Index[NB_SCRIPTS_MAX-1] = NB_SCRIPTS_MAX-1;                                                                                                                                             
//Ce tableau permet à une fonction signal de retenir la valeur qu'avait i au moment où on a connecté le signal                                                                                                                                                               

GtkWidget** openScripts; //Lignes du panneau des scripts                                                                                                                                                                                                                     
GtkWidget** scriptCheck; //Cases à cocher/décocher pour afficher les scripts ou les masquer                                                                                                                                                                                  
GtkWidget** scriptLabel; //Label affichant le nom d'un script dans la bonne couleur                                                                                                                                                                                          
GtkWidget** zChooser; //Spin_button pour choisir dans quel plan afficher ce script                                                                                                                                                                                           

gint nbScripts = 0; //Nombre de scripts à afficher                                                                                                                                                                                                                           
script* scr; //Tableau des scripts à afficher                                                                                                                                                                                                                                
int newScriptNumber = 0; //Numéro donné à un script quand on l'ouvre                                                                                                                                                                                                         
gint zMax; //la plus grande valeur de z parmi les scripts ouverts                                                                                                                                                                                                            
gint buffered = 0; //Nombre d'instantanés actuellement en mémoire                                                                                                                                                                                                            

gint usedWindows = 0;
group* selectedGroup = NULL; //Pointeur sur le groupe actuellement sélectionné                                                                                                                                                                                               
gint selectedWindow = NB_WINDOWS_MAX; //Numéro de la fenêtre sélectionnée (entre 0 et NB_WINDOWS_MAX-1). NB_WINDOWS_MAX indique qu'aucune fenêtre n'est sélectionnée                                                                                                         

GtkWidget* pHBox2; //Panneau des neurones                                                                                                                                                                                                                                    
GtkWidget* pFrameNeurones[NB_WINDOWS_MAX];//Tableau de NB_WINDOWS_MAX adresses de petites fenêtres pour les neurones                                                                                                                                                         
GtkWidget* zoneNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de zones où dessiner des neurones                                                                                                                                                              
group* windowGroup[NB_WINDOWS_MAX]; //Adresse des groupe affiché dans la zoneNeurones de même indice                                                                                                                                                                         
gint windowValue[NB_WINDOWS_MAX]; //Numéro disant quelle valeur des neurones du groupe il faut afficher dans la fenêtre de même indice (0 : s, 1 : s1, 2 : s2, 3 : pic)                                                                                                      
gint windowPosition[NB_WINDOWS_MAX]; //Position de la fenêtre de même indice dans le bandeau du bas (0 : la plus à gauche)                                                                                                                                                   

gint nbColonnesTotal = 0; //Nombre total de colonnes de neurones dans les fenêtres du bandeau du bas                                                                                                                                                                         
gint nbLignesMax = 0; //Nombre maximal de lignes de neurones à afficher dans l'une des fenêtres du bandeau du bas           

//Sémaphores
sem_t sem_script;


//--------------------------------------------------MAIN-------------------------------------------------------------


/**
 * 
 * name: Programme Principale (Main)
 * 
 * @param argv, argc
 * @see enet_initialize()
 * @see server_for_promethes()
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv)
{
   init_japet(argc, argv);      
   
     // Initialisation de GTK+ 
  gtk_init(&argc, &argv);    
   
   pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL); //La fenêtre    
   /* Positionne la GTK_WINDOW "pWindow" au centre de l'écran */
   gtk_window_set_position(GTK_WINDOW(pWindow), GTK_WIN_POS_CENTER);   
   /* Taille de la fenêtre */
   gtk_window_set_default_size(GTK_WINDOW(pWindow), 2000, 1000);   
   /* Titre de la fenêtre */
   gtk_window_set_title(GTK_WINDOW(pWindow), "Japet");   
   //Le signal de fermeture de la fenêtre est connecté à la fenêtre (petite croix)
   g_signal_connect(G_OBJECT(pWindow), "destroy", G_CALLBACK(Close), (GtkWidget*) pWindow); 
        
    
   /*Création d'une VBox (boîte de widgets disposés verticalement) */
   GtkWidget* pVBox = gtk_vbox_new(FALSE, 0);
   /*ajout de pVBox dans pWindow, qui est alors vu comme un GTK_CONTAINER*/
   gtk_container_add(GTK_CONTAINER(pWindow), pVBox);   
    
   /*Création de deux HBox : une pour le panneau latéral et la zone principale, l'autre pour les 6 petites zones*/
   GtkWidget* pHBox1 = gtk_hbox_new(FALSE, 0);
   pHBox2 = gtk_hbox_new(FALSE, 0);
   gtk_widget_set_size_request(pHBox2, 100, 0); //On n'affiche pas encore les zones "neurones"
   /*ajout de pHBox1 et pHBox2 dans pVBox, qui est alors vu comme un GTK_CONTAINER*/
   //void gtk_box_pack_start(GtkBox* box, GtkWidget* child, gboolean expand, gboolean fill, guint padding);
   gtk_box_pack_start(GTK_BOX(pVBox), pHBox1, TRUE, TRUE, 0);
   gtk_box_pack_start(GTK_BOX(pVBox), pHBox2, FALSE, TRUE, 0);
        
   
   /*Panneau latéral*/
      pPane = gtk_vbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(pHBox1), pPane, FALSE, TRUE, 0);
      //gtk_widget_set_size_request(pPane, 250,100); //Limite la largeur du panneau latéral
      //Les échelles
      GtkWidget* pFrameEchelles = gtk_frame_new("Scales");
      gtk_container_add(GTK_CONTAINER(pPane), pFrameEchelles);
      GtkWidget* pVBoxEchelles = gtk_vbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(pFrameEchelles), pVBoxEchelles);      
      
      //Fréquence de réactualisation de l'affichage, quand on est en mode échantillonné (Sampled)
      GtkWidget* refreshSetting = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshSetting, FALSE, TRUE, 0);      
	GtkWidget* refreshLabel = gtk_label_new("Refresh (Hz):");
	refreshScale = gtk_spin_button_new_with_range(1, 24, 1); //Ce widget est déjà déclaré comme variable globale
	//On choisit le nombre de réactualisations de l'affichage par seconde, entre 1 et 24
	gtk_box_pack_start(GTK_BOX(refreshSetting), refreshLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(refreshSetting), refreshScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(refreshScale), REFRESHSCALE_DEFAULT);
	gtk_signal_connect (GTK_OBJECT(refreshScale), "value-changed", (GtkSignalFunc) changeValue, NULL);   
      
      //Echelle de l'axe des x
      GtkWidget* xSetting = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(pVBoxEchelles), xSetting, FALSE, TRUE, 0);      
	GtkWidget* xLabel = gtk_label_new("x scale:");
	xScale = gtk_spin_button_new_with_range(10, 350, 1); //Ce widget est déjà déclaré comme variable globale
	gtk_box_pack_start(GTK_BOX(xSetting), xLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(xSetting), xScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(xScale), XSCALE_DEFAULT);
	gtk_signal_connect (GTK_OBJECT(xScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

      //Echelle de l'axe des y
      GtkWidget* ySetting = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(pVBoxEchelles), ySetting, FALSE, TRUE, 0);
	GtkWidget* yLabel = gtk_label_new("y scale:");
	yScale = gtk_spin_button_new_with_range(10, 350, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(ySetting), yLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ySetting), yScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(yScale), YSCALE_DEFAULT);
	gtk_signal_connect (GTK_OBJECT(yScale), "value-changed", (GtkSignalFunc) changeValue, NULL);
      
      //Décalage des plans selon x
      GtkWidget* zxSetting = gtk_hbox_new(FALSE, 0);      
      gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zxSetting, FALSE, TRUE, 0);
	GtkWidget* zxLabel = gtk_label_new("x gap:");
	zxScale = gtk_spin_button_new_with_range(0, 50, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(zxSetting), zxLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(zxSetting), zxScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(zxScale), XGAP_DEFAULT);
	gtk_signal_connect (GTK_OBJECT(zxScale), "value-changed", (GtkSignalFunc) changeValue, NULL);
      
      //Décalage des plans selon y
      GtkWidget* zySetting = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zySetting, FALSE, TRUE, 0);
	GtkWidget* zyLabel = gtk_label_new("y gap:");
	zyScale = gtk_spin_button_new_with_range(0, 50, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(zySetting), zyLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(zySetting), zyScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(zyScale), YGAP_DEFAULT);
	gtk_signal_connect (GTK_OBJECT(zyScale), "value-changed", (GtkSignalFunc) changeValue, NULL);
      
      //Hauteur minimale d'un neurone
      GtkWidget* neuronHeight = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(pVBoxEchelles), neuronHeight, FALSE, TRUE, 0);
	GtkWidget* neuronHeightLabel = gtk_label_new("Neuron minimal height:");
	neuronHeightScale = gtk_spin_button_new_with_range(1, 50, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(neuronHeight), neuronHeightLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(neuronHeight), neuronHeightScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(neuronHeightScale), NEURON_HEIGHT_MIN_DEFAULT);
	gtk_signal_connect (GTK_OBJECT(neuronHeightScale), "value-changed", (GtkSignalFunc) changeValue, NULL);	
	
      //Nombre digits pour afficher les valeurs des neurones
      GtkWidget* digits = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(pVBoxEchelles), digits, FALSE, TRUE, 0);
	GtkWidget* digitsLabel = gtk_label_new("Neuron digits:");
	digitsScale = gtk_spin_button_new_with_range(1, 10, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(digits), digitsLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(digits), digitsScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(digitsScale), DIGITS_DEFAULT);
	gtk_signal_connect (GTK_OBJECT(digitsScale), "value-changed", (GtkSignalFunc) changeValue, NULL);
		
      //3 boutons
      GtkWidget* pBoutons = gtk_hbox_new(TRUE, 0);
      gtk_box_pack_start(GTK_BOX(pVBoxEchelles), pBoutons, FALSE, TRUE, 0);
	GtkWidget* boutonSave = gtk_button_new_with_label("Save");	
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonSave, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonSave), "clicked", G_CALLBACK(saveScale), NULL);
        GtkWidget* boutonLoad = gtk_button_new_with_label("Load");
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonLoad, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonLoad), "clicked", G_CALLBACK(loadScale), NULL);
	GtkWidget* boutonDefault = gtk_button_new_with_label("Default");
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonDefault, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonDefault), "clicked", G_CALLBACK(defaultScale), NULL);
      
      //Mode d'affichage      
      #ifdef ETIS
	displayMode = "Sampled mode";
      #else
	displayMode = "Off-line mode";
      #endif
      modeLabel = gtk_label_new(displayMode);
      gtk_container_add(GTK_CONTAINER(pPane), modeLabel);      
	
      //Les scripts
      GtkWidget* pFrameScripts = gtk_frame_new("Open scripts");
      gtk_container_add(GTK_CONTAINER(pPane), pFrameScripts);
	pVBoxScripts = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pFrameScripts), pVBoxScripts);
	GtkWidget* askButton = gtk_button_new_with_label("Ask for scripts");
	gtk_box_pack_start(GTK_BOX(pVBoxScripts), askButton, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(askButton), "clicked", G_CALLBACK(askForScripts), NULL);	
	
	openScripts = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*)); //Il y aura autant de lignes que de scripts ouverts.
	scriptCheck = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*)); //Sur chacune de ces lignes, il y a une case à cocher...
	scriptLabel = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*)); //suivie d'un label...
	zChooser = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*));	  //et d'un "spinbutton",
	//GtkWidget** colorLabel = malloc(nbScripts * sizeof(GtkWidget*));
	//GtkWidget** boutonDel = malloc(nbScripts * sizeof(GtkWidget*)); //et un bouton "croix" pour fermer le script	
	      
   //La zone principale   
   GtkWidget* pFrameGroupes = gtk_frame_new("Neural groups");
   gtk_box_pack_start(GTK_BOX(pHBox1), pFrameGroupes, TRUE, TRUE, 0);
   GtkWidget* scrollbars = gtk_scrolled_window_new(NULL, NULL);
   gtk_container_add(GTK_CONTAINER(pFrameGroupes), scrollbars);
   zone3D = gtk_drawing_area_new(); //Déjà déclarée comme variable globale
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars), zone3D);
   gtk_drawing_area_size(GTK_DRAWING_AREA(zone3D), 3000, 1000);
   gtk_signal_connect (GTK_OBJECT(zone3D), "expose_event", (GtkSignalFunc) expose_event, NULL);   
   gtk_widget_set_events(zone3D, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK); //Détecte quand on appuie OU quand on relache un bouton de la souris alors que le curseur est dans la zone3D   
   gtk_signal_connect (GTK_OBJECT (zone3D), "button_press_event", (GtkSignalFunc) button_press_event, NULL);
   gtk_signal_connect (GTK_OBJECT (pWindow), "key_press_event", (GtkSignalFunc) key_press_event, NULL);            
   
   //Appelle la fonction refresh_display à intervalles réguliers
 /*  g_timeout_add ((guint)(1000 / (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(refreshScale))), refresh_display, NULL);*/
   
   gtk_widget_show_all(pWindow); //Affichage du widget pWindow et de tous ceux qui sont dedans
   
   #ifdef ETIS
   //Initialisation d'ivy 
   prom_bus_init(BROADCAST_IP); 
   #endif
   
   gdk_threads_enter();
   gtk_main(); //Boucle infinie : attente des événements
   gdk_threads_leave();

   return EXIT_SUCCESS;
}


//-------------------------------------------------INITIALISATION-----------------------------------------------

//
/**
 * 
 * 
 * Initialise les bibliothèques GTK et ENet, ainsi que quelques tableaux
 * @param argv, argc
 */
void init_japet(int argc, char** argv)
{
  //Initialisations de tableaux
  gint i;
  for (i = 0; i < NB_SCRIPTS_MAX; i++) Index[i] = i;
  for (i = 0; i < NB_WINDOWS_MAX; i++) windowGroup[i] == NULL;   
  
  //Tableau des scripts
  scr = malloc(NB_SCRIPTS_MAX * sizeof(script));
  
  //sémaphores
  sem_init (&sem_script, 0, 1);

  //Initialisation d'ENet
  #ifdef ETIS  
  if (enet_initialize() != 0) 
  {
    printf("An error occurred while initializing ENet.\n");
    exit(EXIT_FAILURE);
  }
  atexit (enet_deinitialize);
  server_for_promethes();
  #endif
}

//--------------------------------------------------------SIGNAUX---------------------------------------------

void cocheDecoche(GtkWidget* pWidget, gpointer pData)
{
    gint i,j;
    (void)pData;
    //Identification du script à afficher ou à masquer
    
    for (i = 0; i < nbScripts; i++) if (scriptCheck[i] == pWidget) j = i;    
    
    scr[j].displayed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (pWidget));
    expose_event(zone3D, NULL);
}

/**
 * 
 * Fonction de fermeture de Japet
 * 
 * @return EXIT_SUCCESS
 */
void Close(GtkWidget* pWidget, gpointer pData) //Fonction de fermeture de Japet
{
  (void)pWidget;
  (void)pData;
  
  exit(EXIT_SUCCESS);
  
  /*
  //Création d'une boîte de dialogue pour faire confirmer la fermeture
  GtkWidget* pQuestion = gtk_message_dialog_new(GTK_WINDOW(pData),
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						"Do you really want to leave Japet ?");
  switch(gtk_dialog_run(GTK_DIALOG(pQuestion)))
  {
    case GTK_RESPONSE_YES:
      gtk_main_quit(); //Sortie de la boucle gtk_main
      break;
    case GTK_RESPONSE_NONE:
    case GTK_RESPONSE_NO:
      gtk_widget_destroy(pQuestion);
      break;
  }*/
}

/**
 * 
 * Un script change de plan
 * 
 */
void changePlan(GtkWidget* pWidget, gpointer pData) //Un script change de plan
{
    
    gint n = *((gint*)pData);    
    scr[n].z = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(pWidget));
    gtk_label_set_text(GTK_LABEL(scriptLabel[n]), g_strconcat("<span foreground=\"", tcolor(scr[n]), "\"><b>", scr[n].name, "</b></span>", NULL));
    gtk_label_set_use_markup(GTK_LABEL(scriptLabel[n]), TRUE);
    expose_event(zone3D, NULL); //On redessine la grille avec la nouvelle valeur de z    
    
    gint i;
    for (i = 0; i < NB_WINDOWS_MAX; i++) if (windowGroup[i] != NULL) if (windowGroup[i]->myScript == &scr[n]) expose_neurons(zoneNeurones[i], NULL);    
    //Réaffiche le contenu des petites fenêtres montrant des groupes de ce script
}

/**
 * 
 * Modification d'une échelle
 * 
 */
void changeValue(GtkWidget* pWidget, gpointer pData) //Modification d'une échelle
{   
  gint i;
  if(pWidget == neuronHeightScale) resizeNeurons();    
  else if (pWidget == digitsScale) 
  {
    for (i = 0; i < NB_WINDOWS_MAX; i++) 
      if (windowGroup[i] != NULL) 
	expose_neurons(zoneNeurones[i], NULL);      
  }
  else expose_event(zone3D, pData); //On redessine la grille avec la nouvelle échelle 
}

//Clic souris
/**
 * 
 * Clic souris
 * 
 */
void button_press_event(GtkWidget* pWidget, GdkEventButton* event)
{
    //Récupération des échelles
    gint a = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(xScale));
    gint b = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(yScale));
    gint c = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(zxScale));
    gint d = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(zyScale));    
    
    gint i, j, k;
    
    (void)pWidget;
    
    gint aucun = 1; //1 : aucun groupe n'est sélectionné
    selectedWindow = NB_WINDOWS_MAX; //aucune fenêtre n'est sélectionnée
        
    for (k = 0; k <= zMax; k++)
      for (i = 0; i < nbScripts; i++) if (scr[i].z == k && scr[i].displayed == TRUE)
	for (j = 0; j < scr[i].nbGroups; j++)
	{
	  if (event->x > a * scr[i].groups[j].x 
	    + c 
	    * (zMax 
	    - scr[i].z
	    ) 
	    - LARGEUR_GROUPE / 2 &&
	      event->x < a * scr[i].groups[j].x + c * (zMax - scr[i].z) + LARGEUR_GROUPE / 2 &&
	      event->y > b * scr[i].groups[j].y + d * scr[i].z - HAUTEUR_GROUPE / 2 &&
	      event->y < b * scr[i].groups[j].y + d * scr[i].z + HAUTEUR_GROUPE / 2)
	  //Si on a cliqué a l'intérieur du dessin d'un groupe  
	  {
	    selectedGroup = &scr[i].groups[j];
	    aucun = 0;	    //Il y a un groupe sélectionné
	    	       
	    if (event->button == 3) //Si clic droit, on ouvre une nouvelle petite fenêtre affichant les neurones de ce groupe, 
	      if (usedWindows < NB_WINDOWS_MAX) //sauf si le nombre max de fenêtres ouvertes est atteint
		if((selectedGroup->sWindow[0] == NB_WINDOWS_MAX) | //ou si toutes les valeurs des neurones de ce groupe sont déjà affichées	     
		  (selectedGroup->sWindow[1] == NB_WINDOWS_MAX) | 
		  (selectedGroup->sWindow[2] == NB_WINDOWS_MAX) | 
		  (selectedGroup->sWindow[3] == NB_WINDOWS_MAX && selectedGroup->function == 4)) //Regardé seulement si le groupe est une image		
		    newWindow(selectedGroup);	    	    	    
	    else //Si clic gauche dans un groupe
	    {
	      for(i = 0; i < NB_WINDOWS_MAX; i++) if(zoneNeurones[i] != NULL) expose_neurons(zoneNeurones[i], NULL); //On réactualise le contenu des petites fenêtres ouvertes
	    }
	  }
	}	
    
    if (aucun == 1) selectedGroup = NULL; //Si on a cliqué en dehors de tout groupe
    
            
    //On réactualise l'affichage
    for(i = 0; i < NB_WINDOWS_MAX; i++) if(zoneNeurones[i] != NULL) expose_neurons(zoneNeurones[i], NULL);              
    expose_event(zone3D, NULL);    
}

/**
 * 
 * Appui sur une touche
 * 
 */
void key_press_event (GtkWidget* pWidget, GdkEventKey *event)
{
  gint i, j;
  gint libre = 0, voisine;  
  gint sauts = 1; 
  (void)pWidget;
   
  switch (event->keyval)
  {    
    case GDK_Up: //Flèche haut, on réduit le y du groupe sélectionné pour le faire monter      
      if (selectedGroup != NULL)
      {	
	do
	{
	  libre = 1;
	  for (i = 0; i < nbScripts; i++) if (scr[i].z == selectedGroup->myScript->z && scr[i].displayed == TRUE) //Pour chaque script dans le même plan que le groupe sélectionné
	    for (j = 0; j < scr[i].nbGroups; j++) if (scr[i].groups[j].x == selectedGroup->x && scr[i].groups[j].y == (selectedGroup->y - sauts))
	    {
	      libre = 0; //La case n'est pas libre
	      sauts++; //Le groupe doit faire un saut de plus
	    }	    
	} while (libre == 0); //Tant qu'on n'a pas trouvé une case libre      	
	selectedGroup->y -= sauts;
	expose_event(zone3D, NULL); 
      }
      break;
      
    case GDK_Down: //Flèche bas, on augmente le y du groupe sélectionné pour le faire descendre
      if (selectedGroup != NULL)
      {
	do
	{
	  libre = 1;
	  for (i = 0; i < nbScripts; i++) if (scr[i].z == selectedGroup->myScript->z && scr[i].displayed == TRUE) for (j = 0; j < scr[i].nbGroups; j++) 
	    if (scr[i].groups[j].x == selectedGroup->x && scr[i].groups[j].y == (selectedGroup->y + sauts))
	    {
	      libre = 0; //La case n'est pas libre
	      sauts++; //Le groupe doit faire un saut de plus
	    }	    
	} while (libre == 0); //Tant qu'on n'a pas trouvé une case libre      
	selectedGroup->y += sauts;
	expose_event(zone3D, NULL); 
      }
      break;
      
    case GDK_Left: //Flèche gauche, on décale la petite fenêtre sélectionnée d'un cran vers la gauche    
      if (selectedWindow < NB_WINDOWS_MAX && windowPosition[selectedWindow] > 0) //S'il y a bien une fenêtre sélectionnée et si ce n'est pas déjà la plus à gauche
      {
	//On cherche le numéro de sa voisine de gauche
	for (i = 0; i < NB_WINDOWS_MAX; i++)
	if (windowPosition[i] == windowPosition[selectedWindow] - 1) 
	{
	  voisine = i;
	  break;
	}
	//On déplace la fenêtre
	gtk_box_reorder_child(GTK_BOX(pHBox2), pFrameNeurones[selectedWindow], windowPosition[selectedWindow] - 1);
	//On enregistre les nouvelles positions
	windowPosition[selectedWindow]--;
	windowPosition[voisine]++;	
	
	//Et on réaffiche toutes les fenêtres
	for (i = 0; i < NB_WINDOWS_MAX; i++) if (windowGroup[i] != NULL) expose_neurons(zoneNeurones[i], NULL);	  
	gtk_widget_show_all(pHBox2); //Réaffichage de tout le bandeau des Neurones	
      }
      break;
      
    case GDK_Right: //Flèche droite, on décale la petite fenêtre sélectionnée d'un cran vers la droite      
      if (selectedWindow < NB_WINDOWS_MAX && windowPosition[selectedWindow] < usedWindows - 1) //S'il y a bien une fenêtre sélectionnée et si ce n'est pas déjà la plus à droite
      {
	//On cherche le numéro de sa voisine de droite
	for (i = 0; i < NB_WINDOWS_MAX; i++)
	if (windowPosition[i] == windowPosition[selectedWindow] + 1) 
	{
	  voisine = i;
	  break;
	}
	//On déplace la fenêtre
	gtk_box_reorder_child(GTK_BOX(pHBox2), pFrameNeurones[selectedWindow], windowPosition[selectedWindow] + 1);
	//On enregistre les nouvelles positions
	windowPosition[selectedWindow]++;
	windowPosition[voisine]--;	
	
	//Et on réaffiche toutes les fenêtres
	for (i = 0; i < NB_WINDOWS_MAX; i++) if (windowGroup[i] != NULL) expose_neurons(zoneNeurones[i], NULL);	  
	gtk_widget_show_all(pHBox2); //Réaffichage de tout le bandeau des Neurones	
      }
      break;
      
    default:; //Appui sur une autre touche. On ne fait rien.
  }  
}


/**
 * 
 * Réactualisation de la zone 3D
 * 
 */
void expose_event (GtkWidget* zone3D, gpointer pData)
{
  cairo_t* cr = gdk_cairo_create(zone3D->window); //Crée un contexte Cairo associé à la drawing_area "zone"
  (void)pData;
  
  //On commence par dessiner un grand rectangle blanc
  cairo_set_source_rgb (cr, WHITE);
  cairo_rectangle (cr, 0, 0, zone3D->allocation.width, zone3D->allocation.height);
  cairo_fill (cr);
  
  gint a = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(xScale));
  gint b = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(yScale));
  gint c = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(zxScale));
  gint d = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(zyScale));
  
  gint i,j, k;
      
  //On recalcule zMax, la plus grande valeur de z parmi les scripts ouverts
  zMax = 0;
  for (i = 0; i < nbScripts; i++) if (scr[i].z > zMax && scr[i].displayed == TRUE) zMax = scr[i].z;      
  
  //Dessin des groupes : on dessine d'abord les scripts du plan z = 0 (s'il y en a), puis ceux du plan z = 1, etc., jusqu'à zMax inclus
  printf("début du dessin des groupes\n");
  for (k = 0; k <= zMax; k++)  
    for (i = 0; i < nbScripts; i++)
      if (scr[i].z == k && scr[i].displayed == TRUE) for (j = 0; j < scr[i].nbGroups; j++)	
	dessinGroupe(cr, a, b, c, d, &scr[i].groups[j], scr[i].z, zMax);
      
  
  cairo_set_source_rgb(cr, BLACK); //On va dessiner en noir
  cairo_set_line_width (cr, GRID_WIDTH); //Épaisseur du trait
  
  gint abscisse, ordonnee, zxSetting, zySetting; 
  
  //Dessin de la grille
  for (k = 0; k <= zMax; k++)
  {
      zxSetting = c * k;
      zySetting = d * k;
  
      //Lignes verticales
      i = 0;  
      do
      {
	abscisse = a * i + zxSetting;
	cairo_move_to(cr, abscisse, 0);
	cairo_line_to(cr, abscisse, zone3D->allocation.height);
	i++;
      } while (abscisse < zone3D->allocation.width);
  
      //Lignes horizontales
      j = 0;  
      do
      {
	ordonnee = b * j + zySetting;
	cairo_move_to(cr, 0, ordonnee);
	cairo_line_to(cr, zone3D->allocation.width, ordonnee);
	j++;
      } while (ordonnee < zone3D->allocation.height);      
  }
  
      //Lignes obliques
      gint I = i, J = j;    
      
      for(i = 0; i < I; i++)
	for(j = 0; j < J; j++)
	{	  
	  cairo_move_to(cr, a * i + c * zMax, b * j);
	  cairo_line_to(cr, a * i, b * j + d * zMax);	  
	}   
  
  cairo_stroke(cr);  //Le contenu de cr est appliqué sur "zone"   
  cairo_destroy(cr); //Puis, on détruit cr
} 

/**
 * 
 * Enregistrer les échelles dans un fichier
 * 
 */
void saveScale(GtkWidget* pWidget, gpointer pData)
{
  GtkWidget* dialog = gtk_file_chooser_dialog_new ("Save your scales",
						   GTK_WINDOW(pWindow),
						   GTK_FILE_CHOOSER_ACTION_SAVE,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						   NULL);
  (void)pWidget;
  (void)pData;
						   
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);  
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char* filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    saveJapetConfigToFile(filename);
    g_free (filename);
  }
  
  gtk_widget_destroy (dialog);  
}


/**
 * 
 * Ouvrir un fichier d'échelles
 * 
 */
void loadScale(GtkWidget* pWidget, gpointer pData)
{
  GtkWidget* dialog = gtk_file_chooser_dialog_new ("Open scales file",
						   GTK_WINDOW(pWindow),
						   GTK_FILE_CHOOSER_ACTION_OPEN,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						   NULL);
  
  (void)pWidget;
  (void)pData;
						   
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char* filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    loadJapetConfigToFile(filename);
    g_free (filename);
  }
  gtk_widget_destroy (dialog);  
  
  refresh_display();
  
}

void defaultScale(GtkWidget* pWidget, gpointer pData)
{  
  (void)pWidget;
  (void)pData;
  
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(refreshScale), REFRESHSCALE_DEFAULT);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(xScale), XSCALE_DEFAULT);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(yScale), YSCALE_DEFAULT);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(zxScale), XGAP_DEFAULT);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(zyScale), YGAP_DEFAULT);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(neuronHeightScale), NEURON_HEIGHT_MIN_DEFAULT);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(digitsScale), DIGITS_DEFAULT);
  expose_event(zone3D, NULL); //On redessine la grille avec la nouvelle échelle
}


/**
 * 
 * Quand on clique sur le bouton "Ask for scripts", Japet envoie un message broadcast à tous les Promethe, leur demandant d'envoyer leurs scripts
 * S'il y avait déjà des scripts dans la mémoire de Japet, ils sont effacés
 * 
 */
void askForScripts(GtkWidget* pWidget, gpointer pData)
{
  (void)pWidget;
  (void)pData;
  
  destroyAllScripts(); //Supprime tous les scripts précédents  
  expose_event(zone3D, NULL);
  
  gint i,j;     
#ifdef ETIS     
  japet_bus_send_message("japet(%d)", JAPET_START);  
  gtk_label_set_text(GTK_LABEL(modeLabel), "Waiting for groups");  
  gtk_widget_show_all(pPane);
  sleep(RECEPTION_DELAY);
  gtk_label_set_text(GTK_LABEL(modeLabel), "Sampled mode");  
  gtk_widget_show_all(pPane);
  //sem_wait(&sem_script);  
#else
  //-------------EXEMPLES DE SCRIPTS---------------------

    //scr = malloc(3 * sizeof(script)); //Tableau des scripts à afficher   
    
    
    //Création des 3 scripts
    newScript(&scr[0], "Ex 1", "192.168.0.14", 0, 4);
    newScript(&scr[1], "Ex 2", "192.168.0.15", 1, 3);
    newScript(&scr[2], "Debug", "192.168.0.16", 2, 1);
        
    //Groupes du script 0
    newGroup(&scr[0].groups[0], &scr[0], "SCR0GR0", 1, 0.1, 16, 4, 4, 2, 0);
    newGroup(&scr[0].groups[1], &scr[0], "SCR0GR1", 1, 0.1, 9, 3, 3, 2, 1);
    scr[0].groups[1].previous[0] = &scr[0].groups[0];	
    newGroup(&scr[0].groups[2], &scr[0], "SCR0GR2", 1, 0.1, 12, 4, 3, 2, 1);
    scr[0].groups[2].previous[0] = &scr[0].groups[1]; 	
    newGroup(&scr[0].groups[3], &scr[0], "SCR0GR3", 1, 0.1, 20, 4, 5, 3, 2);
    scr[0].groups[3].previous[0] = &scr[0].groups[1];
    scr[0].groups[3].previous[1] = &scr[1].groups[1]; 
    
    //Groupes du script 1
    newGroup(&scr[1].groups[0], &scr[1], "SCR1GR0", 1, 0.1, 16, 4, 4, 2, 0);
    newGroup(&scr[1].groups[1], &scr[1], "SCR1GR1", 1, 0.1, 9, 3, 3, 2, 1);
    scr[1].groups[1].previous[0] = &scr[1].groups[0];
    newGroup(&scr[1].groups[2], &scr[1], "SCR1GR2", 1, 0.1, 12, 4, 3, 3, 1);
    scr[1].groups[2].previous[0] = &scr[1].groups[1];      
    
    //Groupes du script 2
    newGroup(&scr[2].groups[0], &scr[2], "SCR2GR0", 4, 0.1, 16, 4, 4, 3, 1);
    scr[2].groups[0].previous[0] = &scr[1].groups[1];
    
    //Neurones d'un groupe - Pour l'instant, ce sont les mêmes pour tous les groupes   

    for (i = 0; i < nbScripts; i++)        
     {
      newNeuron(&scr[i].groups[0].neurons[0], &scr[i].groups[0], 12, 5, 78, 25, 0, 0);
      newNeuron(&scr[i].groups[0].neurons[1], &scr[i].groups[0], 108, -54, 1, 12, 1, 0);
      newNeuron(&scr[i].groups[0].neurons[2], &scr[i].groups[0], 81, 201, 7, -3, 2, 0);
      newNeuron(&scr[i].groups[0].neurons[3], &scr[i].groups[0], 132, 96, 8, 16, 3, 0);
      newNeuron(&scr[i].groups[0].neurons[4], &scr[i].groups[0], 48, 55, -16, 1, 0, 1);
      newNeuron(&scr[i].groups[0].neurons[5], &scr[i].groups[0], -120, 75, 178, -30, 1, 1);
      newNeuron(&scr[i].groups[0].neurons[6], &scr[i].groups[0], 182, 45, 28, -7, 2, 1);
      newNeuron(&scr[i].groups[0].neurons[7], &scr[i].groups[0], -52, 167, 72, 0, 3, 1);
      newNeuron(&scr[i].groups[0].neurons[8], &scr[i].groups[0], 97, 24, 16, 0, 0, 2);
      newNeuron(&scr[i].groups[0].neurons[9], &scr[i].groups[0], 12, 0, -5, 43, 1, 2);
      newNeuron(&scr[i].groups[0].neurons[10], &scr[i].groups[0], 132, 96, 8, -15, 2, 2);
      newNeuron(&scr[i].groups[0].neurons[11], &scr[i].groups[0], 12, 5, 78, 12, 3, 2);
      newNeuron(&scr[i].groups[0].neurons[12], &scr[i].groups[0], -52, 167, 72, 9, 0, 3);
      newNeuron(&scr[i].groups[0].neurons[13], &scr[i].groups[0], 12, 5, 78, -2, 1, 3);
      newNeuron(&scr[i].groups[0].neurons[14], &scr[i].groups[0], 147, 231, 198, -14, 2, 3);      
      newNeuron(&scr[i].groups[0].neurons[15], &scr[i].groups[0], 147, 231, 198, 5, 3, 3);
            
      if (scr[i].nbGroups > 1)
      {
	newNeuron(&scr[i].groups[1].neurons[0], &scr[i].groups[0], 12, 5, 78, 25, 0, 0);
	newNeuron(&scr[i].groups[1].neurons[1], &scr[i].groups[0], 108, -54, 1, 12, 1, 0);
	newNeuron(&scr[i].groups[1].neurons[2], &scr[i].groups[0], 81, 201, 7, -3, 2, 0);
	newNeuron(&scr[i].groups[1].neurons[3], &scr[i].groups[0], 132, 96, 8, 16, 0, 1);
	newNeuron(&scr[i].groups[1].neurons[4], &scr[i].groups[0], 48, 55, -16, 1, 1, 1);
	newNeuron(&scr[i].groups[1].neurons[5], &scr[i].groups[0], -120, 75, 178, -30, 2, 1);
	newNeuron(&scr[i].groups[1].neurons[6], &scr[i].groups[0], 182, 45, 28, -7, 0, 2);
	newNeuron(&scr[i].groups[1].neurons[7], &scr[i].groups[0], -52, 167, 72, 0, 1, 2);
	newNeuron(&scr[i].groups[1].neurons[8], &scr[i].groups[0], 97, 24, 16, 0, 2, 2);	
      }
     }    
#endif      
  
  //Calcul du x de chaque groupe  
  for (i = 0; i < nbScripts; i++)   
     for (j = 0; j < scr[i].nbGroups; j++)
     {
       if (scr[i].groups[j].knownX == FALSE) findX(&scr[i].groups[j]);
     }
   
  //On veut déterminer zMax, la plus grande valeur de z parmi les scripts ouverts
  printf("Calcul de zMax\n");
  zMax = 0;  
  for (i = 0; i < nbScripts; i++) if (scr[i].z > zMax) zMax = scr[i].z;   
  
  //On remplit le panneau des scripts
  printf("début du panneau des scripts\n");
  for (i = 0; i < nbScripts; i++)
  {
    openScripts[i] = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pVBoxScripts), openScripts[i], FALSE, TRUE, 0);
	  
    //Texte = g_locale_to_utf8("<span foreground=\"#73b5ff\"><b>%s</b></span>\n", -1, NULL, NULL, NULL);	  
	  
    //Cases à cocher pour afficher les scripts ou pas
    scriptCheck[i] = gtk_check_button_new();		  
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (scriptCheck[i]), FALSE); //Par défaut, les scripts ne sont pas cochés, donc pas affichés
    g_signal_connect(G_OBJECT(scriptCheck[i]), "toggled", G_CALLBACK(cocheDecoche), &i);
    gtk_box_pack_start(GTK_BOX(openScripts[i]), scriptCheck[i], FALSE, TRUE, 0);
	  
    //Labels des scripts : le nom du script écrit en gras, de la couleur du script
    scriptLabel[i] = gtk_label_new(g_strconcat("<span foreground=\"", tcolor(scr[i]), "\"><b>", scr[i].name, "</b></span>", NULL));
    gtk_label_set_use_markup(GTK_LABEL(scriptLabel[i]), TRUE);
    gtk_box_pack_start(GTK_BOX(openScripts[i]), scriptLabel[i], TRUE, TRUE, 0);	  	  
	  
    zChooser[i] = gtk_spin_button_new_with_range(0, nbScripts, 1); //Choix du plan dans lequel on affiche le script. On n'a pas besoin de plus de plans qu'il n'y a de scripts	  
    gtk_box_pack_start(GTK_BOX(openScripts[i]), zChooser[i], FALSE, TRUE, 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(zChooser[i]), scr[i].z);	  	  
    gtk_signal_connect (GTK_OBJECT(zChooser[i]), "value-changed", (GtkSignalFunc) changePlan, &Index[i]);
    //On envoie &Index[i] et pas &i car la valeur à l'adresse &i aura changé quand on recevra le signal	  
	  
    //boutonDel[i] = gtk_image_new_from_stock(GTK_STOCK_CLOSE, 2);
    //gtk_box_pack_start(GTK_BOX(openScripts[i]), boutonDel[i], FALSE, TRUE, 0);
  }		   
  printf("fin du panneau des scripts\n");
  //expose_event(zone3D, NULL);
  gtk_widget_show_all(pWindow); //Affichage du widget pWindow et de tous ceux qui sont dedans
  printf("fin de askForScripts()\n");
}

void expose_neurons(GtkWidget* zone2D, gpointer pData)
{
  //On cherche le numéro de la fenêtre à afficher  
  gint i, j, currentWindow;
  
  for (i = 0; i < NB_WINDOWS_MAX; i++)
    if (zoneNeurones[i] == zone2D) 
    {
      currentWindow = i;
      break;
    }
  
  //On va chercher l'adresse du groupe à afficher dans le tableau windowGroup
  group* g = windowGroup[currentWindow];    
  
  //On détermine la plus grande valeur à afficher (valMax) et la plus petite (valMax). valMax s'affichera en blanc et valMin en noir.
  gfloat valMax, valMin;     
  gint wV = windowValue[currentWindow]; //Abréviation
  valMax = g->neurons[0].s[wV];
  valMin = g->neurons[0].s[wV];  
  for (i = 1; i < g->nbNeurons; i++)
  {
    if (g->neurons[i].s[wV] > valMax) valMax = g->neurons[i].s[wV];
    if (g->neurons[i].s[wV] < valMin) valMin = g->neurons[i].s[wV];
  } 
  
  //Dimensions de la fenêtre
  gint largeur = zone2D->allocation.width;
  gint hauteur = zone2D->allocation.height;  
  //Dimensions d'un neurone
  gint largeurNeurone = largeur / g->columns;
  gint hauteurNeurone = hauteur / g->rows;  
  
  //Début du dessin
  cairo_t* cr = gdk_cairo_create(zone2D->window); //Crée un contexte Cairo associé à la drawing_area "zone"
  
  //On commence par dessiner un grand rectangle couleur sable
  cairo_set_source_rgb (cr, SAND);    
  cairo_rectangle (cr, 0, 0, largeur, hauteur);
  cairo_fill (cr);       
  
  //Affichage des neurones
  gfloat ndg;
  gint nbDigits = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(digitsScale));
  
  for (i = 0; i < g->nbNeurons; i++)    
  {
    ndg = niveauDeGris(g->neurons[i].s[wV], valMin, valMax);
    cairo_set_source_rgb (cr, ndg, ndg, ndg);
    cairo_rectangle(cr, g->neurons[i].x * largeurNeurone, g->neurons[i].y * hauteurNeurone, largeurNeurone, hauteurNeurone);  
    cairo_fill(cr);
    
    if (largeurNeurone > 4 * (nbDigits + 1) && hauteurNeurone > 15)
    {
      if (ndg > 0.5) color(cr, *g);
      else clearColor(cr, *g);
      cairo_move_to(cr, g->neurons[i].x * largeurNeurone + 4, (g->neurons[i].y + 0.5) * hauteurNeurone);
      gchar valeurNeurone[nbDigits + 1];      
      sprintf(valeurNeurone, "%f", g->neurons[i].s[wV]);
      valeurNeurone[nbDigits] = '\0';
      if (g->neurons[i].s[wV] >= pow(10, nbDigits) || g->neurons[i].s[wV] <= - pow(10, nbDigits-1)) for (j = 0; j < nbDigits; j++) valeurNeurone[j] = '#';
      //Si le nombre de digits demandé est insuffisant pour afficher entièrement la partie entière d'une valeur, des # s'affichent à la place de cette valeur
      cairo_show_text(cr, valeurNeurone);
    }
  }
  
  cairo_stroke(cr);  //Le contenu de cr est appliqué sur "zoneNeurones"     
  
  if (windowGroup[currentWindow] == selectedGroup) //Si le groupe affiché dans cette fenêtre est sélectionné
  {
    cairo_set_line_width (cr, 10); //Traits plus épais
    if (currentWindow == selectedWindow) cairo_set_source_rgb (cr, RED);    
    else cairo_set_source_rgb (cr, INDIGO);    
    cairo_move_to(cr, 0, hauteur);
    cairo_line_to(cr, largeur, hauteur);    
    cairo_stroke(cr);  //Le contenu de cr est appliqué sur "zoneNeurones"   
    cairo_set_line_width (cr, GRID_WIDTH); //Retour aux traits fins 
  }    
  cairo_destroy(cr); //Destruction du calque
}

void button_press_neurons(GtkWidget* zone2D, GdkEventButton* event)
{
    //On cherche le numéro de la fenêtre concernée  
    gint i, currentWindow;     
    for (i = 0; i < NB_WINDOWS_MAX; i++)
      if (zoneNeurones[i] == zone2D) 
      {
	currentWindow = i;
	break;
      }
      
    group* g = windowGroup[currentWindow]; //On va chercher l'adresse du groupe concerné
    gint wV = windowValue[currentWindow]; //et le type de valeur que cette fenêtre affiche     
  
    if (event->button == 3) //Si clic droit dans une petite fenêtre, on la supprime
    {    
      g->sWindow[wV] = NB_WINDOWS_MAX; //Cette valeur, pour ce groupe, n'est plus affichée
      windowGroup[currentWindow] = NULL; //Plus aucun groupe n'est associé à cette fenêtre
      
      if (currentWindow == selectedWindow) selectedWindow = NB_WINDOWS_MAX; //Si on supprime la fenêtre sélectionnée, il n'y a plus de fenêtre sélectionnée
        
      gtk_container_remove (GTK_CONTAINER(pHBox2), pFrameNeurones[currentWindow]); //On efface la fenêtre    
      zoneNeurones[currentWindow] = NULL;
      pFrameNeurones[currentWindow] = NULL;      
      usedWindows--;
      if (usedWindows == 0) gtk_widget_set_size_request(pHBox2, 100, 0); //S'il n'y a plus de petite fenêtre ouverte, on referme le bandeau du bas    
    }  
    else //Si clic gauche, la fenêtre est sélectionnée
    {
	selectedWindow = currentWindow;		
	selectedGroup = g; //Le groupe associé à la fenêtre dans laquelle on a cliqué est également sélectionné
    }  
    
    //On actualise l'affichage
    expose_event(zone3D, NULL);
    for(i = 0; i < NB_WINDOWS_MAX; i++) if(zoneNeurones[i] != NULL) expose_neurons(zoneNeurones[i], NULL);
}  


//-----------------------------------------------------"CONSTRUCTEURS"---------------------------------------------

void newScript(script* s, gchar* name, gchar* machine, gint z, gint nbGroups)
{
    s->name = name;    
    s->machine = machine;
    s->z = z;
    s->nbGroups = nbGroups;
    s->groups = malloc(nbGroups * sizeof(group));
    s->displayed = FALSE;
    
    printf("Création du script %s (envoyé par %s) dans le plan %d. Il comporte %d groupes.\n", s->name, s->machine, s->z, s->nbGroups);
    nbScripts++;    
}

//
/**
 * 
 * Destruction des groupes du script à l'adresse s (ne pas appeler ailleurs que dans destroyAllScripts())
 * 
 * @param s à supprimer
 * 
 */
void destroyScript(script* s)
{
   gint i;   
   for (i = 0; i < s->nbGroups; i++) 
   {     
     if (s->groups[i].neurons != NULL) free(s->groups[i].neurons);    
   }    
   free(s->groups);
   
   nbScripts--;
}

//
/**
 * 
 * Destruction de tous les scripts
 * @see destroyScript()
 * 
 */
void destroyAllScripts()
{
  while (nbScripts > 0) 
  {
    gtk_widget_destroy(zChooser[nbScripts - 1]);
    gtk_widget_destroy(scriptLabel[nbScripts - 1]);
    gtk_widget_destroy(scriptCheck[nbScripts - 1]);
    gtk_widget_destroy(openScripts[nbScripts - 1]);
    destroyScript(&scr[nbScripts - 1]);   
  }
  free(scr);  
  
  //Il faut aussi supprimer toutes les petites fenêtres éventuellement ouverte
  gint i;
  for (i = 0; i < NB_WINDOWS_MAX; i++) if (pFrameNeurones[i] != NULL) 
  {
    gtk_container_remove (GTK_CONTAINER(pHBox2), pFrameNeurones[i]);
    windowGroup[i] = NULL;
    zoneNeurones[i] = NULL;
    pFrameNeurones[i] = NULL;
  }  
  usedWindows = 0;
  gtk_widget_set_size_request(pHBox2, 100, 0);  
  
  newScriptNumber = 0;
  ivyServerNb = 0;
}

/**
 * 
 * Créer un script. 
 * 
 * sWindow: Toutes les cases du script sont initialisées à NB_WINDOWS_MAX
 */
void newGroup(group* g, script* myScript, gchar* name, gint function, gfloat learningSpeed, gint nbNeurons, gint rows, gint columns, gint y, gint nbLinksTo)
{
  g->myScript = myScript;
  g->name = name;
  g->function = function;
  g->learningSpeed = learningSpeed;  
  g->nbNeurons = nbNeurons;
  g->rows = rows;
  g->columns = columns;
  g->neurons = malloc(nbNeurons * sizeof(neuron));
  g->y = y;
  g->knownX = FALSE;
  g->knownY = TRUE;
  g->nbLinksTo = nbLinksTo;
  
  if (nbLinksTo == 0) g->previous = NULL;
  else g->previous = malloc(nbLinksTo * sizeof(group*));
  
  gint i;
  for (i = 0; i < 4; i++) g->sWindow[i] = NB_WINDOWS_MAX;
  //Toutes les cases sont initialisées à NB_WINDOWS_MAX, ce qui signifie que cette valeur n'est encore affichée dans aucune petite fenêtre
  
  g->justRefreshed = FALSE;
}

/**
 * 
 * Créer un nouveau neurone
 */
void newNeuron(neuron* n, group* myGroup, gfloat s, gfloat s1, gfloat s2, gfloat pic, gint x, gint y)
{
  n->myGroup = myGroup;
  n->s[0] = s;
  n->s[1] = s1;
  n->s[2] = s2;
  n->s[3] = pic;
  n->x = x;
  n->y = y;  
}


/**
 * 
 * Mise un jour d'un neurone quand Prométhé envoie de nouvelles données
 * 
 */
void updateNeuron(neuron* n, gfloat s, gfloat s1, gfloat s2, gfloat pic)
{
  n->s[0] = s;
  n->s[1] = s1;
  n->s[2] = s2;
  n->s[3] = pic;
}



/**
 * 
 * Mise un jour d'un groupe quand Prométhé envoie de nouvelles données
 * 
 */
void updateGroup(group* g, gfloat learningSpeed, gfloat execTime)
{
  g->learningSpeed = learningSpeed;
}
  



//-----------------------------------------------------AUTRES FONCTIONS---------------------------------------------


gboolean refresh_display()
{
  if (pWindow == NULL) return FALSE;
  else
  {
    expose_event(zone3D, NULL); //On redessine la grille avec la nouvelle échelle
    //Et on réaffiche toutes les fenêtres
    gint i;
    for (i = 0; i < NB_WINDOWS_MAX; i++) if (windowGroup[i] != NULL) expose_neurons(zoneNeurones[i], NULL);	  
    gtk_widget_show_all(pHBox2);
    return TRUE;
  }
}
  
gchar* tcolor(script S)
{
  switch(S.z)
  {
    case 0 :
	return TDARKGREEN;
    case 1 :
	return TDARKBLUE;
    case 2 :
	return TDARKYELLOW;
    case 3 :
	return TDARKMAGENTA;
    case 4 :
	return TDARKCYAN;
    case 5 :
	return TDARKORANGE;
    case 6 :
	return TDARKGREY;    
  }
}
  
/**
 * 
 * Donne au pinceau la couleur foncée correspondant à une certaine valeur de z
 * 
 */
void color(cairo_t* cr, group g) 
{
  switch (g.myScript->z) //La couleur d'un groupe ou d'une liaison dépend du plan dans lequel il/elle se trouve
  {
    case 0 :
	cairo_set_source_rgb (cr, DARKGREEN);
	break;
    case 1 :
	cairo_set_source_rgb (cr, DARKBLUE);
	break;
    case 2 :
	cairo_set_source_rgb (cr, DARKYELLOW);
	break;
    case 3 :
	cairo_set_source_rgb (cr, DARKMAGENTA);
	break;    
    case 4 :
	cairo_set_source_rgb (cr, DARKCYAN);
	break;    
    case 5 :
	cairo_set_source_rgb (cr, DARKORANGE);
	break;    
    case 6 :
	cairo_set_source_rgb (cr, DARKGREY);
	break;       
  }
}

/**
 * 
 * Donne au pinceau la couleur claire correspondant à une certaine valeur de z
 * 
 */
void clearColor(cairo_t* cr, group g) 
{
  switch (g.myScript->z) //La couleur d'un groupe ou d'une liaison dépend du plan dans lequel il/elle se trouve
  {
    case 0 :
	cairo_set_source_rgb (cr, GREEN);	
	break;
    case 1 :
	cairo_set_source_rgb (cr, BLUE);
	break;
    case 2 :
	cairo_set_source_rgb (cr, YELLOW);
	break;
    case 3 :
	cairo_set_source_rgb (cr, MAGENTA);
	break;    
    case 4 :
	cairo_set_source_rgb (cr, CYAN);
	break;    
    case 5 :
	cairo_set_source_rgb (cr, ORANGE);	
	break;    
    case 6 :
	cairo_set_source_rgb (cr, GREY);
	break;       
  }
}

void dessinGroupe (cairo_t* cr, gint a, gint b, gint c, gint d, group* g, gint z, gint zMax)
{
  gint x = g->x, y = g->y;  
  
  if (g == selectedGroup) cairo_set_source_rgb (cr, RED);
  else color(cr, *g);  
  cairo_rectangle (cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z - HAUTEUR_GROUPE / 2, LARGEUR_GROUPE, HAUTEUR_GROUPE);
  cairo_fill (cr);   
    
  cairo_set_source_rgb (cr, WHITE);
  cairo_set_font_size(cr, 8);
  cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z);
  cairo_show_text(cr, g->name);
  cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z + HAUTEUR_GROUPE / 2);  
  gchar strGroupType[1];
  sprintf(strGroupType, "%d", g->function);
  cairo_show_text(cr, strGroupType);
  
  gint i;
  
  //Dessin des liaisons aboutissant à ce groupe
  for (i = 0; i < g->nbLinksTo; i++) if(g->previous[i]->myScript->displayed == TRUE)
  {
    gint x1 = g->previous[i]->x, y1 = g->previous[i]->y, z1 = g->previous[i]->myScript->z; //Coordonnées du groupe situé avant la liaison
    
    if (g->previous[i]->myScript == g->myScript) color(cr, *g);
    else cairo_set_source_rgb (cr, RED); //Si c'est une liaison entre deux groupes appartenant à des scripts différents, elle est dessinée en rouge
    
    cairo_set_line_width (cr, 0.8); //Trait épais représentant une liaison entre groupes
  
    cairo_move_to(cr, a * x1 + c * (zMax - z1) + LARGEUR_GROUPE / 2, b * y1 + d * z1); //Début de la liaison (à droite du groupe prédécesseur)
    cairo_line_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z); //Fin de la liaison (à gauche du groupe courant)
  
    cairo_stroke(cr);
  }  
}

/**
 * 
 * Calcule la coordonnée x d'un groupe en fonction de ses prédecesseurs
 * 
 */
void findX(group* g)
{
    int i, Max = 0;
    
    if (g->previous == NULL) g->x = 1; 
    //Si ce groupe ne dépend d'aucun autre, on le met tout à gauche
    else
    {
      for (i = 0; i < g->nbLinksTo; i++)
      {
	if (g->previous[i]->knownX == FALSE) findX(g->previous[i]);
	if (g->previous[i]->x > Max) Max = g->previous[i]->x;
      }
      g->x = Max + 1;
    }
    g->knownX = TRUE;    
}

void newWindow(group* g)
{
  gint i, currentWindow;  //On cherche la première case vide dans le tableau windowGroup et on donne son numéro à la nouvelle fenêtre
  for (i = 0; i < NB_WINDOWS_MAX; i++) if (windowGroup[i] == NULL) 
  {
    currentWindow = i;    
    break;
  }    
  
  windowGroup[currentWindow] = g; //On enregistre l'adresse du groupe affiché dans cette fenêtre
  windowPosition[currentWindow] = usedWindows; //et la position de cette fenêtre dans le bandeau
  usedWindows++; //Puis on incrémente le nombre de fenêtres utilisées  
  
  //On détermine si cette fenêtre va afficher les s, les s1, les s2 ou les pic des neurones des groupes  
  gchar* valeurAAfficher;  
  if (g->sWindow[0] == NB_WINDOWS_MAX) 
  {
    valeurAAfficher = "s";    
    windowValue[currentWindow] = 0;
  }
  else if (g->sWindow[1] == NB_WINDOWS_MAX)
  {
    valeurAAfficher = "s1";    
    windowValue[currentWindow] = 1;
  }
  else if (g->sWindow[2] == NB_WINDOWS_MAX) 
  {
    valeurAAfficher = "s2";    
    windowValue[currentWindow] = 2;
  }
  else 
  {
    valeurAAfficher = "pic";    
    windowValue[currentWindow] = 3;
  }   
  g->sWindow[windowValue[currentWindow]] = currentWindow; //On enregistre que c'est CETTE fenêtre qui affiche CETTE valeur des neurones de CE groupe
    
  //Création de la petite fenêtre     
  pFrameNeurones[currentWindow] = gtk_frame_new(g_strconcat(g->name, " - ", valeurAAfficher, NULL)); //Titre de la fenêtre
  gtk_container_add(GTK_CONTAINER(pHBox2), pFrameNeurones[currentWindow]);    
       
  //On met à jour nbLignesMax et nbColonnesTotal
  if (g->rows > nbLignesMax) nbLignesMax = g->rows;
  nbColonnesTotal += g->columns;
  
  zoneNeurones[currentWindow] = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(pFrameNeurones[currentWindow]), zoneNeurones[currentWindow]);      
  gtk_widget_add_events(zoneNeurones[currentWindow], GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect (GTK_OBJECT (zoneNeurones[currentWindow]), "button_press_event", (GtkSignalFunc) button_press_neurons, NULL);   
  
  gint hauteurNeuroneMin = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(neuronHeightScale));
  gtk_widget_set_size_request(pHBox2, 100, hauteurNeuroneMin * (nbLignesMax)); //On adapte la hauteur du bandeau du bas.
  
  resizeNeurons();
  
  gtk_signal_connect(GTK_OBJECT(zoneNeurones[currentWindow]), "expose_event", (GtkSignalFunc) expose_neurons, NULL);       
}

gfloat niveauDeGris(gfloat val, gfloat valMin, gfloat valMax)
{
  gfloat ndg = (val - valMin) / (valMax - valMin);
  return ndg;
}

void resizeNeurons() 
{
  gint hauteurNeuroneMin = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(neuronHeightScale));
  gtk_widget_set_size_request(pHBox2, 100, hauteurNeuroneMin * (nbLignesMax)); //On adapte la hauteur du bandeau du bas.    
    
  //On partage la largeur de la grande fenêtre Japet entre les petites fenêtres ouvertes, en fonction du nombre de colonnes de neurones dans chacune  
  gint i;
  gint largeurJapet = pHBox2->allocation.width; //Largeur totale de la grande fenêtre Japet  
  for (i = 0; i < NB_WINDOWS_MAX; i++) if (windowGroup[i] != NULL)
  {
    gtk_widget_set_size_request(zoneNeurones[i], largeurJapet * windowGroup[i]->columns / nbColonnesTotal, hauteurNeuroneMin * (nbLignesMax));  
    //expose_neurons(zoneNeurones[i], NULL);
  }  
  gtk_widget_show_all(pHBox2); //Réaffichage de tout le bandeau des Neurones
}  

void saveJapetConfigToFile(char* filename) 
{
  #define DEBUG_VAR 1
  #ifndef DEBUG_VAR
  printf("Fonction saveJapetConfigToFile \n");
  //Récupération des valeurs à sauvegarder
  printf("valeur du refreshScale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(refreshScale)));
  printf("valeur du xScale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(yScale)));
  printf("valeur du yScale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(xScale)));  
  printf("valeur du gap zxScale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zyScale)));
  printf("valeur du gap zyScale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zxScale)));
  printf("valeur du neuronHeightScale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(neuronHeightScale)));
  printf("valeur du digitsScale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(digitsScale)));
  #endif
  
  FILE* f = fopen(filename,"w");

	  //fwrite(buff,sizeof(char),sizeof(buff),f);
	  fprintf(f,"%s\n","refreshScale");
	  fprintf(f,"%d\n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(refreshScale)));
	  fprintf(f,"%s\n","xScale");
	  fprintf(f,"%d\n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(xScale)));
	  fprintf(f,"%s\n","yScale");
	  fprintf(f,"%d\n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(yScale)));	  
	  fprintf(f,"%s\n","zxScale");
	  fprintf(f,"%d\n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zxScale)));
	  fprintf(f,"%s\n","zyScale");
	  fprintf(f,"%d\n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zyScale)));	  
	  fprintf(f,"%s\n","neuronHeightScale");
	  fprintf(f,"%d\n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(neuronHeightScale)));
	  fprintf(f,"%s\n","digitsScale");
	  fprintf(f,"%d\n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(digitsScale)));
	  fclose(f); 
  
}

void loadJapetConfigToFile(char* filename) 
{

  gint l_refreshScale = 0;
  gint l_xscale = 0;
  gint l_yscale = 0;
  gint l_zxscale = 0;
  gint l_zyscale = 0;
  gint l_neuronHeightScale = 0;
  gint l_digitsScale = 0;

  char buffer[100]; 
  char* buffer2;

  #ifdef DEBUG_VAR
  printf("Fonction saveJapetConfigToFile \n");
  //Récupération des valeurs à sauvegarder
  printf("valeur du xscale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(yScale)));
  printf("valeur du yscale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(xScale)));  
  printf("valeur du gap zxscale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zyScale)));
  printf("valeur du gap zyscale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zxScale)));
  printf("valeur du neuronHeight spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(neuronHeightScale)));
  printf("valeur du digitsScale spin_button: %d \n",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(digitsScale)));
  #endif
  
  

  FILE* f = fopen(filename,"r");
  printf(">READ FILE\n");
  if(f != NULL) {
	  
	  while(fscanf(f,"%s",buffer) != EOF){
	     
		  printf("> buffer read: --%s--	\n", buffer);
		  
		  if (strcmp(buffer, "refreshScale") == 0){
			  fscanf(f, "%d\n", &l_refreshScale);
		  }
		  else if (strcmp(buffer,"xScale") == 0){
			  fscanf(f,"%d\n",&l_xscale);
		  }
		  else if (strcmp(buffer,"yScale")==0){
			  // yscale value
			  fscanf(f,"%d\n",&l_yscale);
		  }
		  else if (strcmp(buffer,"zxScale")==0){
			  //zxscale
			  fscanf(f,"%d\n",&l_zxscale);
		  }
		  else if (strcmp(buffer,"zyScale")==0){
			  //zyscale
			  fscanf(f,"%d\n",&l_zyscale);
		  }
		  else if (strcmp(buffer,"neuronHeightScale")==0){
			  //neuronHeightScale
			  fscanf(f,"%d\n",&l_neuronHeightScale);
		  }
		  else if (strcmp(buffer,"digitsScale")==0){
			  //digitsScale
			  fscanf(f,"%d\n",&l_digitsScale);
		  }
	  }
	  fclose(f);	
 #ifdef DEBUG_VAR
	  printf("Fonction saveJapetConfigToFile \n");
	  printf("arguments read from config file: %d,%d,%d,%d,%d,%d\n",l_xscale,l_yscale,l_zxscale,l_zyscale,l_neuronHeightScale,l_digitsScale);
 #endif
 
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(xScale), l_xscale);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(yScale), l_yscale);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(zxScale), l_zxscale);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(zyScale), l_zyscale);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(neuronHeightScale), l_neuronHeightScale);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(digitsScale), l_digitsScale);
  } 
  
}

void fatal_error(const char *name_of_file, const char* name_of_function,  int numero_of_line, const char *message, ...)
{
  char total_message[MESSAGE_MAX];
  GtkWidget *dialog;

  va_list arguments;
  va_start(arguments, message);
  vsnprintf(total_message, MESSAGE_MAX, message, arguments);
  va_end(arguments);

  dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s \t %s \t %i :\n \t Error: %s \n", name_of_file, name_of_function, numero_of_line, total_message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
