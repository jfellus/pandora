/* japet.c
 * 
 *
 * Auteurs : Brice Errandonea et Manuel De Palma
 * 
 * Pour compiler : make
 * 
 * 
 */
#include "japet.h"

//--------------------------------------------------1. VARIABLES GLOBALES----------------------------------------------------

GtkWidget* pWindow; //La fenêtre de l'application Japet
GtkWidget *hide_see_scales_button, *menu_bar;
GtkWidget* pPane; //Panneau latéral
GtkWidget* pVBoxScripts; //Panneau des scripts
GtkWidget* zone3D; //La grande zone de dessin des liaisons entre groupes
GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale, *neuronHeightScale, *digitsScale; //Échelles

//Indiquent quel est le mode d'affichage en cours (Off-line, Sampled ou Snapshots)
gchar* displayMode;
GtkWidget* modeLabel;
int currentSnapshot;
int nbSnapshots;

int Index[NB_SCRIPTS_MAX]; //Tableau des indices : Index[0] = 0, Index[1] = 1, ..., Index[NB_SCRIPTS_MAX-1] = NB_SCRIPTS_MAX-1;
//Ce tableau permet à une fonction signal de retenir la valeur qu'avait i au moment où on a connecté le signal                                                                                                                                                               

GtkWidget** openScripts; //Lignes du panneau des scripts                                                                                                                                                                                                                     
GtkWidget** scriptCheck; //Cases à cocher/décocher pour afficher les scripts ou les masquer                                                                                                                                                                                  
GtkWidget** scriptLabel; //Label affichant le nom d'un script dans la bonne couleur                                                                                                                                                                                          
GtkWidget** zChooser; //Spin_button pour choisir dans quel plan afficher ce script
GtkWidget** searchButton; //Boutton pour rechercher un groupe dans ce script

int nbScripts = 0; //Nombre de scripts à afficher
char scriptsNames[NB_SCRIPTS_MAX][SCRIPT_NAME_MAX]; //Tableau des noms des scripts
script scr[NB_SCRIPTS_MAX]; //Tableau des scripts à afficher                                                                                                                                                                                                                                
/*int newScriptNumber = 0; */
//Numéro donné à un script quand on l'ouvre
int zMax; //la plus grande valeur de z parmi les scripts ouverts
int buffered = 0; //Nombre d'instantanés actuellement en mémoire

int nb_update = 0;

int usedWindows = 0;
group* selectedGroup = NULL; //Pointeur sur le groupe actuellement sélectionné                                                                                                                                                                                               
int selectedWindow = NB_WINDOWS_MAX; //Numéro de la fenêtre sélectionnée (entre 0 et NB_WINDOWS_MAX-1). NB_WINDOWS_MAX indique qu'aucune fenêtre n'est sélectionnée

//GtkWidget* h_box_neuron; //Panneau des neurones
GtkWidget* neurons_frame, *zone_neurons;
GtkWidget* pFrameNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de petites fenêtres pour les neurones                                                                                                                                                         
GtkWidget* zoneNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de zones où dessiner des neurones                                                                                                                                                              
group* windowGroup[NB_WINDOWS_MAX]; //Adresse des groupe affiché dans la zoneNeurones de même indice                                                                                                                                                                         
int windowValue[NB_WINDOWS_MAX]; //Numéro disant quelle valeur des neurones du groupe il faut afficher dans la fenêtre de même indice (0 : s, 1 : s1, 2 : s2, 3 : pic)
int windowPosition[NB_WINDOWS_MAX]; //Position de la fenêtre de même indice dans le bandeau du bas (0 : la plus à gauche)

int nbColonnesTotal = 0; //Nombre total de colonnes de neurones dans les fenêtres du bandeau du bas
int nbLignesMax = 0; //Nombre maximal de lignes de neurones à afficher dans l'une des fenêtres du bandeau du bas

int move_neurons_old_x, move_neurons_old_y, move_neurons_start = 0;
GtkWidget *move_neurons_frame = NULL;
//Sémaphores
//sem_t sem_script;

void drag_drop_neuron_frame(GtkWidget *zone_neurons, GdkEventButton *event, gpointer data);
void expose_event_zone_neuron(GtkWidget* zone_neurons, gpointer pData);

