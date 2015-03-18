/*
 Copyright  ETIS — ENSEA, Université de Cergy-Pontoise, CNRS (1991-2014)
 promethe@ensea.fr

 Authors: P. Andry, J.C. Baccon, D. Bailly, A. Blanchard, S. Boucena, A. Chatty, N. Cuperlier, P. Delarboulas, P. Gaussier,
 C. Giovannangeli, C. Grand, L. Hafemeister, C. Hasson, S.K. Hasnain, S. Hanoune, J. Hirel, A. Jauffret, C. Joulain, A. Karaouzène,
 M. Lagarde, S. Leprêtre, M. Maillard, B. Miramond, S. Moga, G. Mostafaoui, A. Pitti, K. Prepin, M. Quoy, A. de Rengervé, A. Revel ...

 See more details and updates in the file AUTHORS

 This software is a computer program whose purpose is to simulate neural networks and control robots or simulations.
 This software is governed by the CeCILL v2.1 license under French law and abiding by the rules of distribution of free software.
 You can use, modify and/ or redistribute the software under the terms of the CeCILL v2.1 license as circulated by CEA, CNRS and INRIA at the following URL "http://www.cecill.info".
 As a counterpart to the access to the source code and  rights to copy, modify and redistribute granted by the license,
 users are provided only with a limited warranty and the software's author, the holder of the economic rights,  and the successive licensors have only limited liability.
 In this respect, the user's attention is drawn to the risks associated with loading, using, modifying and/or developing or reproducing the software by the user in light of its specific status of free software,
 that may mean  that it is complicated to manipulate, and that also therefore means that it is reserved for developers and experienced professionals having in-depth computer knowledge.
 Users are therefore encouraged to load and test the software's suitability as regards their requirements in conditions enabling the security of their systems and/or data to be ensured
 and, more generally, to use and operate it in the same conditions as regards security.
 The fact that you are presently reading this means that you have had knowledge of the CeCILL v2.1 license and that you accept its terms.
 */
/**
 * pandora_language.c
 *
 *  Created on: Mar 13, 2015
 *      Author: Nils Beaussé
 **/

#include "pandora_language.h"

chaine_pando chaines[nbre_chaines_totales];

void init_chaines_nom(chaine_pando* chaines_a_init)
{
  int i=0;
  for(i=0;i<nbre_chaines_totales;i++)
  {
    (chaines_a_init[i]).nom=nom_chaines[i];
  }

}

void lecture_xml_language()
{
  char filename_francais[100]="/home/bin_leto_prom/simulator/pandora/resources/francais.xml";
  char filename_anglais[100]="/home/bin_leto_prom/simulator/pandora/resources/english.xml";
int script_id, group_id, value, value2;
  Node *tree, *nom_node, *group_node, *preferences_node;
  type_group *group;
  type_script *script;

  switch(LANGUE_ACTUELLE)
  {
  case FRANCAIS :
    tree=recurcively_load_tree_pando(filename_francais);
  break;
  case ENGLISH :
    tree= recurcively_load_tree_pando(filename_anglais);
  break;
  }
  //tree=recurcively_load_tree_pando(filename)
  //allouer_toute_chaine(tree)



}



Node *recurcively_load_tree_pando(const char *filename)
{
    FILE *file;
    int i;
    mxml_element_t *element;
    mxml_node_t *main_tree, *current_node, *loaded_tree, *loaded_node,  *current_child_node;

    file = fopen(filename, "r");
    if (file == NULL) EXIT_ON_ERROR("File '%s' can not be opened.", filename);
    main_tree = mxmlLoadFile(NULL, file, MXML_NO_CALLBACK);
    fclose(file);

    for (current_node = mxmlFindElement(main_tree, main_tree, NULL, "file", NULL, MXML_DESCEND);
            current_node != NULL;
            current_node = mxmlFindElement(current_node, main_tree, NULL, "file", NULL, MXML_DESCEND))
    {
        loaded_tree = recurcively_load_tree(mxmlElementGetAttr(current_node, "file"));
        loaded_node = mxmlFindElement(loaded_tree, loaded_tree, current_node->value.element.name, NULL, NULL, MXML_DESCEND);

        mxmlElementDeleteAttr(current_node, "file");

        element = &current_node->value.element;

        for (i=0; i<element->num_attrs; i++)
        {
            mxmlElementSetAttr(loaded_node, element->attrs[i].name, element->attrs[i].value);
        }

        for (current_child_node = mxmlFindElement(main_tree, main_tree, NULL, NULL, NULL, MXML_NO_DESCEND);
                current_child_node != NULL;
                current_child_node = mxmlFindElement(current_child_node, main_tree, NULL, NULL, NULL, MXML_NO_DESCEND))
        {
            mxmlAdd(loaded_node, MXML_ADD_AFTER, NULL, current_child_node);
        }

        mxmlAdd(current_node->parent, MXML_ADD_AFTER, current_node, loaded_node);
        mxmlRemove(current_node);
        mxmlDelete(current_node);
    }

    return main_tree;
}

/*
void dev_create_all_devices(Node tree_of_devices)
{
    Device *device;
    Node *node_of_device;

    // On compte et alloue le tableaus d'appareils declares dans le .de
    number_of_devices = xml_get_number_of_childs(tree_of_devices);
    devices = MANY_ALLOCATIONS(number_of_devices, Device*);
    // On pointe su le premier appareil
    node_of_device = xml_get_first_child(tree_of_devices);
    // On creer, initie et reference chaque appareil
    for (number_of_initialized_devices=0; number_of_initialized_devices < number_of_devices; number_of_initialized_devices++)
    {
        device = ALLOCATION(Device);
        device_init(device, node_of_device);
        devices[number_of_initialized_devices] = device;
        // On cherche la node suivante sauf si c'est la derniere
        if (number_of_initialized_devices + 1 < number_of_devices) node_of_device = xml_get_next_sibling(node_of_device);
    }
}
*/
