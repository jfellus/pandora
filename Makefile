default:
	$(MAKE) install --jobs=$(NUMBER_OF_CORES)
-include ../scripts/variables.mk

#As we use silent we do not need promcolor_gcc
CC:=colorgcc


GTK_FLAGS:= `pkg-config --cflags gtk+-2.0`
SOURCES:=japet.c japet_receive_from_prom.c
OBJECTS:=$(patsubst %.c,%.o,$(SOURCES))
INCLUDES:=-I../shared/include  -I../enet/include  -I..
LIBS:= -L../lib/Linux/enet/lib -lenet -L../lib/Linux/ivy -lglibivy -L../lib/Linux/script -lscript -lmxml `pkg-config --libs gtk+-2.0`

$(objdir)/debug:
	mkdir -p $@

$(objdir)/release:
	mkdir -p $@

	
$(objdir)/debug/%.o:src/%.c | $(objdir)/debug
	$(CC) -c $(CFLAGS) $(FLAGS_DEBUG) $(GTK_FLAGS) $(INCLUDES) $< -o $@ 

$(objdir)/release/%.o:src/%.c | $(objdir)/release
	$(CC) -c $(CFLAGS) $(FLAGS_OPTIM) $(GTK_FLAGS) $(INCLUDES) $< -o $@

japet_debug: $(foreach object, $(OBJECTS), $(objdir)/debug/$(object))
	$(CC) $(CFLAGS) $(FLAGS_DEBUG)  $^ -o $@ $(LIBS)

japet:$(foreach object, $(OBJECTS), $(objdir)/release/$(object))
	$(CC) $(CFLAGS) $(FLAGS_OPTIM) $^ -o $@ $(LIBS)

install:japet_debug japet
	mv $^ $(bindir)/.
	
clean:
	rm -f $(objdir)/debug/*.o $(objdir)/release/*.o 

reset:clean
	rm -f $(bindir)/japet_debug $(bindir)/japet 