//--------------------------------------------------2. MAIN-------------------------------------------------------------


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

	g_thread_init(NULL);
	gdk_threads_init();

	// Initialisation de GTK+
	gtk_init(&argc, &argv);

	init_japet(argc, argv);

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
	GtkWidget* v_box_main = gtk_vbox_new(FALSE, 0);
	/*ajout de v_box_main dans pWindow, qui est alors vu comme un GTK_CONTAINER*/
	gtk_container_add(GTK_CONTAINER(pWindow), v_box_main);

	hide_see_scales_button = gtk_toggle_button_new_with_label("Hide scales");
	g_signal_connect(G_OBJECT(hide_see_scales_button), "toggled", (GtkSignalFunc) on_hide_see_scales_button_active, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_see_scales_button), FALSE);
	gtk_box_pack_start(GTK_BOX(v_box_main), hide_see_scales_button, FALSE, FALSE, 0);

	/*Création de deux HBox : une pour le panneau latéral et la zone principale, l'autre pour les 6 petites zones*/
	GtkWidget* h_box_main = gtk_hbox_new(FALSE, 0);
	GtkWidget* vpaned = gtk_vpaned_new();
	//h_box_neuron = gtk_hbox_new(FALSE, 0);
	//gtk_widget_set_size_request(h_box_neuron, 100, 0); //On n'affiche pas encore les zones "neurones"
	neurons_frame = gtk_frame_new("Neurons' frame");
	/*ajout de h_box_main et h_box_neuron dans v_box_main, qui est alors vu comme un GTK_CONTAINER*/
	//void gtk_box_pack_start(GtkBox* box, GtkWidget* child, gboolean expand, gboolean fill, guint padding);
	gtk_box_pack_start(GTK_BOX(v_box_main), h_box_main, TRUE, TRUE, 0);

	/*Panneau latéral*/
	pPane = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_main), pPane, FALSE, TRUE, 0);
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
	gtk_signal_connect(GTK_OBJECT(refreshScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Echelle de l'axe des x
	GtkWidget* xSetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), xSetting, FALSE, TRUE, 0);
	GtkWidget* xLabel = gtk_label_new("x scale:");
	xScale = gtk_spin_button_new_with_range(10, 350, 1); //Ce widget est déjà déclaré comme variable globale
	gtk_box_pack_start(GTK_BOX(xSetting), xLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(xSetting), xScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(xScale), XSCALE_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(xScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Echelle de l'axe des y
	GtkWidget* ySetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), ySetting, FALSE, TRUE, 0);
	GtkWidget* yLabel = gtk_label_new("y scale:");
	yScale = gtk_spin_button_new_with_range(10, 350, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(ySetting), yLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ySetting), yScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(yScale), YSCALE_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(yScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Décalage des plans selon x
	GtkWidget* zxSetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zxSetting, FALSE, TRUE, 0);
	GtkWidget* zxLabel = gtk_label_new("x gap:");
	zxScale = gtk_spin_button_new_with_range(0, 50, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(zxSetting), zxLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(zxSetting), zxScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(zxScale), XGAP_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(zxScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Décalage des plans selon y
	GtkWidget* zySetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zySetting, FALSE, TRUE, 0);
	GtkWidget* zyLabel = gtk_label_new("y gap:");
	zyScale = gtk_spin_button_new_with_range(0, 50, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(zySetting), zyLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(zySetting), zyScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(zyScale), YGAP_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(zyScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Hauteur minimale d'un neurone
	GtkWidget* neuronHeight = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), neuronHeight, FALSE, TRUE, 0);
	GtkWidget* neuronHeightLabel = gtk_label_new("Neuron minimal height:");
	neuronHeightScale = gtk_spin_button_new_with_range(1, 50, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(neuronHeight), neuronHeightLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(neuronHeight), neuronHeightScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(neuronHeightScale), NEURON_HEIGHT_MIN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(neuronHeightScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Nombre digits pour afficher les valeurs des neurones
	GtkWidget* digits = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), digits, FALSE, TRUE, 0);
	GtkWidget* digitsLabel = gtk_label_new("Neuron digits:");
	digitsScale = gtk_spin_button_new_with_range(1, 10, 1); //Ce widget est déjà déclaré comme variable globale 
	gtk_box_pack_start(GTK_BOX(digits), digitsLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(digits), digitsScale, TRUE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(digitsScale), DIGITS_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(digitsScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//3 boutons
	GtkWidget* pBoutons = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), pBoutons, FALSE, TRUE, 0);
	GtkWidget* boutonSave = gtk_button_new_with_label("Save");
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonSave, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonSave), "clicked", G_CALLBACK(japet_save_preferences), NULL);
	GtkWidget* boutonLoad = gtk_button_new_with_label("Load");
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonLoad, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonLoad), "clicked", G_CALLBACK(japet_load_preferences), NULL);
	GtkWidget* boutonDefault = gtk_button_new_with_label("Default");
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonDefault, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonDefault), "clicked", G_CALLBACK(defaultScale), NULL);

	displayMode = "Sampled mode";

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
	zChooser = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*)); //et d'un "spinbutton",
	searchButton = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*));
	//GtkWidget** colorLabel = malloc(nbScripts * sizeof(GtkWidget*));
	//GtkWidget** boutonDel = malloc(nbScripts * sizeof(GtkWidget*)); //et un bouton "croix" pour fermer le script	

	//La zone principale
	GtkWidget* pFrameGroupes = gtk_frame_new("Neural groups");
	gtk_container_add(GTK_CONTAINER(vpaned), pFrameGroupes); //gtk_box_pack_start(GTK_BOX(VBox), h_box_neuron, FALSE, TRUE, 0);
	GtkWidget* scrollbars = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(pFrameGroupes), scrollbars);
	zone3D = gtk_drawing_area_new(); //Déjà déclarée comme variable globale
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars), zone3D);
	gtk_drawing_area_size(GTK_DRAWING_AREA(zone3D), 3000, 3000);
	gtk_signal_connect(GTK_OBJECT(zone3D), "expose_event", (GtkSignalFunc) expose_event, NULL);
	gtk_widget_set_events(zone3D, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK); //Détecte quand on appuie OU quand on relache un bouton de la souris alors que le curseur est dans la zone3D
	gtk_signal_connect(GTK_OBJECT(zone3D), "button_press_event", (GtkSignalFunc) button_press_event, NULL);
	gtk_signal_connect(GTK_OBJECT(pWindow), "key_press_event", (GtkSignalFunc) key_press_event, NULL);

	gtk_container_add(GTK_CONTAINER(vpaned), neurons_frame);
	GtkWidget *scrollbars2 = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(neurons_frame), scrollbars2);
	zone_neurons = gtk_layout_new(NULL, NULL);
	gtk_widget_set_size_request(GTK_WIDGET(zone_neurons), 3000, 3000);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars2), zone_neurons);
	gtk_widget_add_events(zone_neurons, GDK_ALL_EVENTS_MASK);
	//	g_signal_connect(GTK_OBJECT(zone_neurons), "expose-event", (GtkSignalFunc) expose_event_zone_neuron, NULL);
	g_signal_connect(GTK_OBJECT(zone_neurons), "button-release-event", (GtkSignalFunc) drag_drop_neuron_frame, NULL);

	gtk_box_pack_start(GTK_BOX(h_box_main), vpaned, TRUE, TRUE, 0);

	//Appelle la fonction refresh_display à intervalles réguliers si on est en mode échantillonné ('a' est la deuxième lettre de "Sampled mode")
	if (displayMode[1] == 'a') g_timeout_add((guint)(1000 / (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(refreshScale))), refresh_display, NULL);

	gtk_widget_show_all(pWindow); //Affichage du widget pWindow et de tous ceux qui sont dedans

	prom_bus_init(BROADCAST_IP);

	gdk_threads_enter();
	gtk_main(); //Boucle infinie : attente des événements
	gdk_threads_leave();

	return EXIT_SUCCESS;
}

//-------------------------------------------------3. INITIALISATION-----------------------------------------------
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
	int i;

	(void) argc;
	(void) argv;

	for (i = 0; i < NB_SCRIPTS_MAX; i++)
		Index[i] = i;
	for (i = 0; i < NB_WINDOWS_MAX; i++)
		windowGroup[i] = NULL;

	//Tableau des scripts
	for (i = 0; i < NB_SCRIPTS_MAX; i++)
		scr[i].z = -4; //Les scripts non créés sont placés dans le plan -4

	//Initialisation d'ENet
	if (enet_initialize() != 0)
	{
		printf("An error occurred while initializing ENet.\n");
		exit(EXIT_FAILURE);
	}
	atexit(enet_deinitialize);
	server_for_promethes();

	nbSnapshots = 1; //Par défaut, 1 série de valeurs est enregistrée à leur création dans le tableau "buffer" de chaque neurone
}

//-----------------------------------------------------4. SIGNAUX---------------------------------------------

void cocheDecoche(GtkWidget* pWidget, gpointer pData)
{
	int i;
	(void) pData;

	//Identification du script à afficher ou à masquer
	for (i = 0; i < nbScripts; i++)
		if (scriptCheck[i] == pWidget) break;

	scr[i].displayed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));

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
	(void) pWidget;
	(void) pData;

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

	int n = *((int*) pData);
	int l = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(pWidget));
	scr[n].z = l;
	gtk_label_set_text(GTK_LABEL(scriptLabel[n]), g_strconcat("<span foreground=\"", tcolor(scr[n]), "\"><b>", scr[n].name, "</b></span>", NULL));
	gtk_label_set_use_markup(GTK_LABEL(scriptLabel[n]), TRUE);

	expose_event(zone3D, NULL);//On redessine la grille avec la nouvelle valeur de z

	int i;
	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (windowGroup[i] != NULL) if (windowGroup[i]->myScript == &scr[n]) expose_neurons(zoneNeurones[i], NULL);
	//Réaffiche le contenu des petites fenêtres montrant des groupes de ce script
}

/**
 * 
 * Modification d'une échelle
 * 
 */
void changeValue(GtkWidget* pWidget, gpointer pData) //Modification d'une échelle
{
	int i;

	(void) pData;

	if (pWidget == digitsScale)
	{
		for (i = 0; i < NB_WINDOWS_MAX; i++)
			if (windowGroup[i] != NULL) expose_neurons(zoneNeurones[i], NULL);
	}
	else
	{
		expose_event(zone3D, NULL);//On redessine la grille avec la nouvelle échelle
	}
}

void on_search_group_button_active(GtkWidget* pWidget, script *script)
{
	(void) pWidget;

	GtkWidget *search_dialog, *search_entry;
	int i;

	search_dialog = gtk_dialog_new_with_buttons("Recherche d'un groupe", GTK_WINDOW(pWindow), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_window_set_default_size(GTK_WINDOW(search_dialog), 300, 75);

	search_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(search_entry), "Saisissez le nom du groupe recherché");

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(search_dialog)->vbox), search_entry, TRUE, FALSE, 0);

	gtk_widget_show_all(GTK_DIALOG(search_dialog)->vbox);

	selectedGroup = NULL;

	switch (gtk_dialog_run(GTK_DIALOG(search_dialog)))
	{
	case GTK_RESPONSE_OK:
		for (i = 0; i < script->nbGroups; i++)
		{
			if (strcmp(gtk_entry_get_text(GTK_ENTRY(search_entry)), script->groups[i].name) == 0)
			{
				selectedGroup = &script->groups[i];
				break;
			}
		}
		break;
	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_NONE:
	default:
		break;
	}

	gtk_widget_destroy(search_dialog);
	expose_event(zone3D, NULL);
}

