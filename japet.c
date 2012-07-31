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

GtkWidget *pWindow; //La fenêtre de l'application Japet
GtkWidget *hide_see_scales_button; //Boutton permettant de cacher le menu de droite
GtkWidget *pPane; //Panneau latéral
GtkWidget *pVBoxScripts; //Panneau des scripts
GtkWidget *zone3D; //La grande zone de dessin des liaisons entre groupes
GtkWidget *refreshScale, *xScale, *yScale, *zxScale, *zyScale, *digitsScale; //Échelles
GtkWidget *check_button_draw_connections, *check_button_draw_net_connections;
//Indiquent quel est le mode d'affichage en cours (Off-line, Sampled ou Snapshots)
char *displayMode;
GtkWidget *modeLabel;
int currentSnapshot;
int nbSnapshots;

int Index[NB_SCRIPTS_MAX]; //Tableau des indices : Index[0] = 0, Index[1] = 1, ..., Index[NB_SCRIPTS_MAX-1] = NB_SCRIPTS_MAX-1;
//Ce tableau permet à une fonction signal de retenir la valeur qu'avait i au moment où on a connecté le signal                                                                                                                                                               

GtkWidget **openScripts; //Lignes du panneau des scripts
GtkWidget **scriptCheck; //Cases à cocher/décocher pour afficher les scripts ou les masquer
GtkWidget **scriptLabel; //Label affichant le nom d'un script dans la bonne couleur
GtkWidget **zChooser; //Spin_button pour choisir dans quel plan afficher ce script
GtkWidget **searchButton; //Boutton pour rechercher un groupe dans ce script

int nbScripts = 0; //Nombre de scripts à afficher
char scriptsNames[NB_SCRIPTS_MAX][SCRIPT_NAME_MAX]; //Tableau des noms des scripts
script scr[NB_SCRIPTS_MAX]; //Tableau des scripts à afficher
int zMax; //la plus grande valeur de z parmi les scripts ouverts
int buffered = 0; //Nombre d'instantanés actuellement en mémoire

int usedWindows = 0;
group *selectedGroup = NULL; //Pointeur sur le groupe actuellement sélectionné
int selectedWindow = NB_WINDOWS_MAX; //Numéro de la fenêtre sélectionnée (entre 0 et NB_WINDOWS_MAX-1). NB_WINDOWS_MAX indique qu'aucune fenêtre n'est sélectionnée

GtkWidget *neurons_frame, *zone_neurons;//Panneau des neurones
GtkWidget *pFrameNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de petites fenêtres pour les neurones
GtkWidget *zoneNeurones[NB_WINDOWS_MAX]; //Tableau de NB_WINDOWS_MAX adresses de zones où dessiner des neurones
group *windowGroup[NB_WINDOWS_MAX]; //Adresse des groupe affiché dans la zoneNeurones de même indice
int windowValue[NB_WINDOWS_MAX]; //Numéro disant quelle valeur des neurones du groupe il faut afficher dans la fenêtre de même indice (0 : s, 1 : s1, 2 : s2, 3 : pic)
coordonnees windowPosition[NB_WINDOWS_MAX]; //Position de la fenêtre dans la zone_neurons

int nbColonnesTotal = 0; //Nombre total de colonnes de neurones dans les fenêtres du bandeau du bas
int nbLignesMax = 0; //Nombre maximal de lignes de neurones à afficher dans l'une des fenêtres du bandeau du bas

char id[BUS_ID_MAX]; //bus_id
char file_preferences[PATH_MAX]; //fichier de préférences (*.jap)

int move_neurons_old_x, move_neurons_old_y, move_neurons_start = 0, move_neurons_frame_id = -1; //Pour bouger un frame_neuron
int open_neurons_start = 0; //Pour ouvrir un frame_neuron
group *open_group = NULL;

guint refresh_timer_id; //id du timer actualement utiliser pour le rafraichissement des frame_neurons ouvertes

type_script_link net_link[SCRIPT_LINKS_MAX];
int nb_net_link = 0;
//--------------------------------------------------2. MAIN-------------------------------------------------------------

