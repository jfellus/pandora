all: 
	gcc -g `pkg-config --cflags --libs gtk+-2.0` -o japet  japet.c japet_receive_from_prom.c  -I../shared/include -lenet -L../lib/Linux/ivy -lglibivy -L../lib/Linux/script -lscript -lmxml -Wall -W 

 #-Wall -W 