void on_hide_see_scales_button_active(GtkWidget* hide_see_scales_button, gpointer pData)
{
	(void) pData;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_see_scales_button)))
	{
		gtk_widget_hide(pPane);
		gtk_button_set_label(GTK_BUTTON(hide_see_scales_button), "Show scales");
	}
	else
	{
		gtk_widget_show(pPane);
		gtk_button_set_label(GTK_BUTTON(hide_see_scales_button), "Hide scales");
	}
}

//Clic souris
/**
 * 
 * Clic souris
 * 
 */
void button_press_event(GtkWidget* pWidget, GdkEventButton* event)
{
	int script_id, group_id, k;
	int neuron_zone_id;
	//Récupération des échelles
	int a = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(xScale));
	int b = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(yScale));
	int c = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(zxScale));
	int d = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(zyScale));

	(void) pWidget;

	int aucun = 1; //1 : aucun groupe n'est sélectionné
	selectedWindow = NB_WINDOWS_MAX; //aucune fenêtre n'est sélectionnée

	for (k = 0; k <= zMax; k++)
		for (script_id = 0; script_id < nbScripts; script_id++)
			if (scr[script_id].z == k && scr[script_id].displayed == TRUE) for (group_id = 0; group_id < scr[script_id].nbGroups; group_id++)
			{
				if (event->x > a * scr[script_id].groups[group_id].x + c * (zMax - scr[script_id].z) - LARGEUR_GROUPE / 2 && event->x < a * scr[script_id].groups[group_id].x + c * (zMax - scr[script_id].z) + LARGEUR_GROUPE / 2 && event->y > b
						* scr[script_id].groups[group_id].y + d * scr[script_id].z - HAUTEUR_GROUPE / 2 && event->y < b * scr[script_id].groups[group_id].y + d * scr[script_id].z + HAUTEUR_GROUPE / 2)
				//Si on a cliqué a l'intérieur du dessin d'un groupe
				{
					selectedGroup = &scr[script_id].groups[group_id];
					aucun = 0; //Il y a un groupe sélectionné

					if (event->button == 3)
					{ //Si clic droit, on ouvre une nouvelle petite fenêtre affichant les neurones de ce groupe,
						if (usedWindows < NB_WINDOWS_MAX)//sauf si le nombre max de fenêtres ouvertes est atteint
						{
							if (selectedGroup->sWindow == NB_WINDOWS_MAX) //ou si toutes les valeurs des neurones de ce groupe sont déjà affichées
							{
								newWindow(selectedGroup);
								japet_bus_send_message("japet(%d,%d) %s", JAPET_START_GROUP, group_id, scr[script_id].name);
							}
						}
					}
					else //Si clic gauche dans un groupe
					{
						for (neuron_zone_id = 0; neuron_zone_id < NB_WINDOWS_MAX; neuron_zone_id++)
							if (zoneNeurones[neuron_zone_id] != NULL) expose_neurons(zoneNeurones[neuron_zone_id], NULL); //On réactualise le contenu des petites fenêtres ouvertes
					}
				}
			}

	if (aucun == 1) selectedGroup = NULL; //Si on a cliqué en dehors de tout groupe


	//On réactualise l'affichage
	for (k = 0; k < NB_WINDOWS_MAX; k++)
		if (zoneNeurones[k] != NULL) expose_neurons(zoneNeurones[k], NULL);
	expose_event(zone3D, NULL);

}

/**
 *
 * Appui sur une touche
 *
 */
void key_press_event(GtkWidget* pWidget, GdkEventKey *event)
{
	int i, j;
	int libre = 0/*, voisine*/;
	int sauts = 1;
	(void) pWidget;

	switch (event->keyval)
	{
	case GDK_Up: //Touche "Page up", on réduit le y du groupe sélectionné pour le faire monter
		if (selectedGroup != NULL)
		{
			do
			{
				libre = 1;
				for (i = 0; i < nbScripts; i++)
					if (scr[i].z == selectedGroup->myScript->z && scr[i].displayed == TRUE) //Pour chaque script dans le même plan que le groupe sélectionné
					for (j = 0; j < scr[i].nbGroups; j++)
						if (scr[i].groups[j].x == selectedGroup->x && scr[i].groups[j].y == (selectedGroup->y - sauts))
						{
							libre = 0; //La case n'est pas libre
							sauts++; //Le groupe doit faire un saut de plus
						}
			} while (libre == 0); //Tant qu'on n'a pas trouvé une case libre
			selectedGroup->y -= sauts;
			expose_event(zone3D, NULL);
		}
		break;

	case GDK_Down: //Touche "Page down", on augmente le y du groupe sélectionné pour le faire descendre
		if (selectedGroup != NULL)
		{
			do
			{
				libre = 1;
				for (i = 0; i < nbScripts; i++)
					if (scr[i].z == selectedGroup->myScript->z && scr[i].displayed == TRUE) for (j = 0; j < scr[i].nbGroups; j++)
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
		if (selectedWindow < NB_WINDOWS_MAX) //S'il y a bien une fenêtre sélectionnée et si ce n'est pas déjà la plus à gauche
		{
			gtk_layout_move(GTK_LAYOUT(zone_neurons), pFrameNeurones[selectedWindow], 0, 0);
		}
		break;

	case GDK_Right: //Flèche droite, on décale la petite fenêtre sélectionnée d'un cran vers la droite
		/*if (selectedWindow < NB_WINDOWS_MAX && windowPosition[selectedWindow] < usedWindows - 1) //S'il y a bien une fenêtre sélectionnée et si ce n'est pas déjà la plus à droite
		 {
		 //On cherche le numéro de sa voisine de droite
		 for (i = 0; i < NB_WINDOWS_MAX; i++)
		 if (windowPosition[i] == windowPosition[selectedWindow] + 1)
		 {
		 voisine = i;
		 break;
		 }
		 //On déplace la fenêtre
		 gtk_box_reorder_child(GTK_BOX(h_box_neuron), pFrameNeurones[selectedWindow], windowPosition[selectedWindow] + 1);
		 //On enregistre les nouvelles positions
		 windowPosition[selectedWindow]++;
		 windowPosition[voisine]--;

		 //Et on réaffiche toutes les fenêtres
		 for (i = 0; i < NB_WINDOWS_MAX; i++)
		 if (windowGroup[i] != NULL) expose_neurons(zoneNeurones[i], NULL);
		 gtk_widget_show_all(h_box_neuron); //Réaffichage de tout le bandeau des Neurones
		 }*/
		break;
		/*
		 *Les touches "espace", "plus" et "moins" sont destinées au mode "instantanés"
		 *
		 *
		 case GDK_space: //Barre d'espace : on change de mode.

		 printf("%s\n", displayMode);
		 if (displayMode[1] == 'n')
		 {
		 displayMode = "Sampled mode";
		 nbSnapshots = 0;
		 gtk_label_set_text(GTK_LABEL(modeLabel), "Sampled mode");
		 }
		 else if (displayMode[1] == 'a')
		 {
		 displayMode = "Snapshots mode";
		 currentSnapshot = 0;
		 sprintf(strCurrent, "%d", currentSnapshot + 1);
		 sprintf(strTotal, "%d", nbSnapshots);
		 gtk_label_set_text(GTK_LABEL(modeLabel), g_strconcat(strCurrent, "/", strTotal, NULL));
		 }
		 printf("On a changé de mode\n");
		 refresh_display();
		 break;

		 case GDK_minus: //En mode instantannés, on revient à l'instantané précédent
		 if (displayMode[1] == 'n') //Si on est en mode instantanés ('n' est la deuxième lettre de "Snapshots mode")
		 {
		 currentSnapshot--;
		 refresh_display();
		 }
		 break;

		 case GDK_plus: //En mode instantanés, on passe à l'instantané suivant
		 if (displayMode[1] == 'n') //Si on est en mode instantanés ('n' est la deuxième lettre de "Snapshots mode")
		 {
		 currentSnapshot++;
		 refresh_display();
		 }
		 break;
		 */
	default:
		; //Appui sur une autre touche. On ne fait rien.
	}
}

void expose_event_zone_neuron(GtkWidget* zone_neurons, gpointer pData)
{
	(void) pData;

	cairo_t* cr = gdk_cairo_create(zone_neurons->window);

	cairo_set_source_rgb(cr, WHITE);
	cairo_rectangle(cr, 0, 0, 3000, 3000);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, RED); //On va dessiner en noir
	cairo_set_line_width(cr, GRID_WIDTH); //Épaisseur du trait

	int i;
	for (i = 0; i < 3000; i += 10)
	{
		cairo_move_to(cr, 0, i);
		cairo_line_to(cr, 3000, i);
	}

	for (i = 0; i < 3000; i += 10)
	{
		cairo_move_to(cr, i, 0);
		cairo_line_to(cr, i, 3000);
	}

	cairo_stroke(cr); //Le contenu de cr est appliqué sur "zone"
	cairo_destroy(cr);
}

/**
 * 
 * Réactualisation de la zone 3D
 * 
 */
void expose_event(GtkWidget* zone3D, gpointer pData)
{
	cairo_t* cr = gdk_cairo_create(zone3D->window); //Crée un contexte Cairo associé à la drawing_area "zone"
	(void) pData;

	//On commence par dessiner un grand rectangle blanc
	cairo_set_source_rgb(cr, WHITE);
	cairo_rectangle(cr, 0, 0, zone3D->allocation.width, zone3D->allocation.height);
	cairo_fill(cr);

	int a = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(xScale));
	int b = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(yScale));
	int c = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(zxScale));
	int d = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(zyScale));

	int i, j, k;

	//On recalcule zMax, la plus grande valeur de z parmi les scripts ouverts
	zMax = 0;
	for (i = 0; i < nbScripts; i++)
		if (scr[i].z > zMax && scr[i].displayed == TRUE) zMax = scr[i].z;

	//Dessin des groupes : on dessine d'abord les scripts du plan z = 0 (s'il y en a), puis ceux du plan z = 1, etc., jusqu'à zMax inclus

	for (k = 0; k <= zMax; k++)
	{
		for (i = 0; i < nbScripts; i++)
		{
			if (scr[i].z == k && scr[i].displayed == TRUE)
			{
				for (j = 0; j < scr[i].nbGroups; j++)
				{
					dessinGroupe(cr, a, b, c, d, &scr[i].groups[j], scr[i].z, zMax);
				}
			}
		}
	}

	cairo_set_source_rgb(cr, GREY); //On va dessiner en noir
	cairo_set_line_width(cr, GRID_WIDTH); //Épaisseur du trait

	int abscisse, ordonnee, zxSetting, zySetting;

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
	int I = i, J = j;

	for (i = 0; i < I; i++)
		for (j = 0; j < J; j++)
		{
			cairo_move_to(cr, a * i + c * zMax, b * j);
			cairo_line_to(cr, a * i, b * j + d * zMax);
		}

	cairo_stroke(cr); //Le contenu de cr est appliqué sur "zone"
	cairo_destroy(cr); //Puis, on détruit cr
}