void on_signal_interupt(int signal)
{
	switch (signal)
	{
	case SIGINT:
		japet_bus_send_message(id, "japet(%d,0)", JAPET_STOP);
		exit(EXIT_SUCCESS);
		break;
	case SIGSEGV:
		printf("SEGFAULT\n");
		japet_bus_send_message(id, "japet(%d,0)", JAPET_STOP);
		exit(EXIT_FAILURE);
		break;
	default:
		printf("signal %d capture par le gestionnaire mais non traite... revoir le gestinnaire ou le remplissage de sigaction\n", signal);
		break;
	}
}

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
	int option;
	struct sigaction action;

	sigfillset(&action.sa_mask);
	action.sa_handler = on_signal_interupt;
	action.sa_flags = 0;
	if (sigaction(SIGINT, &action, NULL) != 0)
	{
		printf("Signal SIGINT not catched.");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGSEGV, &action, NULL) != 0)
	{
		printf("Signal SIGSEGV not catched.");
		exit(EXIT_FAILURE);
	}

	g_thread_init(NULL);
	gdk_threads_init();

	// Initialisation de GTK+
	gtk_init(&argc, &argv);

	init_japet(argc, argv);

	id[0] = 0;
	file_preferences[0] = 0;

	//On regarde les fichier passés en ligne de commande
	if (optind < argc)
	{
		// fichier des préférences (*.jap)
		if (strstr(argv[optind], ".jap"))
		{
			FILE *file = fopen(argv[optind], "r");
			//si le fichier existe
			if (file != NULL)
			{
				fclose(file);
				strncpy(file_preferences, argv[optind], PATH_MAX);
			}
			//s'il n'existe pas
			else
			{
				fprintf(stderr, "%s : file not found", argv[optind]);
				exit(EXIT_FAILURE);
			}
		}
	}

	//La fenêtre principale
	pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	/* Positionne la GTK_WINDOW "pWindow" au centre de l'écran */
	gtk_window_set_position(GTK_WINDOW(pWindow), GTK_WIN_POS_CENTER);
	/* Taille de la fenêtre */
	gtk_window_set_default_size(GTK_WINDOW(pWindow), 1200, 800);
	/* Titre de la fenêtre */
	gtk_window_set_title(GTK_WINDOW(pWindow), "Japet");
	//Le signal de fermeture de la fenêtre est connecté à la fenêtre (petite croix)
	g_signal_connect(G_OBJECT(pWindow), "destroy", G_CALLBACK(Close), (GtkWidget*) pWindow);

	/*Création d'une VBox (boîte de widgets disposés verticalement) */
	GtkWidget *v_box_main = gtk_vbox_new(FALSE, 0);
	/*ajout de v_box_main dans pWindow, qui est alors vu comme un GTK_CONTAINER*/
	gtk_container_add(GTK_CONTAINER(pWindow), v_box_main);

	hide_see_scales_button = gtk_toggle_button_new_with_label("Hide scales");
	g_signal_connect(G_OBJECT(hide_see_scales_button), "toggled", (GtkSignalFunc) on_hide_see_scales_button_active, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_see_scales_button), FALSE);
	gtk_widget_set_size_request(hide_see_scales_button, 150, 30);
	gtk_box_pack_start(GTK_BOX(v_box_main), hide_see_scales_button, FALSE, FALSE, 0);

	/*Création de deux HBox : une pour le panneau latéral et la zone principale, l'autre pour les 6 petites zones*/
	GtkWidget *h_box_main = gtk_hbox_new(FALSE, 0);
	GtkWidget *vpaned = gtk_vpaned_new();
	gtk_paned_set_position(GTK_PANED(vpaned), 600);
	neurons_frame = gtk_frame_new("Neurons' frame");
	gtk_box_pack_start(GTK_BOX(v_box_main), h_box_main, TRUE, TRUE, 0);

	/*Panneau latéral*/
	pPane = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_main), pPane, FALSE, TRUE, 0);

	//Les échelles
	GtkWidget *pFrameEchelles = gtk_frame_new("Scales");
	gtk_container_add(GTK_CONTAINER(pPane), pFrameEchelles);
	GtkWidget *pVBoxEchelles = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pFrameEchelles), pVBoxEchelles);

	//Fréquence de réactualisation de l'affichage, quand on est en mode échantillonné (Sampled)
	GtkWidget *refreshSetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), refreshSetting, FALSE, TRUE, 0);
	GtkWidget *refreshLabel = gtk_label_new("Refresh (Hz):");
	refreshScale = gtk_hscale_new_with_range(1, 24, 1); //Ce widget est déjà déclaré comme variable globale
	//On choisit le nombre de réactualisations de l'affichage par seconde, entre 1 et 24
	gtk_box_pack_start(GTK_BOX(refreshSetting), refreshLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(refreshSetting), refreshScale, TRUE, TRUE, 0);
	gtk_range_set_value(GTK_RANGE(refreshScale), REFRESHSCALE_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(refreshScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Echelle de l'axe des x
	GtkWidget *xSetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), xSetting, FALSE, TRUE, 0);
	GtkWidget *xLabel = gtk_label_new("x scale:");
	xScale = gtk_hscale_new_with_range(10, 350, 1); //Ce widget est déjà déclaré comme variable globale
	gtk_box_pack_start(GTK_BOX(xSetting), xLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(xSetting), xScale, TRUE, TRUE, 0);
	gtk_range_set_value(GTK_RANGE(xScale), XSCALE_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(xScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Echelle de l'axe des y
	GtkWidget *ySetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), ySetting, FALSE, TRUE, 0);
	GtkWidget *yLabel = gtk_label_new("y scale:");
	yScale = gtk_hscale_new_with_range(10, 350, 1); //Ce widget est déjà déclaré comme variable globale
	gtk_box_pack_start(GTK_BOX(ySetting), yLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ySetting), yScale, TRUE, TRUE, 0);
	gtk_range_set_value(GTK_RANGE(yScale), YSCALE_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(yScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Décalage des plans selon x
	GtkWidget *zxSetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zxSetting, FALSE, TRUE, 0);
	GtkWidget *zxLabel = gtk_label_new("x gap:");
	zxScale = gtk_hscale_new_with_range(0, 200, 1); //Ce widget est déjà déclaré comme variable globale
	gtk_box_pack_start(GTK_BOX(zxSetting), zxLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(zxSetting), zxScale, TRUE, TRUE, 0);
	gtk_range_set_value(GTK_RANGE(zxScale), XGAP_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(zxScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Décalage des plans selon y
	GtkWidget *zySetting = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), zySetting, FALSE, TRUE, 0);
	GtkWidget *zyLabel = gtk_label_new("y gap:");
	zyScale = gtk_hscale_new_with_range(0, 2000, 1); //Ce widget est déjà déclaré comme variable globale
	gtk_box_pack_start(GTK_BOX(zySetting), zyLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(zySetting), zyScale, TRUE, TRUE, 0);
	gtk_range_set_value(GTK_RANGE(zyScale), YGAP_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(zyScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//Nombre digits pour afficher les valeurs des neurones
	GtkWidget *digits = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), digits, FALSE, TRUE, 0);
	GtkWidget *digitsLabel = gtk_label_new("Neuron digits:");
	digitsScale = gtk_hscale_new_with_range(1, 10, 1); //Ce widget est déjà déclaré comme variable globale
	gtk_box_pack_start(GTK_BOX(digits), digitsLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(digits), digitsScale, TRUE, TRUE, 0);
	gtk_range_set_value(GTK_RANGE(digitsScale), DIGITS_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(digitsScale), "value-changed", (GtkSignalFunc) changeValue, NULL);

	//3 boutons
	GtkWidget *pBoutons = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), pBoutons, FALSE, TRUE, 0);
	GtkWidget *boutonSave = gtk_button_new_with_label("Save");
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonSave, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonSave), "clicked", G_CALLBACK(japet_save_preferences), NULL);
	GtkWidget *boutonLoad = gtk_button_new_with_label("Load");
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonLoad, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonLoad), "clicked", G_CALLBACK(japet_load_preferences), NULL);
	GtkWidget *boutonDefault = gtk_button_new_with_label("Default");
	gtk_box_pack_start(GTK_BOX(pBoutons), boutonDefault, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(boutonDefault), "clicked", G_CALLBACK(defaultScale), NULL);

	check_button_draw_connections = gtk_check_button_new_with_label("draw connections");
	check_button_draw_net_connections = gtk_check_button_new_with_label("draw net connections");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_draw_connections), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_draw_net_connections), TRUE);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), check_button_draw_connections, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pVBoxEchelles), check_button_draw_net_connections, FALSE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(check_button_draw_connections), "toggled", (GtkSignalFunc) on_check_button_draw_active, NULL);
	gtk_signal_connect(GTK_OBJECT(check_button_draw_net_connections), "toggled", (GtkSignalFunc) on_check_button_draw_active, NULL);

	displayMode = "Sampled mode";

	modeLabel = gtk_label_new(displayMode);
	gtk_container_add(GTK_CONTAINER(pPane), modeLabel);

	//Les scripts
	GtkWidget *pFrameScripts = gtk_frame_new("Open scripts");
	gtk_container_add(GTK_CONTAINER(pPane), pFrameScripts);
	pVBoxScripts = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pFrameScripts), pVBoxScripts);
	GtkWidget *askButton = gtk_button_new_with_label("Ask for scripts");
	gtk_box_pack_start(GTK_BOX(pVBoxScripts), askButton, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(askButton), "clicked", G_CALLBACK(askForScripts), NULL);

	openScripts = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*)); //Il y aura autant de lignes que de scripts ouverts.
	scriptCheck = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*)); //Sur chacune de ces lignes, il y a une case à cocher...
	scriptLabel = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*)); //suivie d'un label...
	zChooser = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*)); // d'un "spinbutton",
	searchButton = malloc(NB_SCRIPTS_MAX * sizeof(GtkWidget*));// et d'un boutton "recherche"

	//La zone principale
	GtkWidget *pFrameGroupes = gtk_frame_new("Neural groups");
	gtk_container_add(GTK_CONTAINER(vpaned), pFrameGroupes);
	GtkWidget *scrollbars = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(pFrameGroupes), scrollbars);
	zone3D = gtk_drawing_area_new(); //Déjà déclarée comme variable globale
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars), zone3D);
	gtk_drawing_area_size(GTK_DRAWING_AREA(zone3D), 10000, 10000);
	gtk_signal_connect(GTK_OBJECT(zone3D), "expose_event", (GtkSignalFunc) expose_event, NULL);
	gtk_widget_set_events(zone3D, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK); //Détecte quand on appuie OU quand on relache un bouton de la souris alors que le curseur est dans la zone3D
	gtk_signal_connect(GTK_OBJECT(zone3D), "button_press_event", (GtkSignalFunc) button_press_event, NULL);
	gtk_signal_connect(GTK_OBJECT(pWindow), "key_press_event", (GtkSignalFunc) key_press_event, NULL);

	//la zone des groupes de neurones
	gtk_container_add(GTK_CONTAINER(vpaned), neurons_frame);
	GtkWidget *scrollbars2 = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(neurons_frame), scrollbars2);
	zone_neurons = gtk_layout_new(NULL, NULL);
	gtk_widget_set_size_request(GTK_WIDGET(zone_neurons), 3000, 3000);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollbars2), zone_neurons);
	gtk_widget_add_events(zone_neurons, GDK_ALL_EVENTS_MASK);
	g_signal_connect(GTK_OBJECT(zone_neurons), "button-release-event", (GtkSignalFunc) drag_drop_neuron_frame, NULL);

	gtk_box_pack_start(GTK_BOX(h_box_main), vpaned, TRUE, TRUE, 0);

	//Appelle la fonction refresh_display à intervalles réguliers si on est en mode échantillonné ('a' est la deuxième lettre de "Sampled mode")
	if (displayMode[1] == 'a') refresh_timer_id = g_timeout_add((guint)(1000 / (int) gtk_range_get_value(GTK_RANGE(refreshScale))), refresh_display, NULL);
	gtk_widget_show_all(pWindow); //Affichage du widget pWindow et de tous ceux qui sont dedans

	prom_bus_init(BROADCAST_IP);

	//si un fichier des preférences (*.jap) à été passé en ligne de commande on le charge
	if (file_preferences[0] != 0) loadJapetConfigToFile(file_preferences);

	//On regarde les options passées en ligne de commande
	while ((option = getopt(argc, argv, "i:")) != -1)
	{
		switch (option)
		{
		// -i "bus_id"
		case 'i':
			strncpy(id, optarg, BUS_ID_MAX);
			break;
			// autres options : erreur
		default:
			fprintf(stderr, "\tUsage: %s [-i bus_id] \n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	//si après chargement il n'y a pas de bus_id
	if (id[0] == 0)
	{
		fprintf(stderr, "\tUsage: %s [-i bus_id] \n", argv[0]);
		exit(EXIT_FAILURE);
	}

	for (option = 0; option < SCRIPT_LINKS_MAX; option++)
	{
		net_link[option].previous = NULL;
		net_link[option].next = NULL;
		net_link[option].type = -1;
	}

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
void init_japet(int argc, char **argv)
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

void cocheDecoche(GtkWidget *pWidget, gpointer pData)
{
	int i;
	(void) pData;

	//Identification du script à afficher ou à masquer
	for (i = 0; i < nbScripts; i++)
		if (scriptCheck[i] == pWidget)
		{
			scr[i].displayed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pWidget));
			break;
		}

	expose_event(zone3D, NULL);
}

/**
 *
 * Fonction de fermeture de Japet
 *
 * @return EXIT_SUCCESS
 */
void Close(GtkWidget *pWidget, gpointer pData) //Fonction de fermeture de Japet
{
	(void) pWidget;
	(void) pData;

	//On indique aux prométhés qu'ils doivent arreter d'envoyer des données à Japet
	japet_bus_send_message(id, "japet(%d,0)", JAPET_STOP);

	exit(EXIT_SUCCESS);
}

/**
 *
 * Un script change de plan
 *
 */
void changePlan(GtkWidget *pWidget, gpointer pData) //Un script change de plan
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
void changeValue(GtkWidget *pWidget, gpointer pData) //Modification d'une échelle
{
	int i;

	(void) pData;

	//Si le changement concerne les digits
	if (pWidget == digitsScale)
	{
		for (i = 0; i < NB_WINDOWS_MAX; i++)
			if (windowGroup[i] != NULL) expose_neurons(zoneNeurones[i], NULL);
	}
	//Si il concerne le rafraîchissement
	else if (pWidget == refreshScale)
	{
		//on détruit le timer
		g_source_destroy(g_main_context_find_source_by_id(NULL, refresh_timer_id));
		//on en crée un autre avec la nouvelle valeur de rafraîchissement
		refresh_timer_id = g_timeout_add((guint)(1000 / gtk_range_get_value(GTK_RANGE(refreshScale))), refresh_display, NULL);
	}
	else
	{
		expose_event(zone3D, NULL);//On redessine la grille avec la nouvelle échelle
	}
}

void on_search_group_button_active(GtkWidget *pWidget, script *script)
{
	(void) pWidget;

	GtkWidget *search_dialog, *search_entry, *h_box, *name_radio_button_search, *function_radio_button_search;
	int i, last_occurence = 0, stop = 0;

	//On crée une fenêtre de dialogue
	search_dialog = gtk_dialog_new_with_buttons("Recherche d'un groupe", GTK_WINDOW(pWindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_window_set_default_size(GTK_WINDOW(search_dialog), 300, 75);

	name_radio_button_search = gtk_radio_button_new_with_label_from_widget(NULL, "par nom");
	function_radio_button_search = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(name_radio_button_search), "par fonction");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(name_radio_button_search), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(function_radio_button_search), FALSE);

	h_box = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(h_box), name_radio_button_search, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(h_box), function_radio_button_search, TRUE, FALSE, 0);

	//On lui ajoute une entrée de saisie de texte
	search_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(search_entry), "nom/fonction du groupe recherché");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(search_dialog)->vbox), h_box, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(search_dialog)->vbox), search_entry, TRUE, FALSE, 0);

	gtk_widget_show_all(GTK_DIALOG(search_dialog)->vbox);

	//Aucun groupe n'est séléctionné
	selectedGroup = NULL;

	do
	{
		switch (gtk_dialog_run(GTK_DIALOG(search_dialog)))
		{
		//Si la réponse est OK
		case GTK_RESPONSE_OK:
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(name_radio_button_search)))
			{
				for (i = 0; i < script->nbGroups; i++)
				{
					if (strcmp(gtk_entry_get_text(GTK_ENTRY(search_entry)), script->groups[i].name) == 0)
					{
						//Le groupe correspondant est séléctionné
						selectedGroup = &script->groups[i];
						break;
					}
				}
				stop = 1;
			}
			else
			{
				for (i = last_occurence; i < script->nbGroups; i++)
				{
					if (strcmp(gtk_entry_get_text(GTK_ENTRY(search_entry)), script->groups[i].function) == 0)
					{
						//Le groupe correspondant est séléctionné
						selectedGroup = &script->groups[i];
						last_occurence = i + 1;
						break;
					}
				}
			}
			break;
		case GTK_RESPONSE_CANCEL:
			stop = 1;
			break;
		case GTK_RESPONSE_NONE:
			stop = 1;
			break;
		default:
			stop = 1;
			break;
		}
		expose_event(zone3D, NULL);
	} while (stop != 1);

	gtk_widget_destroy(search_dialog);
}

