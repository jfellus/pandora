#include "graphic.h"
#include "japet.h"

void dessinGroupe(cairo_t *cr, int a, int b, int c, int d, group *g, int y_offset,  int z, int zMax)
{
	int i;
	int x = g->x, y = g->y;

	if (g == selectedGroup) cairo_set_source_rgb(cr, RED);
	else color(cr, *g);

	cairo_rectangle(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * (y + y_offset) + d * z - HAUTEUR_GROUPE / 2, LARGEUR_GROUPE, HAUTEUR_GROUPE);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, WHITE);
	cairo_set_font_size(cr, 8);
	cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * (y + y_offset) + d * z);
	cairo_show_text(cr, g->name);
	cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * (y + y_offset) + d * z + HAUTEUR_GROUPE / 2);

	cairo_show_text(cr, g->function);


	if (graphic.draw_links)
	{
		//Dessin des liaisons aboutissant à ce groupe
		for (i = 0; i < g->nbLinksTo; i++)
			if (g->previous[i]->myScript->displayed == TRUE)
			{
				int x1 = g->previous[i]->x, y1 = g->previous[i]->y, z1 = g->previous[i]->myScript->z; //Coordonnées du groupe situé avant la liaison

				if (g->previous[i]->myScript == g->myScript) color(cr, *g);

				cairo_set_line_width(cr, 0.8); //Trait épais représentant une liaison entre groupes

				cairo_move_to(cr, a * x1 + c * (zMax - z1) + LARGEUR_GROUPE / 2, b * (y1 + y_offset) + d * z1); //Début de la liaison (à droite du groupe prédécesseur)
				cairo_line_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * (y + y_offset) + d * z); //Fin de la liaison (à gauche du groupe courant)

				cairo_stroke(cr);
			}
	}

	if (graphic.draw_net_links)
	{
		for (i = 0; i < nb_net_link; i++)
		{
			if (net_link[i].previous == g)
			{
				cairo_set_source_rgb(cr, GREY);
				cairo_set_line_width(cr, 3);
				cairo_move_to(cr, a * x + c * (zMax - z) + LARGEUR_GROUPE / 2, b * (y + y_offset) + d * z - 10);
				cairo_line_to(cr, a * x + c * (zMax - z) + LARGEUR_GROUPE / 2 + 10, b * (y + y_offset) + d * z - 10);

				if (net_link[i].type == NET_LINK_ACK || net_link[i].type == NET_LINK_BLOCK_ACK)
				{
					cairo_move_to(cr, a * x + c * (zMax - z) + LARGEUR_GROUPE / 2, b * (y + y_offset) + d * z);
					cairo_set_source_rgb(cr, INDIGO);
					cairo_show_text(cr, "ack");
				}
				cairo_stroke(cr);
			}
			else if (net_link[i].next == g)
			{
				cairo_set_source_rgb(cr, GREY);
				cairo_set_line_width(cr, 3);
				cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2 - 10, b * (y + y_offset) + d * z - 10);
				cairo_line_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2, b * (y + y_offset) + d * z - 10);

				if (net_link[i].type == NET_LINK_BLOCK || net_link[i].type == NET_LINK_BLOCK_ACK)
				{
					cairo_move_to(cr, a * x + c * (zMax - z) - LARGEUR_GROUPE / 2 - 25, b * (y + y_offset) + d * z);
					cairo_set_source_rgb(cr, RED);
					cairo_show_text(cr, "block");
				}
				cairo_stroke(cr);
			}
		}
	}

}
