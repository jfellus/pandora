
/** contenu de base pour typedef struct type_groupe */

/** numero symbolique utile pour des scripts symboliques complexes */
    char no_name[SIZE_NO_NAME]; 
/** nom du groupe pour entree ou sortie Numero du groupe pour
utilisateur */
    char nom[TAILLE_CHAINE];    
    int no;                     
/**   position X du centre du groupe      */
    int posx;                   
/**   position Y du centre du groupe      */
    int posy;			
/**   position du centre du groupe      */
    int p_posx;                 
/**  dans le debug de promethe          */
    int p_posy;                 
/** 1: orientation inverse pour dessin  */
    int reverse;                
/** nbre de voies de liaisons           */
    int nbre_voie;              

/** pour dire si un groupe est en faute */
    int fault;			
/** pour eviter de le re-etudier        */
    int deja_active;            
    int deja_appris;		
/** echelle temporelle pour savoir quand l'executer */
    int ech_temps;		
/** no du premier neurone du groupe     */
    int premier_ele;            

/** pour les futures ameliorations de l'IHM : selection multiple de
groupes a deplacer... */
    int selected;	
  /** 1 si le groupe doit etre affiche lors du debug, 0 sinon */
    int debug;         
/** type du groupe */ 
    int type; 		

/** specifie si c'est une sortie pour rt_token : numero correspondant
a la priorite du token */
    my_int(type2);      

/** nbre de lignes de la carte */
    my_int(taillex);    
/** nbre de colonnes de la carte        */
    my_int(tailley);    
/** nbre de neurones dans le groupe     */
    my_int(nbre);       
/** seuil pour chaque neurone si diff 0 */
    my_float(seuil);    
/** pour neurones a trois etats, colonne */
    my_float(tolerance);
    my_int(dvp);	
/** distance voisinage d diff et compet */
    my_int(dvn);        
/** coeff pour le renforcement dans PTM */
    my_float(alpha);    
/** estimation normalisation pour PTM   */
    my_float(nbre_de_1);
/** constante pour les fcts de sortie   */
    my_float(sigma_f);  
/** simulation speed and learning rate  */
    my_float(learning_rate);
    my_float(simulation_speed);
    my_float(noise_level);

/** init diff si carte           */
    float tableau_gaussienne[taille_max_tableau_diff_gaussienne];   

/** valeur de retour lors de l'utilisation du gpe: 0 si le groupe n'a
pu s'executer, 1 si OK, 2 si un break intervient et 3 si le groupe a
deja ete detruit et le thread normalement recupere */
    int return_value; 
/**Rend le groupe directement activable. Gestion Execution temps reel */
    int rttoken;               
/**nombre de threads lors du calcul de la mise a jour M.M. 07/02/07*/
    my_int(nb_threads);	       
/** flag pour gerer les breakpoints */
    int breakpoint;            