void on_hide_see_scales_button_active(GtkWidget *hide_see_scales_button, gpointer pData)
{
	(void) pData;

	//Si le bouton est activé
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_see_scales_button)))
	{
		//On cache le menu de droite
		gtk_widget_hide(pPane);
		//On actualise le label du boutton
		gtk_button_set_label(GTK_BUTTON(hide_see_scales_button), "Show scales");
	}
	//Si le bouton est désactivé
	else
	{
		//On montre le menu de droite
		gtk_widget_show(pPane);
		//On actualise le label du boutton
		gtk_button_set_label(GTK_BUTTON(hide_see_scales_button), "Hide scales");
	}
}

void on_check_button_draw_active(GtkWidget *check_button, gpointer data)
{
	(void) check_button;
	(void) data;

	expose_event(zone3D, NULL);
}

/**
 *
 * Clic souris
 *
 */
void button_press_event(GtkWidget *pWidget, GdkEventButton *event)
{
	int script_id, group_id, k;
	int neuron_zone_id;
	//Récupération des échelles
	int a = (int) gtk_range_get_value(GTK_RANGE(xScale));
	int b = (int) gtk_range_get_value(GTK_RANGE(yScale));
	int c = (int) gtk_range_get_value(GTK_RANGE(zxScale));
	int d = (int) gtk_range_get_value(GTK_RANGE(zyScale));

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
						if (selectedGroup->sWindow == NB_WINDOWS_MAX) //ou si toutes les valeurs des neurones de ce groupe sont déjà affichées
						{
							open_neurons_start = 1;
							move_neurons_start = 0;
							open_group = selectedGroup;
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
void key_press_event(GtkWidget *pWidget, GdkEventKey *event)
{
	int i, j;
	int libre = 0;
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
		/*Les touches "espace", "plus" et "moins" sont destinées au mode "instantanés"
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

/**
 *
 * Réactualisation de la zone 3D
 *
 */
void expose_event(GtkWidget *zone3D, gpointer pData)
{
	cairo_t *cr = gdk_cairo_create(zone3D->window); //Crée un contexte Cairo associé à la drawing_area "zone"
	(void) pData;

	//On commence par dessiner un grand rectangle blanc
	cairo_set_source_rgb(cr, WHITE);
	cairo_rectangle(cr, 0, 0, zone3D->allocation.width, zone3D->allocation.height);
	cairo_fill(cr);

	double dashes[] = { 10.0, 20.0 };

	int a = (int) gtk_range_get_value(GTK_RANGE(xScale));
	int b = (int) gtk_range_get_value(GTK_RANGE(yScale));
	int c = (int) gtk_range_get_value(GTK_RANGE(zxScale));
	int d = (int) gtk_range_get_value(GTK_RANGE(zyScale));
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

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_net_connections)))
	{
		for (i = 0; i < nb_net_link; i++)
		{
			if (net_link[i].previous != NULL && net_link[i].next != NULL)
			{
				if (net_link[i].previous->myScript->displayed && net_link[i].next->myScript->displayed)
				{
					cairo_set_source_rgb(cr, RED);
					cairo_set_line_width(cr, 3);
					if (net_link[i].type == NET_LINK_SIMPLE) cairo_set_dash(cr, dashes, 2, 0);
					cairo_move_to(cr, a * net_link[i].previous->x + c * (zMax - net_link[i].previous->myScript->z) + LARGEUR_GROUPE / 2 - 25, b * net_link[i].previous->y + d * net_link[i].previous->myScript->z);
					cairo_line_to(cr, a * net_link[i].next->x + c * (zMax - net_link[i].next->myScript->z) - LARGEUR_GROUPE / 2 + 25, b * net_link[i].next->y + d * net_link[i].next->myScript->z);
					cairo_stroke(cr);
				}
			}
		}
	}
	cairo_destroy(cr); //Puis, on détruit cr
}