/**
 * 
 * Enregistrer les échelles dans un fichier
 * 
 */
void japet_save_preferences(GtkWidget* pWidget, gpointer pData)
{
	GtkWidget* dialog = gtk_file_chooser_dialog_new("Save your scales", GTK_WINDOW(pWindow), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	int i, test;
	(void) pWidget;
	(void) pData;

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		for (i = 0; filename[i] != '\0'; i++)
		{
			if (filename[i] == '.')
			{
				filename[i + 1] = 'j';
				filename[i + 2] = 'a';
				filename[i + 3] = 'p';
				filename[i + 4] = '\0';
				test = 1;
			}
		}

		if (test == 0) rename(filename, strcat(filename, ".jap"));
		saveJapetConfigToFile(filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
}

/**
 * 
 * Ouvrir un fichier d'échelles
 * 
 */
void japet_load_preferences(GtkWidget* pWidget, gpointer pData)
{
	GtkWidget* dialog = gtk_file_chooser_dialog_new("Open scales file", GTK_WINDOW(pWindow), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	(void) pWidget;
	(void) pData;

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		loadJapetConfigToFile(filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);

	refresh_display();

}

void defaultScale(GtkWidget* pWidget, gpointer pData)
{
	(void) pWidget;
	(void) pData;

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
 * @param GtkWidget* pWidget : pointeur sur le bouton "Ask for scripts", gpointer pData : donnée éventuelle passée à la fonction (non utilisée) 
 * @see destroyAllScripts() japet_bus_send_message() findY() findX()
 */
void askForScripts(GtkWidget* pWidget, gpointer pData)
{
	(void) pWidget;
	(void) pData;

	if (nbScripts > 0) destroyAllScripts();//Supprime tous les scripts précédents

	expose_event(zone3D, NULL);

	japet_bus_send_message("japet(%d,0)", JAPET_START);
}

void update_neurons_display(int script_id, int neuronGroupId)
{
	int i;

	for (i = 0; i < NB_WINDOWS_MAX; i++)
	{
		if (windowGroup[i] == &scr[script_id].groups[neuronGroupId])
		{
			gdk_threads_enter();
			expose_neurons(zoneNeurones[i], NULL);
			gdk_flush();
			gdk_threads_leave();
		}
	}
}

void update_script_display(int script_id)
{
	int i;

	/**Calcul du x de chaque groupe
	 */
	for (i = 0; i < scr[script_id].nbGroups; i++)
	{
		if (scr[script_id].groups[i].knownX == FALSE) findX(&scr[script_id].groups[i]);
	}

	/**Calcul du y de chaque groupe
	 */
	for (i = 0; i < scr[script_id].nbGroups; i++)
	{
		if (scr[script_id].groups[i].knownY == FALSE) findY(&scr[script_id].groups[i]);
	}

	/**On veut déterminer zMax, la plus grande valeur de z parmi les scripts ouverts
	 */
	zMax = nbScripts;
	scr[script_id].z = script_id;

	/**On remplit le panneau des scripts
	 */
	gdk_threads_enter();

	openScripts[script_id] = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxScripts), openScripts[script_id], FALSE, TRUE, 0);

	//Cases à cocher pour afficher les scripts ou pas
	scriptCheck[script_id] = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), FALSE); //Par défaut, les scripts ne sont pas cochés, donc pas affichés
	g_signal_connect(G_OBJECT(scriptCheck[script_id]), "toggled", G_CALLBACK(cocheDecoche), &script_id);
	gtk_box_pack_start(GTK_BOX(openScripts[script_id]), scriptCheck[script_id], FALSE, TRUE, 0);

	//Labels des scripts : le nom du script écrit en gras, de la couleur du script
	scriptLabel[script_id] = gtk_label_new(g_strconcat("<span foreground=\"", tcolor(scr[script_id]), "\"><b>", scr[script_id].name, "</b></span>", NULL));
	gtk_label_set_use_markup(GTK_LABEL(scriptLabel[script_id]), TRUE);
	gtk_box_pack_start(GTK_BOX(openScripts[script_id]), scriptLabel[script_id], TRUE, TRUE, 0);

	zChooser[script_id] = gtk_spin_button_new_with_range(0, nbScripts - 1, 1); //Choix du plan dans lequel on affiche le script. On n'a pas besoin de plus de plans qu'il n'y a de scripts
	gtk_box_pack_start(GTK_BOX(openScripts[script_id]), zChooser[script_id], FALSE, TRUE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(zChooser[script_id]), scr[script_id].z);
	gtk_signal_connect(GTK_OBJECT(zChooser[script_id]), "value-changed", (GtkSignalFunc) changePlan, &Index[script_id]);
	//On envoie &Index[i] et pas &i car la valeur à l'adresse &i aura changé quand on recevra le signal

	searchButton[script_id] = gtk_toggle_button_new_with_label("Search a group");
	gtk_box_pack_start(GTK_BOX(openScripts[script_id]), searchButton[script_id], FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(searchButton[script_id]), "toggled", (GtkSignalFunc) on_search_group_button_active, &scr[script_id]);

	expose_event(zone3D, NULL);

	//Pour que les cases soient cochées par défaut

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), TRUE);
	scr[script_id].displayed = TRUE;

	gtk_widget_show_all(pWindow); //Affichage du widget pWindow et de tous ceux qui sont dedans
	gdk_threads_leave();
}

