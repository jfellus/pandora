default:
	$(MAKE) install --jobs=$(NUMBER_OF_CORES) --silent

all: default


include ../scripts/variables.mk
CC:=colorgcc

CFLAGS:= -Wextra -W -Wall -Wno-variadic-macros -fkeep-inline-functions -Wwrite-strings -Wfloat-equal -Wunknown-pragmas -Wdeclaration-after-statement -D_REENTRANT -DDAEMON_COM -DLinux -D_GNU_SOURCE -std=c99
GTK_LIBS:= -pthread -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgthread-2.0 -lrt -lglib-2.0  
PACKAGES:= gtk+-3.0 gmodule-2.0 gthread-2.0
GTK_FLAGS:= `pkg-config --cflags $(PACKAGES)`
SOURCES:=pandora.c pandora_prompt.c pandora_receive_from_prom.c pandora_graphic.c pandora_ivy.c pandora_save.c pandora_file_save.c pandora_architecture.c
OBJECTS:=$(patsubst %.c,%.o,$(SOURCES))
INCLUDES:=-I../shared/include  -I../enet/include  -I..
LIBS:= -L../lib/$(system)/enet/lib -lenet -L../lib/$(system)/ivy -lglibivy -L../lib/$(system)/script -lscript -lmxml `pkg-config --libs $(PACKAGES)`

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
