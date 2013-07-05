

#include "pandora_save.h"
#include "basic_tools.h"
//TODO Fonctionne : à commenter et optimiser et regler le probleme du 0, !
// Si le groupe change : rappeler first call
// Si le groupe





void destroy_saving_ref(type_script *scripts_used[NB_SCRIPTS_MAX])
{
	int N=0;
		int indScript, indGroup;


		for (indScript=0; indScript<number_of_scripts;indScript++)
		{
			N=scripts_used[indScript]->number_of_groups;
			for(indGroup=0;indGroup<N;indGroup++)
			{
				if (scripts_used[indScript]->groups[indGroup].on_saving==1 || scripts_used[indScript]->groups[indGroup].associated_file!=NULL)
				{
					fclose(scripts_used[indScript]->groups[indGroup].associated_file);
					printf("\n\nCloture du fichier associé à %s\n\n",scripts_used[indScript]->groups[indGroup].name);
					scripts_used[indScript]->groups[indGroup].on_saving=0; //par sécurité mais normalement c'est fait ailleurs.
				}

			}
		}

}

void destroy_saving_ref_one(type_script* script_used)
{
	int N=0;
		int indGroup;



			N=script_used->number_of_groups;
			for(indGroup=0;indGroup<N;indGroup++)
			{
				if (script_used->groups[indGroup].on_saving==1)
				{
					fclose(script_used->groups[indGroup].associated_file);
					printf("\n\nCloture du fichier associé à %s\n\n",script_used->groups[indGroup].name);
					script_used->groups[indGroup].on_saving=0; //par sécurité mais normalement c'est fait ailleurs.
				}

			}


}

void file_create(type_group *used_group)
{
	char file_path_ori[MAX_LENGHT_PATHNAME];
	char file_path[MAX_LENGHT_PATHNAME]=PATH_NAME;
	char num[4];
	FILE* test=NULL;
	int i=0;

	//strcpy(file_path,PATH_NAME);

	//Creation du nom de fichier
	strcat(file_path,used_group->name);
	strcat(file_path,"_");
	strcat(file_path,used_group->script->name);
	strcat(file_path,"_save");
//	printf("\netat 1=%s",file_path);

	//on test si un fichier a cet emplacement existe déja
	strcpy(file_path_ori,file_path);
	do // fonctionne mais peut bugger si il fait trop de fois la boucle
	{
		i++;
		test=fopen(file_path,"r");


		if (test!=NULL)
			{
				strcpy(file_path,file_path_ori);
				fclose(test);
				strcat(file_path,"_");
				sprintf(num, "%d", i);
				strcat(file_path,num);
				//printf("\npathname =%s",file_path);
			}

	} while(test!=NULL);

	used_group->associated_file = fopen(file_path, "a+");
	printf("\npathname final=%s",file_path);

	if (used_group->associated_file != NULL)
	{
		used_group->on_saving=1;
		fprintf(used_group->associated_file,"%d\n%d\n",used_group->rows,used_group->columns);


		architecture_display_update(architecture_display,NULL); // pour remettre à jour l'affichage quand les groupes sont on saving

	}
	else
	{
		// On affiche un message d'erreur et on quitte
		used_group->on_saving=0;
		printf("Impossible d'ouvrir ou de creer les fichiers de sauvegarde %s",file_path);

	}

}

void continuous_saving(type_group *used_group)
{

	int i,j,incrementation;
	struct timespec tp;

	incrementation = used_group->number_of_neurons / (used_group->columns * used_group->rows);
	setlocale(LC_NUMERIC, "C");
	//printf("\nincrementation=%d\n",incrementation);


	if (used_group->on_saving==0) // Si le groupe n'est pas déja en train de se sauvegarder alors on doit creer le nom de fichier
	{
		file_create(used_group);
	}


	if (used_group->on_saving==1)
	{
		clock_gettime ( CLOCK_MONOTONIC, &tp );
		fprintf(used_group->associated_file,"#Time: %li\n",tp.tv_nsec);
		for (i=0;i<used_group->rows;i++)
		{
			//lignes
			for(j=0;j<used_group->columns * incrementation;j+= incrementation)
			{
				//colonnes
				fprintf(used_group->associated_file,"%f ",used_group->neurons[i*used_group->columns * incrementation + j].s1);//TODO pour le moment avec s1 : fonctionne mais un peu par miracle !!! ne devrais pas !!!
			}
			fprintf(used_group->associated_file,"\n");
		}
		fprintf(used_group->associated_file,"\n");



	}

}

//peut etre inutile
/*
void tabula_rasa_selected(type_script *scripts_to_deselect[NB_SCRIPTS_MAX])
{
	int N=0;
	int indScript, indGroup;

	for (indScript=0; indScript<number_of_scripts;indScript++)
	{
		N=scripts_to_deselect[indScript]->number_of_groups;
		for(indGroup=0;indGroup<N;indGroup++)
		{
			scripts_to_deselect[indScript]->groups[indGroup].selected_for_save=0;
		}
	}

}
*/
/*
//peut etre inutile
gboolean compare_list(char* string_table, char *compared,state_of_saving* state)
{
	int indTable;
	gboolean retur=0;
	for(indTable=0;indTable<state->nbr_group_to_save;indTable++)
	{
		if(strcmp(&(string_table[indTable]),compared)) retur=1;

	}
	return retur;
}
*/
