################################################################################
# Copyright  ETIS — ENSEA, Université de Cergy-Pontoise, CNRS (1991-2014)
#promethe@ensea.fr
#
# Authors: P. Andry, J.C. Baccon, D. Bailly, A. Blanchard, S. Boucena, A. Chatty, N. Cuperlier, P. Delarboulas, P. Gaussier, 
# C. Giovannangeli, C. Grand, L. Hafemeister, C. Hasson, S.K. Hasnain, S. Hanoune, J. Hirel, A. Jauffret, C. Joulain, A. Karaouzène,  
# M. Lagarde, S. Leprêtre, M. Maillard, B. Miramond, S. Moga, G. Mostafaoui, A. Pitti, K. Prepin, M. Quoy, A. de Rengervé, A. Revel ...
#
# See more details and updates in the file AUTHORS 
#
# This software is a computer program whose purpose is to simulate neural networks and control robots or simulations.
# This software is governed by the CeCILL v2.1 license under French law and abiding by the rules of distribution of free software. 
# You can use, modify and/ or redistribute the software under the terms of the CeCILL v2.1 license as circulated by CEA, CNRS and INRIA at the following URL "http://www.cecill.info". 
# As a counterpart to the access to the source code and  rights to copy, modify and redistribute granted by the license, 
# users are provided only with a limited warranty and the software's author, the holder of the economic rights,  and the successive licensors have only limited liability. 
# In this respect, the user's attention is drawn to the risks associated with loading, using, modifying and/or developing or reproducing the software by the user in light of its specific status of free software, 
# that may mean  that it is complicated to manipulate, and that also therefore means that it is reserved for developers and experienced professionals having in-depth computer knowledge. 
# Users are therefore encouraged to load and test the software's suitability as regards their requirements in conditions enabling the security of their systems and/or data to be ensured 
# and, more generally, to use and operate it in the same conditions as regards security. 
# The fact that you are presently reading this means that you have had knowledge of the CeCILL v2.1 license and that you accept its terms.
################################################################################
default:
	$(MAKE) install --jobs=$(NUMBER_OF_CORES) --silent

all: default


include ../scripts/variables.mk
CC:=prom_colorgcc

CFLAGS:= -Wextra -W -Wall -Wno-variadic-macros -fkeep-inline-functions -Wwrite-strings -Wfloat-equal -Wunknown-pragmas -Wdeclaration-after-statement -D_REENTRANT -DDAEMON_COM -DLinux -D_GNU_SOURCE -std=c99
GTK_LIBS:= -pthread -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgthread-2.0 -lrt -lglib-2.0  
PACKAGES:= gtk+-3.0 gmodule-2.0 gthread-2.0
GTK_FLAGS:= `pkg-config --cflags $(PACKAGES)`
SOURCES:=pandora.c pandora_prompt.c pandora_receive_from_prom.c pandora_graphic.c pandora_ivy.c pandora_save.c pandora_file_save.c pandora_architecture.c
OBJECTS:=$(patsubst %.c,%.o,$(SOURCES))
INCLUDES:=-I../shared/include  -I../enet/include  -I..
LIBS:= -L../lib/$(system)/enet/lib -lenet -L../lib/$(system)/ivy -lglibivy -L../lib/$(system)/script -lscript -lmxml `pkg-config --libs $(PACKAGES)` -lm -lpcre  -Wl,-rpath,$ORIGIN/Libraries  ../lib/$(system)/blc/libblc.so -ldl


CFLAGS+="-DGTK_DISABLE_SINGLE_INCLUDES"
#CFLAGS+="-DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
CFLAGS+="-DGSEAL_ENABLE"

$(objdir)/debug:
	mkdir -p $@

$(objdir)/release:
	mkdir -p $@

$(objdir)/debug/%.o:src/%.c | $(objdir)/debug
	$(CC) -c -ggdb $(CFLAGS) $(FLAGS_DEBUG) $(GTK_FLAGS) $(INCLUDES) $< -o $@  

$(objdir)/release/%.o:src/%.c | $(objdir)/release
	$(CC) -c $(CFLAGS) $(FLAGS_OPTIM) $(GTK_FLAGS) $(INCLUDES) $< -o $@

pandora_debug: $(foreach object, $(OBJECTS), $(objdir)/debug/$(object)) 
	$(CC) $(CFLAGS) $(FLAGS_DEBUG)  $^ -o $@ $(LIBS) 

pandora:$(foreach object, $(OBJECTS), $(objdir)/release/$(object))
	$(CC) $(CFLAGS) $(FLAGS_OPTIM) $^ -o $@ $(LIBS)

$(rsrcdir)/%: resources/%
	cp $^ $@

$(bindir)/%: %
	cp $^ $@

install:$(bindir)/pandora_debug $(bindir)/pandora $(rsrcdir)/pandora_icon.png $(rsrcdir)/pandora_group_display_properties.glade


clean:
	rm -f $(objdir)/debug/*.o $(objdir)/release/*.o 

reset:clean
	rm -f $(bindir)/pandora_debug $(bindir)/pandora 