int get_width_height(int nb_row_column)
{
	if (nb_row_column == 1) return 75;
	else if (nb_row_column <= 16) return 300;
	else if (nb_row_column <= 128) return 400;
	else if (nb_row_column <= 256) return 500;
	else return 600;
}

/**
 *  Affiche les neurones d'une petite fenêtre
 */
void expose_neurons(GtkWidget* zone2D, gpointer pData)
{
	int i, j, currentWindow;

	(void) pData;

	//On cherche le numéro de la fenêtre à afficher
	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (zoneNeurones[i] == zone2D)
		{
			currentWindow = i;
			break;
		}

	//On va chercher l'adresse du groupe à afficher dans le tableau windowGroup
	group* g = windowGroup[currentWindow];

	//On détermine la plus grande valeur à afficher (valMax) et la plus petite (valMax). valMax s'affichera en blanc et valMin en noir.
	float valMax, valMin;
	int wV = windowValue[currentWindow]; //Abréviation
	int incrementation = g->nbNeurons / (g->rows * g->columns);

	valMax = g->neurons[0].s[wV];
	valMin = g->neurons[0].s[wV];
	for (i = 1; i < g->nbNeurons; i += incrementation)
	{
		if (g->neurons[i].s[wV] > valMax) valMax = g->neurons[i].s[wV];
		if (g->neurons[i].s[wV] < valMin) valMin = g->neurons[i].s[wV];
	}

	//Dimensions de la fenêtre
	int largeurNeuron = get_width_height(g->columns) / g->columns;
	int hauteurNeuron = get_width_height(g->rows) / g->rows;
	//Dimensions d'un neurone
	//int largeurNeurone = largeur / g->columns;
	//int hauteurNeurone = hauteur / g->rows;

	//Début du dessin
	cairo_t* cr = gdk_cairo_create(zone2D->window); //Crée un contexte Cairo associé à la drawing_area "zone"

	//On commence par dessiner un grand rectangle couleur sable
	cairo_set_source_rgb(cr, SAND);
	cairo_rectangle(cr, 0, 0, 100, 100);
	cairo_fill(cr);

	//Affichage des neurones
	float ndg;
	int nbDigits = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(digitsScale));

	for (i = 0; i < g->nbNeurons; i += incrementation)
	{
		ndg = niveauDeGris(g->neurons[i].s[wV], valMin, valMax);
		cairo_set_source_rgb(cr, ndg, ndg, ndg);
		cairo_rectangle(cr, g->neurons[i].x * largeurNeuron, g->neurons[i].y * hauteurNeuron, largeurNeuron, hauteurNeuron);
		cairo_fill(cr);

		if (ndg > 0.5) color(cr, *g);
		else clearColor(cr, *g);
		cairo_move_to(cr, g->neurons[i].x * largeurNeuron + 4, (g->neurons[i].y + 0.5) * hauteurNeuron);
		gchar valeurNeurone[nbDigits + 1];

		//S'il y a assez de place sur le neurone pour afficher sa valeur avec le nombre de digits demandé
		if (largeurNeuron > 4 * (nbDigits + 1) && hauteurNeuron > 15)
		{
			sprintf(valeurNeurone, "%f", (g->neurons[i].s[wV] / 100));
			valeurNeurone[nbDigits] = '\0';
			if (g->neurons[i].s[wV] >= pow(10, nbDigits) || g->neurons[i].s[wV] <= -pow(10, nbDigits - 1)) for (j = 0; j < nbDigits; j++)
				valeurNeurone[j] = '#';
			//Si le nombre de digits demandé est insuffisant pour afficher entièrement la partie entière d'une valeur, des # s'affichent à la place de cette valeur
		}
		else
		{
			valeurNeurone[0] = '.';
			valeurNeurone[1] = '\0';
		}
		cairo_show_text(cr, valeurNeurone);
	}

	cairo_stroke(cr); //Le contenu de cr est appliqué sur "zoneNeurones"

	if (windowGroup[currentWindow] == selectedGroup) //Si le groupe affiché dans cette fenêtre est sélectionné
	{
		cairo_set_line_width(cr, 10); //Traits plus épais
		cairo_set_source_rgb(cr, RED);
		cairo_move_to(cr, 0, get_width_height(g->rows));
		cairo_line_to(cr, get_width_height(g->columns), get_width_height(g->rows));
		cairo_stroke(cr); //Le contenu de cr est appliqué sur "zoneNeurones"
		cairo_set_line_width(cr, GRID_WIDTH); //Retour aux traits fins*/
	}
	cairo_destroy(cr); //Destruction du calque
}

int button_press_neurons(GtkWidget* zone2D, GdkEventButton* event)
{
	//On cherche le numéro de la fenêtre concernée
	int i, group_id, currentWindow;
	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (zoneNeurones[i] == zone2D)
		{
			currentWindow = i;
			break;
		}

	group* g = windowGroup[currentWindow]; //On va chercher l'adresse du groupe concerné

	if (event->button == 3) //Si clic droit dans une petite fenêtre, on la supprime
	{
		g->sWindow = NB_WINDOWS_MAX; //Cette valeur, pour ce groupe, n'est plus affichée

		for (group_id = 0; group_id < windowGroup[currentWindow]->myScript->nbGroups; group_id++)
		{
			if (windowGroup[currentWindow] == &windowGroup[currentWindow]->myScript->groups[group_id])
			{
				japet_bus_send_message("japet(%d,%d) %s", JAPET_STOP_GROUP, group_id, windowGroup[currentWindow]->myScript->name);
			}
		}
		windowGroup[currentWindow] = NULL; //Plus aucun groupe n'est associé à cette fenêtre

		if (currentWindow == selectedWindow) selectedWindow = NB_WINDOWS_MAX; //Si on supprime la fenêtre sélectionnée, il n'y a plus de fenêtre sélectionnée

		//gtk_container_remove(GTK_CONTAINER(h_box_neuron), pFrameNeurones[currentWindow]); //On efface la fenêtre
		gtk_container_remove(GTK_CONTAINER(zone_neurons), pFrameNeurones[currentWindow]);
		zoneNeurones[currentWindow] = NULL;
		pFrameNeurones[currentWindow] = NULL;
		usedWindows--;

		//if (usedWindows == 0) gtk_widget_set_size_request(h_box_neuron, 100, 0); //S'il n'y a plus de petite fenêtre ouverte, on referme le bandeau du bas
	}
	else //Si clic gauche, la fenêtre est sélectionnée
	{
		selectedWindow = currentWindow;
		selectedGroup = g; //Le groupe associé à la fenêtre dans laquelle on a cliqué est également sélectionné
		move_neurons_old_x = event->x;
		move_neurons_old_y = event->y;
		move_neurons_start = 1;
		move_neurons_frame = pFrameNeurones[currentWindow];
	}

	//On actualise l'affichage
	expose_event(zone3D, NULL);

	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (zoneNeurones[i] != NULL) expose_neurons(zoneNeurones[i], NULL);

	return TRUE;
}

