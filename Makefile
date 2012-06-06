all: 
	gcc -g `pkg-config --cflags --libs gtk+-2.0` -o japet  japet.c japet_receive_from_prom.c  -I../enet/include -I../shared/include -L../lib/Linux/enet/lib -lenet -L../lib/Linux/ivy -lglibivy #-Wall -W 

 #-Wall -W 