/**
 *
 * Enregistrer les préférences dans un fichier
 *
 */
void japet_save_preferences(GtkWidget *pWidget, gpointer pData)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Save your scales", GTK_WINDOW(pWindow), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
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
 * Ouvrir un fichier de préférences
 *
 */
void japet_load_preferences(GtkWidget *pWidget, gpointer pData)
{
	GtkFileFilter *file_filter, *generic_file_filter;
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open scales file", GTK_WINDOW(pWindow), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	(void) pWidget;
	(void) pData;

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		file_filter = gtk_file_filter_new();
		generic_file_filter = gtk_file_filter_new();

		gtk_file_filter_add_pattern(file_filter, "*.net");
		gtk_file_filter_add_pattern(generic_file_filter, "*");

		gtk_file_filter_set_name(file_filter, "coeos/themis (.net)");
		gtk_file_filter_set_name(generic_file_filter, "all types");

		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), generic_file_filter);

		strncpy(file_preferences, filename, PATH_MAX);
		loadJapetConfigToFile(filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);

	refresh_display();

}

void defaultScale(GtkWidget *pWidget, gpointer pData)
{
	(void) pWidget;
	(void) pData;

	gtk_range_set_value(GTK_RANGE(refreshScale), REFRESHSCALE_DEFAULT);
	gtk_range_set_value(GTK_RANGE(xScale), XSCALE_DEFAULT);
	gtk_range_set_value(GTK_RANGE(yScale), YSCALE_DEFAULT);
	gtk_range_set_value(GTK_RANGE(zxScale), XGAP_DEFAULT);
	gtk_range_set_value(GTK_RANGE(zyScale), YGAP_DEFAULT);
	gtk_range_set_value(GTK_RANGE(digitsScale), DIGITS_DEFAULT);

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
void askForScripts(GtkWidget *pWidget, gpointer pData)
{
	(void) pWidget;
	(void) pData;

	if (nbScripts > 0) destroyAllScripts();//Supprime tous les scripts précédents

	expose_event(zone3D, NULL);

	japet_bus_send_message(id, "japet(%d,0)", JAPET_START);
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

void update_positions(int script_id)
{
	int i;

	gdk_threads_enter();

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

	gdk_threads_leave();

}

void update_script_display(int script_id)
{
	GtkWidget *search_icon;

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

	searchButton[script_id] = gtk_toggle_button_new();
	search_icon = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(searchButton[script_id]), search_icon);
	gtk_box_pack_start(GTK_BOX(openScripts[script_id]), searchButton[script_id], FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(searchButton[script_id]), "toggled", (GtkSignalFunc) on_search_group_button_active, &scr[script_id]);

	expose_event(zone3D, NULL);

	//Pour que les cases soient cochées par défaut
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), TRUE);
	scr[script_id].displayed = TRUE;

	if (file_preferences[0] != 0)
	{
		int i, j, k;
		Node *tree, *loading_script, *loading_group;

		tree = xml_load_file(file_preferences);
		if (xml_get_number_of_childs(tree) > 2)
		{
			loading_script = xml_get_first_child_with_node_name(tree, "script");

			for (i = 2; i < xml_get_number_of_childs(tree); i++)
			{

				if (!strcmp(scr[script_id].name, xml_get_string(loading_script, "name")))
				{
					if (xml_get_int(loading_script, "visibility") == 1)
					{
						scr[script_id].displayed = TRUE;
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), TRUE);
					}
					else
					{
						scr[script_id].displayed = FALSE;
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scriptCheck[script_id]), FALSE);
					}

					if (xml_get_number_of_childs(loading_script) > 0)
					{
						for (j = 0; j < scr[script_id].nbGroups; j++)
						{
							loading_group = xml_get_first_child_with_node_name(loading_script, "group");
							for (k = 0; k < xml_get_number_of_childs(loading_script); k++)
							{
								if (!strcmp(scr[script_id].groups[j].name, xml_get_string(loading_group, "name")))
								{
									newWindow(&scr[script_id].groups[j], xml_get_float(loading_group, "x"), xml_get_float(loading_group, "y"));
								}
								loading_group = xml_get_next_homonymous_sibling(loading_group);
							}
						}
					}
				}
				loading_script = xml_get_next_homonymous_sibling(loading_script);
			}
		}
	}

	gtk_widget_show_all(pWindow); //Affichage du widget pWindow et de tous ceux qui sont dedans
	gdk_threads_leave();
}