void switch_output(GtkWidget* pWidget, group *group)
{
	char *output_name;
	int currentWindow;

	for (currentWindow = 0; currentWindow < NB_WINDOWS_MAX; currentWindow++)
	{
		if (windowGroup[currentWindow] == group) break;
	}

	windowValue[currentWindow]++;

	if (windowValue[currentWindow] == 3) windowValue[currentWindow] = 0;

	if (windowValue[currentWindow] == 0) output_name = "s";
	else if (windowValue[currentWindow] == 1) output_name = "s1";
	else if (windowValue[currentWindow] == 2) output_name = "s2";

	gtk_button_set_label(GTK_BUTTON(pWidget), g_strconcat(windowGroup[currentWindow]->name, " - ", output_name, NULL));
	expose_neurons(zoneNeurones[currentWindow], NULL);
}

void drag_drop_neuron_frame(GtkWidget *zone_neurons, GdkEventButton *event, gpointer data)
{
	(void) data;

	if (move_neurons_frame != NULL) gtk_layout_move(GTK_LAYOUT(zone_neurons), move_neurons_frame, ((int)(event->x / 25)) * 25, ((int)(event->y / 25)) * 25);

	move_neurons_start = 0;
	move_neurons_frame = NULL;
}

//--------------------------------------------------5. "CONSTRUCTEURS"---------------------------------------------

void newScript(script* script, char* name, char* machine, int z, int nbGroups)
{
	int tailleName, tailleMachine;

	tailleName = strlen(name);
	script->name = (char*) malloc((tailleName + 1) * sizeof(char));
	strncpy(script->name, name, tailleName);
	script->name[tailleName] = '\0';

	tailleMachine = strlen(machine);
	script->machine = (char*) malloc((tailleMachine + 1) * sizeof(char));
	strncpy(script->machine, machine, tailleMachine);
	script->machine[tailleMachine] = '\0';

	script->z = z;
	script->nbGroups = nbGroups;
	script->groups = malloc(nbGroups * sizeof(group));
	script->displayed = FALSE;
}

/**
 * 
 * Destruction des groupes du script à l'adresse s
 * 
 * @param s à supprimer
 * 
 */
void destroyScript(int script_id)
{
	int i;

	scr[script_id].autorize_neurons_update = 0;

	gtk_widget_destroy(zChooser[script_id]);
	gtk_widget_destroy(scriptLabel[script_id]);
	gtk_widget_destroy(scriptCheck[script_id]);
	gtk_widget_destroy(openScripts[script_id]);
	gtk_widget_destroy(searchButton[script_id]);

	for (i = 0; i < scr[script_id].nbGroups; i++)
	{
		free(scr[script_id].groups[i].previous);
		free(scr[script_id].groups[i].neurons);
	}
	free(scr[script_id].name);
	free(scr[script_id].machine);
}

//
/**
 * 
 * Destruction de tous les scripts
 * 
 */

void destroyAllScripts()
{
	int i, current_script;

	//Tableau des scripts
	for (i = 0; i < NB_SCRIPTS_MAX; i++)
		scr[i].z = -4; //Les scripts non créés sont placés dans le plan -4

	for (current_script = 0; current_script < nbScripts; current_script++)
	{
		enet_peer_disconnect(scr[current_script].peer, 0);
		destroyScript(current_script);
	}

	nbScripts = 0;

	//Il faut aussi supprimer toutes les petites fenêtres éventuellement ouvertes
	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (pFrameNeurones[i] != NULL)
		{
			//gtk_container_remove(GTK_CONTAINER(h_box_neuron), pFrameNeurones[i]);
			gtk_container_remove(GTK_CONTAINER(zone_neurons), pFrameNeurones[i]);
			windowGroup[i] = NULL;
			zoneNeurones[i] = NULL;
			pFrameNeurones[i] = NULL;
		}

	usedWindows = 0;
	//gtk_widget_set_size_request(h_box_neuron, 100, 0);

	ivyServerNb = 0;

	//Réinitialisation d'ENet
	/*enet_deinitialize();

	 if (enet_initialize() != 0)
	 {
	 printf("An error occurred while initializing ENet.\n");
	 exit(EXIT_FAILURE);
	 }

	 atexit(enet_deinitialize);*/
	//server_for_promethes();

	nbSnapshots = 1; //Par défaut, 1 série de valeurs est enregistrée à leur création dans le tableau "buffer" de chaque neurone
}

/**
 * 
 * Créer un script. 
 * 
 * sWindow: Toutes les cases du script sont initialisées à NB_WINDOWS_MAX
 */
void newGroup(group *g, script* myScript, char* name, char* function, float learningSpeed, int nbNeurons, int rows, int columns, int y, int nbLinksTo, int firstNeuron)
{
	g->myScript = myScript;

	int tailleName = strlen(name);
	g->name = (char*) malloc((tailleName + 1) * sizeof(char));
	strncpy(g->name, name, tailleName);
	g->name[tailleName] = '\0';

	int tailleFonction = strlen(function);
	g->function = (char*) malloc((tailleFonction + 1) * sizeof(char));
	sprintf(g->function, "%s", function);
	g->name[tailleFonction] = '\0';

	g->learningSpeed = learningSpeed;
	g->nbNeurons = nbNeurons;
	g->rows = rows;
	g->columns = columns;
	g->neurons = malloc(nbNeurons * sizeof(neuron));
	g->y = y;
	g->knownX = FALSE;
	g->knownY = FALSE;
	g->nbLinksTo = nbLinksTo;

	if (nbLinksTo == 0) g->previous = NULL;
	else g->previous = malloc(nbLinksTo * sizeof(group*));

	int i;
	for (i = 0; i < 4; i++)
		g->sWindow = NB_WINDOWS_MAX;
	//Toutes les cases sont initialisées à NB_WINDOWS_MAX, ce qui signifie que cette valeur n'est encore affichée dans aucune petite fenêtre

	g->justRefreshed = FALSE;
	g->firstNeuron = firstNeuron;
	g->allocatedNeurons = 0;
}

/**
 * 
 * Créer un nouveau neurone
 */
void newNeuron(neuron* neuron, group* myGroup, float s, float s1, float s2, float pic, int x, int y)
{
	neuron->myGroup = myGroup;
	neuron->s[0] = s;
	neuron->s[1] = s1;
	neuron->s[2] = s2;
	neuron->s[3] = pic;
	neuron->x = x;
	neuron->y = y;
	neuron->myGroup->allocatedNeurons++;

	//Ces valeurs sont aussi enregistrées dans la première ligne du tableau "buffer"
	neuron->buffer[0][0] = s;
	neuron->buffer[1][0] = s1;
	neuron->buffer[2][0] = s2;
	neuron->buffer[3][0] = pic;
}

/**
 * 
 * Mise un jour d'un neurone quand Prométhé envoie de nouvelles données
 * 
 */
