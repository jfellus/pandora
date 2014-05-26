/**
 * colors.h
 * 
 * Auteur : Brice Errandonea
 * 
 */

#include <gdk/gdk.h>

#define COLORS_MAX 7

/**
 * Ce fichier contient les diff��rents define permettant de simplifier la gestion des couleurs.
 */
//--------------------------------------------------COULEURS---------------------------------------------------------
//Pour le dessin (et le texte dans les zones de dessin)
#define BLACK 	0.0, 0.0, 0.0,1.0	//Grille
#define WHITE 	1, 1, 1,1.0	//Fond
#define RED 		1, 0, 0,1.0	//Groupe s��lectionn��, fen��tre s��lectionn��e et liaisons entre scripts
#define LIGHTRED    210.0/255.0, 161.0/255.0, 161.0/255.0,1.0 //Groupe s��lectionn��, fen��tre s��lectionn��e et liaisons entre scripts
#define DARKRED 	0.5, 0, 0,1.0

#define DARKGREEN	0, 0.5, 0,1.0	//Plan z = 0
#define LIGHTGREEN  127.0/255.0, 219.0/255.0, 140.0/255.0,1.0   //Plan z = 0
#define GREEN 		0, 1, 0,1.0		//Plan z = 0 actualis��
#define DARKMAGENTA	0.5, 0, 0.5,1.0	//Plan z = 1
#define LIGHTMAGENTA  230.0/255.0, 154.0/255.0, 241.0/255.0,1.0   //Plan z = 0
#define MAGENTA		1, 0, 1,1.0		//Plan z = 1 actualis��
#define	DARKYELLOW	0.5, 0.5, 0,1.0	//Plan z = 2
#define YELLOW 		1, 1, 0,1.0 	//Plan z = 2 actualis��
#define DARKBLUE	0.1, 0.1, 0.6,1.0 	//Plan z = 3
#define LIGHTBLUE   128.0/255.0, 208.0/255.0, 217.0/255.0,1.0    //Plan z = 3
#define BLUE 		0, 0, 1,1.0 	//Plan z = 3 actualis��
#define BLUE_TRAN   0, 0, 1,0.7
#define DARKCYAN	0, 0.5, 0.5,1.0	//Plan z = 4
#define CYAN 		0, 1, 1,1.0		//Plan z = 4 actualis��
#define LIGHTCYAN   193.0/255.0, 255.0/255.0, 236.0/255.0,1.0    //Plan z = 3
#define DARKORANGE	0.5, 0.25, 0,1.0 	//Plan z = 5
#define LIGHTORANGE  244.0/255.0, 192.0/255.0, 101.0/255.0,1.0    //Plan z = 5
#define ORANGE 		1, 0.5, 0,1.0 	//Plan z = 5 actualis��
#define DARKGREY	0.25, 0.25, 0.25,1.0	//Plan z = 6
#define GREY		0.5, 0.5, 0.5,1.0	//Plan z = 6 actualis��
#define SAND 	1, 1, 0.5,1.0	//Pas de neurone �� cet endroit
#define SALMON 	1, 0.5, 0.5,1.0
#define INDIGO	0.5, 0.5, 1,1.0 //Fen��tre montrant les neurones du groupe s��lectionn��
#define FUCHSIA	1, 0, 0.5,1.0
#define APPLE	0.5, 1, 0,1.0
#define MINT	0, 1, 0.5,1.0
#define PURPLE 	0.5, 0, 1,1.0
#define COBALT	0, 0.5, 1,1.0
#define ANISE	0.5, 1, 0.5,1.0
#define LIGHTANISE   212.0/255.0, 225.0/255.0, 151.0/255.0,1.0


const GdkRGBA colors[COLORS_MAX];
