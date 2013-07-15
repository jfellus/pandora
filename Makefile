default:
	$(MAKE) install --jobs=$(NUMBER_OF_CORES)

include ../scripts/variables.mk
#As we use silent we do not need promcolor_gcc
CC:=prom_colorgcc

PACKAGES:= gtk+-2.0 gmodule-2.0 gthread-2.0
GTK_FLAGS:= `pkg-config --cflags $(PACKAGES)`
SOURCES:=pandora.c pandora_receive_from_prom.c pandora_graphic.c pandora_ivy.c pandora_save.c
OBJECTS:=$(patsubst %.c,%.o,$(SOURCES))
INCLUDES:=-I../shared/include  -I../enet/include  -I..
LIBS:= -L../lib/$(system)/enet/lib -lenet -L../lib/$(system)/ivy -lglibivy -L../lib/$(system)/script -lscript -L../lib/$(system)/graphique -lgraphique -lmxml `pkg-config --libs $(PACKAGES)`

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