//Actuellement, cette fonction n'est pas utilisée. Elle sera peut-être utile pour le mode "instantanés"
void updateNeuron(neuron* neuron, gfloat s, gfloat s1, gfloat s2, gfloat pic)
{
	neuron->s[0] = s;
	neuron->s[1] = s1;
	neuron->s[2] = s2;
	neuron->s[3] = pic;

	int i;

	//Mise en mémoire
	if (displayMode[1] == 'a') //Si on est en mode échantillonné ('a' est la deuxième lettre de "Sampled mode")
	{
		if (nbSnapshots == NB_BUFFERED_MAX) //Si le tableau "buffer" est plein, on oublie sa première ligne (la plus ancienne) et on recule chaque valeur d'une ligne
		{
			for (i = 1; i < nbSnapshots; i++)
			{
				neuron->buffer[0][i - 1] = neuron->buffer[0][i];
				neuron->buffer[1][i - 1] = neuron->buffer[1][i];
				neuron->buffer[2][i - 1] = neuron->buffer[2][i];
				neuron->buffer[3][i - 1] = neuron->buffer[3][i];
			}
		}

		neuron->buffer[0][nbSnapshots - 1] = s;
		neuron->buffer[1][nbSnapshots - 1] = s1;
		neuron->buffer[2][nbSnapshots - 1] = s2;
		neuron->buffer[3][nbSnapshots - 1] = pic;
	}
}

//---------------------------------------------------6. AUTRES FONCTIONS---------------------------------------------

/** 
 * Cette fonction permet de traiter les informations provenant de prométhé pour mettre à jour
 * l'affichage. Elle est appelée automatiquement plusieurs fois par seconde.
 */
gboolean refresh_display()
{
	//if(displayMode[1] == 'a') //Si on est en mode échantillonné ('a' est la deuxième lettre de "Sampled mode")
	//{
	if (pWindow == NULL) return FALSE;
	else
	{
		//expose_event(zone3D, NULL); //On redessine la grille
		//Et on réaffiche toutes les petites fenêtres
		int i;
		for (i = 0; i < NB_WINDOWS_MAX; i++)
			if (windowGroup[i] != NULL) expose_neurons(zoneNeurones[i], NULL);
		//gtk_widget_show_all(h_box_neuron);
		gtk_widget_show_all(neurons_frame);
		return TRUE;
	}
	//}
}

char* tcolor(script script)
{
	switch (script.z)
	{
	case 0:
		return TDARKGREEN;
	case 1:
		return TDARKMAGENTA;
	case 2:
		return TDARKYELLOW;
	case 3:
		return TDARKBLUE;
	case 4:
		return TDARKCYAN;
	case 5:
		return TDARKORANGE;
	case 6:
		return TDARKGREY;
	default:
		return TCOBALT;
	}
}

/**
 * 
 * Donne au pinceau la couleur foncée correspondant à une certaine valeur de z
 * 
 */
void color(cairo_t* cr, group group)
{
	switch (group.myScript->z)
	//La couleur d'un groupe ou d'une liaison dépend du plan dans lequel il/elle se trouve
	{
	case 0:
		cairo_set_source_rgb(cr, DARKGREEN);
		break;
	case 1:
		cairo_set_source_rgb(cr, DARKMAGENTA);
		break;
	case 2:
		cairo_set_source_rgb(cr, DARKYELLOW);
		break;
	case 3:
		cairo_set_source_rgb(cr, DARKBLUE);
		break;
	case 4:
		cairo_set_source_rgb(cr, DARKCYAN);
		break;
	case 5:
		cairo_set_source_rgb(cr, DARKORANGE);
		break;
	case 6:
		cairo_set_source_rgb(cr, DARKGREY);
		break;
	}
}

/**
 * 
 * Donne au pinceau la couleur claire correspondant à une certaine valeur de z
 * 
 */
void clearColor(cairo_t* cr, group group)
{
	switch (group.myScript->z)
	//La couleur d'un groupe ou d'une liaison dépend du plan dans lequel il/elle se trouve
	{
	case 0:
		cairo_set_source_rgb(cr, GREEN);
		break;
	case 1:
		cairo_set_source_rgb(cr, MAGENTA);
		break;
	case 2:
		cairo_set_source_rgb(cr, YELLOW);
		break;
	case 3:
		cairo_set_source_rgb(cr, BLUE);
		break;
	case 4:
		cairo_set_source_rgb(cr, CYAN);
		break;
	case 5:
		cairo_set_source_rgb(cr, ORANGE);
		break;
	case 6:
		cairo_set_source_rgb(cr, GREY);
		break;
	}
}

void dessinGroupe(cairo_t* cr, int a, int b, int c, int d, group* g, int z, int zMax)
{
	int x = g->x, y = g->y;

	if (g == selectedGroup) cairo_set_source_rgb(cr, RED);
	//else if (g->justRefreshed == TRUE) clearColor(cr, *g);
	else color(cr, *g);
	cairo_rectangle(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z - HAUTEUR_GROUPE / 2, LARGEUR_GROUPE, HAUTEUR_GROUPE);
	cairo_fill(cr);

	/*	if (g->justRefreshed == TRUE) cairo_set_source_rgb(cr, BLACK);
	 else */
	cairo_set_source_rgb(cr, WHITE);
	cairo_set_font_size(cr, 8);
	cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z);
	cairo_show_text(cr, g->name);
	cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z + HAUTEUR_GROUPE / 2);

	cairo_show_text(cr, g->function);

	int i;

	//Dessin des liaisons aboutissant à ce groupe
	for (i = 0; i < g->nbLinksTo; i++)
		if (g->previous[i]->myScript->displayed == TRUE)
		{
			gint x1 = g->previous[i]->x, y1 = g->previous[i]->y, z1 = g->previous[i]->myScript->z; //Coordonnées du groupe situé avant la liaison

			if (g->previous[i]->myScript == g->myScript) color(cr, *g);
			else cairo_set_source_rgb(cr, RED); //Si c'est une liaison entre deux groupes appartenant à des scripts différents, elle est dessinée en rouge

			cairo_set_line_width(cr, 0.8); //Trait épais représentant une liaison entre groupes

			cairo_move_to(cr, a * x1 + c * (zMax - z1) + LARGEUR_GROUPE / 2, b * y1 + d * z1); //Début de la liaison (à droite du groupe prédécesseur)
			cairo_line_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z); //Fin de la liaison (à gauche du groupe courant)

			cairo_stroke(cr);
		}
}

/**
 * 
 * Calcule la coordonnée x d'un groupe en fonction de ses prédécesseurs
 * 
 */
void findX(group* group)
{
	int i, Max = 0;

	if (group->previous == NULL)
	{
		group->x = 1;
	}
	else
	{
		for (i = 0; i < group->nbLinksTo; i++)
		{
			if (group->previous[i]->knownX == FALSE) findX(group->previous[i]);
			if (group->previous[i]->x > Max) Max = group->previous[i]->x;
		}
		group->x = Max + 1;
	}

	group->knownX = TRUE;
}

/**
 * 
 * Calcule la coordonnée y d'un groupe
 * 
 */
void findY(group* group)
{
	int freeY;
	int i, j;

	freeY = 2;

	for (i = 0; i < nbScripts; i++)
	{
		if (scr[i].z == group->myScript->z)
		{
			for (j = 0; j < scr[i].nbGroups; j++)
			{
				if (scr[i].groups[j].knownY == TRUE && scr[i].groups[j].x == group->x)
				{
					freeY++;
				}
			}
		}
	}
	group->y = freeY;

	group->knownY = TRUE;
}