int get_width_height(int nb_row_column)
{
	if (nb_row_column == 1) return 75;
	else if (nb_row_column <= 16) return 300;
	else if (nb_row_column <= 128) return 400;
	else if (nb_row_column <= 256) return 700;
	else return 1000;
}

/**
 *  Affiche les neurones d'une petite fenêtre
 */
void expose_neurons(GtkWidget *zone2D, gpointer pData)
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
	int incrementation = g->nbNeurons / (g->columns * g->rows);

	valMax = g->neurons[0].s[wV];
	valMin = g->neurons[0].s[wV];
	for (i = 1; i < g->nbNeurons; i += incrementation)
	{
		if (g->neurons[i].s[wV] > valMax) valMax = g->neurons[i].s[wV];
		if (g->neurons[i].s[wV] < valMin) valMin = g->neurons[i].s[wV];
	}

	//Dimensions d'un neurone
	float largeurNeuron = (float) get_width_height(g->columns) / (float) g->columns;
	float hauteurNeuron = (float) get_width_height(g->rows) / (float) g->rows;

	//Début du dessin
	cairo_t *cr = gdk_cairo_create(zone2D->window); //Crée un contexte Cairo associé à la drawing_area "zone"

	//Affichage des neurones
	float ndg;
	int nbDigits = gtk_range_get_value(GTK_RANGE(digitsScale));

	int x = 0, y = 0;
	for (i = 0; i < g->nbNeurons; i += incrementation)
	{
		ndg = niveauDeGris(g->neurons[i].s[wV], valMin, valMax);
		cairo_set_source_rgb(cr, ndg, ndg, ndg);
		cairo_rectangle(cr, x * largeurNeuron, y * hauteurNeuron, largeurNeuron, hauteurNeuron);
		cairo_fill(cr);

		if (ndg > 0.5) color(cr, *g);
		else clearColor(cr, *g);

		cairo_move_to(cr, x * largeurNeuron + 4, (y + 0.5) * hauteurNeuron);

		char valeurNeurone[nbDigits + 1];

		x++;
		if (x >= g->columns)
		{
			x = 0;
			y++;
		}

		//S'il y a assez de place sur le neurone pour afficher sa valeur avec le nombre de digits demandé
		if (largeurNeuron - 1 > 4 * (nbDigits + 1) && hauteurNeuron > 16)
		{
			sprintf(valeurNeurone, "%f", (g->neurons[i].s[wV] / 100));
			valeurNeurone[nbDigits] = '\0';
			if (g->neurons[i].s[wV] >= pow(10, nbDigits) || g->neurons[i].s[wV] <= -pow(10, nbDigits - 1)) for (j = 0; j < nbDigits; j++)
				valeurNeurone[j] = '#';
			//Si le nombre de digits demandé est insuffisant pour afficher entièrement la partie entière d'une valeur, des # s'affichent à la place de cette valeur
			cairo_show_text(cr, valeurNeurone);
		}
	}

	cairo_stroke(cr);//Le contenu de cr est appliqué sur "zoneNeurones"

	if (windowGroup[currentWindow] == selectedGroup) //Si le groupe affiché dans cette fenêtre est sélectionné
	{
		cairo_set_line_width(cr, 10); //Traits plus épais
		cairo_set_source_rgb(cr, RED);
		cairo_move_to(cr, 0, get_width_height(g->rows));
		cairo_line_to(cr, get_width_height(g->columns) - 1, get_width_height(g->rows));
		cairo_stroke(cr); //Le contenu de cr est appliqué sur "zoneNeurones"
		cairo_set_line_width(cr, GRID_WIDTH); //Retour aux traits fins*/
	}
	cairo_destroy(cr); //Destruction du calque
}

