/**
 * colors.h
 * 
 * Auteur : Brice Errandonea
 * 
 */

//--------------------------------------------------COULEURS---------------------------------------------------------

//Pour le dessin (et le texte dans les zones de dessin)
#define BLACK 	0.0, 0.0, 0.0,1.0	//Grille
#define WHITE 	1, 1, 1,1.0	//Fond
#define RED 		1, 0, 0,1.0	//Groupe sélectionné, fenêtre sélectionnée et liaisons entre scripts
#define LIGHTRED    210.0/255.0, 161.0/255.0, 161.0/255.0,1.0 //Groupe sélectionné, fenêtre sélectionnée et liaisons entre scripts
#define DARKRED 	0.5, 0, 0,1.0

#define DARKGREEN	0, 0.5, 0,1.0	//Plan z = 0
#define LIGHTGREEN  127.0/255.0, 219.0/255.0, 140.0/255.0,1.0   //Plan z = 0
#define GREEN 		0, 1, 0,1.0		//Plan z = 0 actualisé
#define DARKMAGENTA	0.5, 0, 0.5,1.0	//Plan z = 1
#define LIGHTMAGENTA  230.0/255.0, 154.0/255.0, 241.0/255.0,1.0   //Plan z = 0
#define MAGENTA		1, 0, 1,1.0		//Plan z = 1 actualisé
#define	DARKYELLOW	0.5, 0.5, 0,1.0	//Plan z = 2
#define YELLOW 		1, 1, 0,1.0 	//Plan z = 2 actualisé
#define DARKBLUE	0.1, 0.1, 0.6,1.0 	//Plan z = 3
#define LIGHTBLUE   128.0/255.0, 208.0/255.0, 217.0/255.0,1.0    //Plan z = 3
#define BLUE 		0, 0, 1,1.0 	//Plan z = 3 actualisé
#define BLUE_TRAN   0, 0, 1,0.7
#define DARKCYAN	0, 0.5, 0.5,1.0	//Plan z = 4
#define CYAN 		0, 1, 1,1.0		//Plan z = 4 actualisé
#define LIGHTCYAN   193.0/255.0, 255.0/255.0, 236.0/255.0,1.0    //Plan z = 3
#define DARKORANGE	0.5, 0.25, 0,1.0 	//Plan z = 5
#define LIGHTORANGE  244.0/255.0, 192.0/255.0, 101.0/255.0,1.0    //Plan z = 5
#define ORANGE 		1, 0.5, 0,1.0 	//Plan z = 5 actualisé
#define DARKGREY	0.25, 0.25, 0.25,1.0	//Plan z = 6
#define GREY		0.5, 0.5, 0.5,1.0	//Plan z = 6 actualisé
#define SAND 	1, 1, 0.5,1.0	//Pas de neurone à cet endroit
#define SALMON 	1, 0.5, 0.5,1.0
#define INDIGO	0.5, 0.5, 1,1.0 //Fenêtre montrant les neurones du groupe sélectionné
#define FUCHSIA	1, 0, 0.5,1.0
#define APPLE	0.5, 1, 0,1.0
#define MINT	0, 1, 0.5,1.0
#define PURPLE 	0.5, 0, 1,1.0
#define COBALT	0, 0.5, 1,1.0
#define ANISE	0.5, 1, 0.5,1.0
#define LIGHTANISE   212.0/255.0, 225.0/255.0, 151.0/255.0,1.0

//Pour le texte des labels
#define TBLACK "#000000"
#define TWHITE "#FFFFFF"

#define TRED "#FF0000"
#define TGREEN "#00FF00"
#define TBLUE "#0000FF"
#define TMAGENTA "#FF00FF"
#define TCYAN "#00FFFF"
#define TYELLOW "#FFFF00"

#define TDARKRED "#800000"
#define TDARKGREEN "#008000"
#define TDARKBLUE "#000080"
#define TDARKMAGENTA "#800080"
#define TDARKCYAN "#008080"
#define TDARKYELLOW "#808000"

#define TDARKGREY "#404040"
#define TGREY "#808080"

#define TDARKORANGE "#804000"
#define TORANGE "#FF8000"

#define TFUCHSIA "#FF0080"
#define TAPPLE "#80FF00"
#define TMINT "#00FF80"
#define TPURPLE "#8000FF"
#define TCOBALT "#0080FF"

#define TSAND	"#FFFF80"
#define TSALMON "#FF8080"
#define TANISE "#80FF80"
#define TINDIGO "#8080FF" 

//Ceci évite d'inclure graphicTx.h incompatible avec GTK3.0 tout en assurant la compatibilité de pandora.
#define noir 0
#define blanc 16
#define gris 8
#define rouge 17
#define vert 18
#define bleu 19
#define ciel 20
#define jaune 21
#define violet 22
#define marine 23
#define pelouse 24
#define bordeau 25
#define kaki 26