void newWindow(group* g)
{
	int i, currentWindow; //On cherche la première case vide dans le tableau windowGroup et on donne son numéro à la nouvelle fenêtre
	GtkWidget *button_frame;

	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (windowGroup[i] == NULL)
		{
			currentWindow = i;
			break;
		}

	windowGroup[currentWindow] = g; //On enregistre l'adresse du groupe affiché dans cette fenêtre
	windowPosition[currentWindow] = usedWindows; //et la position de cette fenêtre dans le bandeau
	usedWindows++; //Puis on incrémente le nombre de fenêtres utilisées

	windowValue[currentWindow] = 1;

	g->sWindow = currentWindow; //On enregistre que c'est CETTE fenêtre qui affiche CETTE valeur des neurones de CE groupe

	//Création de la petite fenêtre
	pFrameNeurones[currentWindow] = gtk_frame_new(""); //Titre de la fenêtre

	gtk_layout_put(GTK_LAYOUT(zone_neurons), pFrameNeurones[currentWindow], 400 * (usedWindows - 1), 0);

	button_frame = gtk_toggle_button_new_with_label(g_strconcat(g->name, " - ", "s1", NULL));
	g_signal_connect(GTK_OBJECT(button_frame), "toggled", (GtkSignalFunc) switch_output, g);
	gtk_frame_set_label_widget(GTK_FRAME(pFrameNeurones[currentWindow]), button_frame);

	/*gtk_widget_add_events(pFrameNeurones[currentWindow], GDK_BUTTON_PRESS_MASK);*/

	/*
	 g_signal_connect(GTK_OBJECT(pFrameNeurones[currentWindow]), "drag-begin", (GtkSignalFunc) drag_begin_neuron_frame, NULL);
	 g_signal_connect(GTK_OBJECT(pFrameNeurones[currentWindow]), "drag-drop", (GtkSignalFunc) drag_drop_neuron_frame, NULL);
	 */

	zoneNeurones[currentWindow] = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(pFrameNeurones[currentWindow]), zoneNeurones[currentWindow]);
	gtk_widget_add_events(zoneNeurones[currentWindow], GDK_BUTTON_PRESS_MASK);
	g_signal_connect(GTK_OBJECT(zoneNeurones[currentWindow]), "button_press_event", (GtkSignalFunc) button_press_neurons, NULL);

	resizeNeurons();

	g_signal_connect(GTK_OBJECT(zoneNeurones[currentWindow]), "expose_event", (GtkSignalFunc) expose_neurons, NULL);
}

float niveauDeGris(float val, float valMin, float valMax)
{
	float ndg = (val - valMin) / (valMax - valMin);
	return ndg;
}

void resizeNeurons()
{
	//On partage la largeur de la grande fenêtre Japet entre les petites fenêtres ouvertes, en fonction du nombre de colonnes de neurones dans chacune
	int i;
	//int largeurJapet = h_box_neuron->allocation.width; //Largeur totale de la grande fenêtre Japet
	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (windowGroup[i] != NULL)
		{
			gtk_widget_set_size_request(zoneNeurones[i], get_width_height(windowGroup[i]->columns), get_width_height(windowGroup[i]->rows));
			expose_neurons(zoneNeurones[i], NULL);
		}
	gtk_widget_show_all(neurons_frame);
}

const char * whitespace_callback_japet(mxml_node_t *node, int where)
{
	const char *name;

	if (node == NULL)
	{
		fprintf(stderr, "%s \n", __FUNCTION__);
		return NULL;
	}

	name = node->value.element.name;

	if (where == MXML_WS_AFTER_CLOSE || where == MXML_WS_AFTER_OPEN)
	{
		return ("\n");
	}

	if (!strcmp(name, "scale") || !strcmp(name, "script"))
	{
		if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) return ("\n\t");
	}
	/*else if (!strcmp(name, "script_oscillo"))
	 {
	 if (where == MXML_WS_BEFORE_OPEN) return ("\n\t\t");
	 else if (where == MXML_WS_BEFORE_CLOSE) return ("\t\t");
	 }
	 else if(!strcmp(name, "informations") || !strcmp(name, "script"))
	 {
	 if (where == MXML_WS_BEFORE_OPEN) return ("\n\t");
	 }
	 else if (!strcmp(name, "group"))
	 {
	 if (where == MXML_WS_BEFORE_OPEN) return ("\t\t\t");
	 }*/

	return (NULL);
}
void saveJapetConfigToFile(char* filename)
{
	int script_id;
	Node *tree, *scale, *script;

	tree = mxmlNewXML("1.0");

	scale = mxmlNewElement(tree, "scale");
	xml_set_string(scale, "type", "refresh_scale");
	xml_set_int(scale, "value", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(refreshScale)));

	scale = mxmlNewElement(tree, "scale");
	xml_set_string(scale, "type", "x_scale");
	xml_set_int(scale, "value", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(xScale)));

	scale = mxmlNewElement(tree, "scale");
	xml_set_string(scale, "type", "y_scale");
	xml_set_int(scale, "value", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(yScale)));

	scale = mxmlNewElement(tree, "scale");
	xml_set_string(scale, "type", "z_x_scale");
	xml_set_int(scale, "value", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zxScale)));

	scale = mxmlNewElement(tree, "scale");
	xml_set_string(scale, "type", "z_y_scale");
	xml_set_int(scale, "value", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zyScale)));

	scale = mxmlNewElement(tree, "scale");
	xml_set_string(scale, "type", "neuron_height_scale");
	xml_set_int(scale, "value", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(neuronHeightScale)));

	scale = mxmlNewElement(tree, "scale");
	xml_set_string(scale, "type", "digits_scale");
	xml_set_int(scale, "value", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(digitsScale)));

	for (script_id = 0; script_id < nbScripts; script_id++)
	{
		script = mxmlNewElement(tree, "script");
		xml_set_string(script, "name", scr[script_id].name);

		if (scr[script_id].displayed) xml_set_int(script, "visibility", 1);
		else xml_set_int(script, "visibility", 0);
	}

	xml_save_file(filename, tree, whitespace_callback_japet);
}

void loadJapetConfigToFile(char* filename)
{
	int script_id, i;
	Node *tree, *loading_node;

	tree = xml_load_file(filename);

	loading_node = xml_get_first_child_with_node_name(tree, "scale");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(refreshScale), xml_get_int(loading_node, "value"));

	loading_node = xml_get_next_homonymous_sibling(loading_node);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(xScale), xml_get_int(loading_node, "value"));

	loading_node = xml_get_next_homonymous_sibling(loading_node);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(yScale), xml_get_int(loading_node, "value"));

	loading_node = xml_get_next_homonymous_sibling(loading_node);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(zxScale), xml_get_int(loading_node, "value"));

	loading_node = xml_get_next_homonymous_sibling(loading_node);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(zyScale), xml_get_int(loading_node, "value"));

	loading_node = xml_get_next_homonymous_sibling(loading_node);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(neuronHeightScale), xml_get_int(loading_node, "value"));

	loading_node = xml_get_next_homonymous_sibling(loading_node);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(digitsScale), xml_get_int(loading_node, "value"));

	for (script_id = 0; script_id < nbScripts; script_id++)
	{
		loading_node = xml_get_first_child_with_node_name(tree, "script");
		if (!strcmp(scr[script_id].name, xml_get_string(loading_node, "name")))
		{
			if (xml_get_int(loading_node, "visibility") == 1)
			{
				scr[script_id].displayed = TRUE;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), TRUE);
			}
			else
			{
				scr[script_id].displayed = FALSE;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), FALSE);
			}
		}

		for (i = 8; i < xml_get_number_of_childs(tree); i++)
		{
			loading_node = xml_get_next_homonymous_sibling(loading_node);
			if (!strcmp(scr[script_id].name, xml_get_string(loading_node, "name")))
			{
				if (xml_get_int(loading_node, "visibility") == 1)
				{
					scr[script_id].displayed = TRUE;
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), TRUE);
				}
				else
				{
					scr[script_id].displayed = FALSE;
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), FALSE);
				}
			}
		}
	}
}

void fatal_error(const char *name_of_file, const char* name_of_function, int numero_of_line, const char *message, ...)
{
	char total_message[MESSAGE_MAX];

	GtkWidget *dialog;
	va_list arguments;
	va_start(arguments, message);
	vsnprintf(total_message, MESSAGE_MAX, message, arguments);
	va_end(arguments);

	dialog = gtk_message_dialog_new(GTK_WINDOW(pWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s \t %s \t %i :\n \t Error: %s \n", name_of_file, name_of_function, numero_of_line, total_message);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	exit(EXIT_FAILURE);
}