int button_press_neurons(GtkWidget *zone2D, GdkEventButton *event)
{
	int i, group_id, currentWindow;

	//On cherche le numéro de la fenêtre concernée
	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (zoneNeurones[i] == zone2D)
		{
			currentWindow = i;
			break;
		}

	group* g = windowGroup[currentWindow]; //On va chercher l'adresse du groupe concerné

	if (event->button == 2) //Si clic molette dans une petite fenêtre, on la supprime
	{
		g->sWindow = NB_WINDOWS_MAX; //Cette valeur, pour ce groupe, n'est plus affichée

		for (group_id = 0; group_id < windowGroup[currentWindow]->myScript->nbGroups; group_id++)
		{
			if (windowGroup[currentWindow] == &windowGroup[currentWindow]->myScript->groups[group_id])
			{
				japet_bus_send_message(id, "japet(%d,%d) %s", JAPET_STOP_GROUP, group_id, windowGroup[currentWindow]->myScript->name);
			}
		}
		windowGroup[currentWindow] = NULL; //Plus aucun groupe n'est associé à cette fenêtre

		if (currentWindow == selectedWindow) selectedWindow = NB_WINDOWS_MAX; //Si on supprime la fenêtre sélectionnée, il n'y a plus de fenêtre sélectionnée

		gtk_container_remove(GTK_CONTAINER(zone_neurons), pFrameNeurones[currentWindow]);//On efface la fenêtre
		zoneNeurones[currentWindow] = NULL;
		pFrameNeurones[currentWindow] = NULL;
		usedWindows--;
	}
	else if (event->button == 3) //Si droit, la fenêtre peut être déplacée
	{
		move_neurons_old_x = event->x;
		move_neurons_old_y = event->y;
		move_neurons_start = 1;
		move_neurons_frame_id = currentWindow;
	}
	else if (event->button == 1) //Si clic gauche la fenêtre est séléctionnée
	{
		selectedWindow = currentWindow;
		selectedGroup = g; //Le groupe associé à la fenêtre dans laquelle on a cliqué est également sélectionné
	}

	//On actualise l'affichage
	expose_event(zone3D, NULL);

	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (zoneNeurones[i] != NULL) expose_neurons(zoneNeurones[i], NULL);

	return TRUE;
}

void switch_output(GtkWidget *pWidget, group *group)
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

	if (move_neurons_frame_id != -1 && move_neurons_start == 1)
	{
		gtk_layout_move(GTK_LAYOUT(zone_neurons), pFrameNeurones[move_neurons_frame_id], ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
		windowPosition[move_neurons_frame_id].x = ((int) (event->x / 25)) * 25;
		windowPosition[move_neurons_frame_id].y = ((int) (event->y / 25)) * 25;
		move_neurons_frame_id = -1;
		move_neurons_start = 0;
	}
	else if (open_group != NULL && open_neurons_start == 1)
	{
		newWindow(open_group, ((int) (event->x / 25)) * 25, ((int) (event->y / 25)) * 25);
		open_neurons_start = 0;
	}
}

//--------------------------------------------------5. "CONSTRUCTEURS"---------------------------------------------

void newScript(script *script, char *name, char *machine, int z, int nbGroups)
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
 * @param script_id : id du script à supprimer
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

	for (i = 0; i < nb_net_link; i++)
	{
		net_link[i].previous = NULL;
		net_link[i].next = NULL;
	}

	nb_net_link = 0;

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
			gtk_container_remove(GTK_CONTAINER(zone_neurons), pFrameNeurones[i]);
			windowGroup[i] = NULL;
			zoneNeurones[i] = NULL;
			pFrameNeurones[i] = NULL;
		}

	usedWindows = 0;

	ivyServerNb = 0;

	nbSnapshots = 1; //Par défaut, 1 série de valeurs est enregistrée à leur création dans le tableau "buffer" de chaque neurone
}

/**
 * Créer un script.
 */
void newGroup(group *g, script *myScript, char *name, char *function, float learningSpeed, int nbNeurons, int rows, int columns, int y, int nbLinksTo, int firstNeuron)
{
	g->myScript = myScript;

	int tailleName = strlen(name);
	g->name = (char*) malloc((tailleName + 1) * sizeof(char));
	strncpy(g->name, name, tailleName);
	g->name[tailleName] = '\0';

	int tailleFonction = strlen(function);
	g->function = (char*) malloc((tailleFonction + 1) * sizeof(char));
	sprintf(g->function, "%s", function);
	g->function[tailleFonction] = '\0';

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
 * Créer un nouveau neurone
 */
void newNeuron(neuron *neuron, group *myGroup, float s, float s1, float s2, float pic, int x, int y)
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
 * Mise un jour d'un neurone quand Prométhé envoie de nouvelles données
 */
//Actuellement, cette fonction n'est pas utilisée. Elle sera peut-être utile pour le mode "instantanés"
void updateNeuron(neuron *neuron, float s, float s1, float s2, float pic)
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
void color(cairo_t *cr, group group)
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
void clearColor(cairo_t *cr, group group)
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

void dessinGroupe(cairo_t *cr, int a, int b, int c, int d, group *g, int z, int zMax)
{
	int x = g->x, y = g->y;

	if (g == selectedGroup) cairo_set_source_rgb(cr, RED);
	else color(cr, *g);
	cairo_rectangle(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z - HAUTEUR_GROUPE / 2, LARGEUR_GROUPE, HAUTEUR_GROUPE);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, WHITE);
	cairo_set_font_size(cr, 8);
	cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z);
	cairo_show_text(cr, g->name);
	cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z + HAUTEUR_GROUPE / 2);

	cairo_show_text(cr, g->function);

	int i;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_connections)))
	{
		//Dessin des liaisons aboutissant à ce groupe
		for (i = 0; i < g->nbLinksTo; i++)
			if (g->previous[i]->myScript->displayed == TRUE)
			{
				int x1 = g->previous[i]->x, y1 = g->previous[i]->y, z1 = g->previous[i]->myScript->z; //Coordonnées du groupe situé avant la liaison

				if (g->previous[i]->myScript == g->myScript) color(cr, *g);

				cairo_set_line_width(cr, 0.8); //Trait épais représentant une liaison entre groupes

				cairo_move_to(cr, a * x1 + c * (zMax - z1) + LARGEUR_GROUPE / 2, b * y1 + d * z1); //Début de la liaison (à droite du groupe prédécesseur)
				cairo_line_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z); //Fin de la liaison (à gauche du groupe courant)

				cairo_stroke(cr);
			}
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_draw_net_connections)))
	{
		for (i = 0; i < nb_net_link; i++)
		{
			if (net_link[i].previous == g)
			{
				cairo_set_source_rgb(cr, GREY);
				cairo_set_line_width(cr, 3);
				cairo_move_to(cr, a * x + c * (zMax - z) + LARGEUR_GROUPE / 2, b * y + d * z - 10);
				cairo_line_to(cr, a * x + c * (zMax - z) + LARGEUR_GROUPE / 2 + 10, b * y + d * z - 10);

				if (net_link[i].type == NET_LINK_ACK || net_link[i].type == NET_LINK_BLOCK_ACK)
				{
					cairo_move_to(cr, a * x + c * (zMax - z) + LARGEUR_GROUPE / 2, b * y + d * z);
					cairo_set_source_rgb(cr, INDIGO);
					cairo_show_text(cr, "ack");
				}
				cairo_stroke(cr);
			}
			else if (net_link[i].next == g)
			{
				cairo_set_source_rgb(cr, GREY);
				cairo_set_line_width(cr, 3);
				cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2 - 10, b * y + d * z - 10);
				cairo_line_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * y + d * z - 10);

				if (net_link[i].type == NET_LINK_BLOCK || net_link[i].type == NET_LINK_BLOCK_ACK)
				{
					cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2 - 25, b * y + d * z);
					cairo_set_source_rgb(cr, RED);
					cairo_show_text(cr, "block");
				}
				cairo_stroke(cr);
			}
		}
	}

}

/**
 *
 * Calcule la coordonnée x d'un groupe en fonction de ses prédécesseurs
 *
 */
void findX(group *group)
{
	int i;

	if (group->previous == NULL)
	{
		group->x = 1;
	}
	else
	{
		for (i = 0; i < group->nbLinksTo; i++)
		{
			if (group->previous[i]->knownX == FALSE) findX(group->previous[i]);
			if (group->previous[i]->x >= group->x) group->x = group->previous[i]->x + 1;
		}

		for (i = 0; i < nb_net_link; i++)
		{
			if (net_link[i].next == group && net_link[i].previous != NULL)
			{
				if (net_link[i].type == NET_LINK_BLOCK || net_link[i].type == NET_LINK_BLOCK_ACK)
				{
					if (!net_link[i].previous->knownX) findX(net_link[i].previous);
					if (net_link[i].next->x < net_link[i].previous->x) net_link[i].next->x = net_link[i].previous->x + 1;
				}
			}
		}

		//	group->x = Max + 1;
	}

	group->knownX = TRUE;
}

/**
 *
 * Calcule la coordonnée y d'un groupe
 *
 */
void findY(group *group)
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

void newWindow(group *g, float pos_x, float pos_y)
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
	usedWindows++; //Puis on incrémente le nombre de fenêtres utilisées

	windowValue[currentWindow] = 1;

	g->sWindow = currentWindow; //On enregistre que c'est CETTE fenêtre qui affiche CETTE valeur des neurones de CE groupe

	//Création de la petite fenêtre
	pFrameNeurones[currentWindow] = gtk_frame_new(""); //Titre de la fenêtre

	gtk_layout_put(GTK_LAYOUT(zone_neurons), pFrameNeurones[currentWindow], pos_x, pos_y);
	windowPosition[currentWindow].x = pos_x;
	windowPosition[currentWindow].y = pos_y;

	button_frame = gtk_toggle_button_new_with_label(g_strconcat(g->name, " - ", "s1", NULL));
	g_signal_connect(GTK_OBJECT(button_frame), "toggled", (GtkSignalFunc) switch_output, g);
	gtk_frame_set_label_widget(GTK_FRAME(pFrameNeurones[currentWindow]), button_frame);

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
	int i;

	for (i = 0; i < NB_WINDOWS_MAX; i++)
		if (windowGroup[i] != NULL)
		{
			gtk_widget_set_size_request(zoneNeurones[i], get_width_height(windowGroup[i]->columns), get_width_height(windowGroup[i]->rows));
			expose_neurons(zoneNeurones[i], NULL);
		}
	gtk_widget_show_all(neurons_frame);
}

/*
 * Mise en page du xml dans le fichier de sauvegarde des préférences (*.jap)
 */
const char* whitespace_callback_japet(mxml_node_t *node, int where)
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

	if (!strcmp(name, "properties") || !strcmp(name, "script") || !strcmp(name, "bus_id"))
	{
		if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) return ("\t");
	}
	else if (!strcmp(name, "group"))
	{
		if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) return ("\t\t");
	}

	return (NULL);
}
void saveJapetConfigToFile(char *filename)
{
	int script_id, current_window;
	Node *tree, *bus_id, *properties, *script, *group;

	tree = mxmlNewXML("1.0");

	bus_id = mxmlNewElement(tree, "bus_id");
	xml_set_string(bus_id, "name", id);

	properties = mxmlNewElement(tree, "properties");
	xml_set_int(properties, "refresh", gtk_range_get_value(GTK_RANGE(refreshScale)));

	xml_set_int(properties, "x_scale", gtk_range_get_value(GTK_RANGE(xScale)));

	xml_set_int(properties, "y_scale", gtk_range_get_value(GTK_RANGE(yScale)));

	xml_set_int(properties, "z_x_scale", gtk_range_get_value(GTK_RANGE(zxScale)));

	xml_set_int(properties, "z_y_scale", gtk_range_get_value(GTK_RANGE(zyScale)));

	xml_set_int(properties, "digits", gtk_range_get_value(GTK_RANGE(digitsScale)));

	for (script_id = 0; script_id < nbScripts; script_id++)
	{
		script = mxmlNewElement(tree, "script");
		xml_set_string(script, "name", scr[script_id].name);

		if (scr[script_id].displayed) xml_set_int(script, "visibility", 1);
		else xml_set_int(script, "visibility", 0);

		for (current_window = 0; current_window < usedWindows; current_window++)
		{
			if (windowGroup[current_window]->myScript == &scr[script_id])
			{
				group = mxmlNewElement(script, "group");
				xml_set_string(group, "name", windowGroup[current_window]->name);
				xml_set_int(group, "x", windowPosition[current_window].x);
				xml_set_int(group, "y", windowPosition[current_window].y);
			}
		}
	}

	xml_save_file(filename, tree, whitespace_callback_japet);
}

void loadJapetConfigToFile(char *filename)
{
	int script_id, i, j, k;
	Node *tree, *loading_node, *loading_group;

	tree = xml_load_file(filename);

	loading_node = xml_get_first_child_with_node_name(tree, "bus_id");
	strcpy(id, xml_get_string(loading_node, "name"));

	loading_node = xml_get_first_child_with_node_name(tree, "properties");
	gtk_range_set_value(GTK_RANGE(refreshScale), xml_get_int(loading_node, "refresh"));

	gtk_range_set_value(GTK_RANGE(xScale), (double) xml_get_int(loading_node, "x_scale"));

	gtk_range_set_value(GTK_RANGE(yScale), xml_get_int(loading_node, "y_scale"));

	gtk_range_set_value(GTK_RANGE(zxScale), xml_get_int(loading_node, "z_x_scale"));

	gtk_range_set_value(GTK_RANGE(zyScale), xml_get_int(loading_node, "z_y_scale"));

	gtk_range_set_value(GTK_RANGE(digitsScale), xml_get_int(loading_node, "digits"));

	for (script_id = 0; script_id < nbScripts; script_id++)
		if (xml_get_number_of_childs(tree) > 2)
		{
			loading_node = xml_get_first_child_with_node_name(tree, "script");

			for (i = 2; i < xml_get_number_of_childs(tree); i++)
			{

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

					if (xml_get_number_of_childs(loading_node) > 0)
					{
						for (j = 0; j < scr[script_id].nbGroups; j++)
						{
							loading_group = xml_get_first_child_with_node_name(loading_node, "group");
							for (k = 0; k < xml_get_number_of_childs(loading_node); k++)
							{
								if (!strcmp(scr[script_id].groups[j].name, xml_get_string(loading_group, "name")))
								{
									newWindow(&scr[script_id].groups[j], xml_get_float(loading_group, "x"), xml_get_float(loading_group, "y"));
								}
								loading_group = xml_get_next_homonymous_sibling(loading_group);
							}
						}
					}
				}
				loading_node = xml_get_next_homonymous_sibling(loading_node);
			}
		}
}

void fatal_error(const char *name_of_file, const char *name_of_function, int numero_of_line, const char *message, ...)
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
