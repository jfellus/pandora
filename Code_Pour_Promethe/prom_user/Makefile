
default: | /tmp/bricerra/logs 
	$(MAKE) all --jobs=8 > /tmp/bricerra/logs/prom_user_make.log

include ../scripts/variables.mk

all:gui_debug blind_debug gui_release blind_release 
	rm -f /home/bricerra/bin_leto_prom/simulator
	ln -sf /home/bricerra/simulateur /home/bricerra/bin_leto_prom/simulator

gui_debug:SigProc_gui_debug IHM_gui_debug IO_Robot_gui_debug Parallel_Comm_gui_debug NN_IO_gui_debug Sensors_gui_debug promethe_gui_debug

SigProc_gui_debug:
	$(MAKE) --file=src/SigProc/Makefile.libSigProc_gui_debug

IHM_gui_debug:
	$(MAKE) --file=src/IHM/Makefile.libIHM_gui_debug

IO_Robot_gui_debug:
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_gui_debug

Parallel_Comm_gui_debug:
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_gui_debug

NN_IO_gui_debug:
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_gui_debug

Sensors_gui_debug:
	$(MAKE) --file=src/Sensors/Makefile.libSensors_gui_debug

promethe_gui_debug:
	$(MAKE) --file=Makefile.promethe_gui_debug

blind_debug:SigProc_blind_debug IHM_blind_debug IO_Robot_blind_debug Parallel_Comm_blind_debug NN_IO_blind_debug Sensors_blind_debug promethe_blind_debug

SigProc_blind_debug:
	$(MAKE) --file=src/SigProc/Makefile.libSigProc_blind_debug

IHM_blind_debug:
	$(MAKE) --file=src/IHM/Makefile.libIHM_blind_debug

IO_Robot_blind_debug:
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_blind_debug

Parallel_Comm_blind_debug:
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_blind_debug

NN_IO_blind_debug:
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_blind_debug

Sensors_blind_debug:
	$(MAKE) --file=src/Sensors/Makefile.libSensors_blind_debug

promethe_blind_debug:
	$(MAKE) --file=Makefile.promethe_blind_debug

gui_release:SigProc_gui_release IHM_gui_release IO_Robot_gui_release Parallel_Comm_gui_release NN_IO_gui_release Sensors_gui_release promethe_gui_release

SigProc_gui_release:
	$(MAKE) --file=src/SigProc/Makefile.libSigProc_gui_release

IHM_gui_release:
	$(MAKE) --file=src/IHM/Makefile.libIHM_gui_release

IO_Robot_gui_release:
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_gui_release

Parallel_Comm_gui_release:
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_gui_release

NN_IO_gui_release:
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_gui_release

Sensors_gui_release:
	$(MAKE) --file=src/Sensors/Makefile.libSensors_gui_release

promethe_gui_release:
	$(MAKE) --file=Makefile.promethe_gui_release

blind_release:SigProc_blind_release IHM_blind_release IO_Robot_blind_release Parallel_Comm_blind_release NN_IO_blind_release Sensors_blind_release promethe_blind_release

SigProc_blind_release:
	$(MAKE) --file=src/SigProc/Makefile.libSigProc_blind_release

IHM_blind_release:
	$(MAKE) --file=src/IHM/Makefile.libIHM_blind_release

IO_Robot_blind_release:
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_blind_release

Parallel_Comm_blind_release:
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_blind_release

NN_IO_blind_release:
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_blind_release

Sensors_blind_release:
	$(MAKE) --file=src/Sensors/Makefile.libSensors_blind_release

promethe_blind_release:
	$(MAKE) --file=Makefile.promethe_blind_release

clean:
	$(MAKE) --file=src/SigProc/Makefile.libSigProc_gui_debug clean
	$(MAKE) --file=src/IHM/Makefile.libIHM_gui_debug clean
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_gui_debug clean
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_gui_debug clean
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_gui_debug clean
	$(MAKE) --file=src/Sensors/Makefile.libSensors_gui_debug clean
	$(MAKE) --file=Makefile.promethe_gui_debug clean

	$(MAKE) --file=src/SigProc/Makefile.libSigProc_blind_debug clean
	$(MAKE) --file=src/IHM/Makefile.libIHM_blind_debug clean
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_blind_debug clean
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_blind_debug clean
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_blind_debug clean
	$(MAKE) --file=src/Sensors/Makefile.libSensors_blind_debug clean
	$(MAKE) --file=Makefile.promethe_blind_debug clean

	$(MAKE) --file=src/SigProc/Makefile.libSigProc_gui_release clean
	$(MAKE) --file=src/IHM/Makefile.libIHM_gui_release clean
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_gui_release clean
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_gui_release clean
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_gui_release clean
	$(MAKE) --file=src/Sensors/Makefile.libSensors_gui_release clean
	$(MAKE) --file=Makefile.promethe_gui_release clean

	$(MAKE) --file=src/SigProc/Makefile.libSigProc_blind_release clean
	$(MAKE) --file=src/IHM/Makefile.libIHM_blind_release clean
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_blind_release clean
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_blind_release clean
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_blind_release clean
	$(MAKE) --file=src/Sensors/Makefile.libSensors_blind_release clean
	$(MAKE) --file=Makefile.promethe_blind_release clean

reset:
	$(MAKE) --file=src/SigProc/Makefile.libSigProc_gui_debug reset
	$(MAKE) --file=src/IHM/Makefile.libIHM_gui_debug reset
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_gui_debug reset
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_gui_debug reset
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_gui_debug reset
	$(MAKE) --file=src/Sensors/Makefile.libSensors_gui_debug reset
	$(MAKE) --file=Makefile.promethe_gui_debug reset

	$(MAKE) --file=src/SigProc/Makefile.libSigProc_blind_debug reset
	$(MAKE) --file=src/IHM/Makefile.libIHM_blind_debug reset
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_blind_debug reset
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_blind_debug reset
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_blind_debug reset
	$(MAKE) --file=src/Sensors/Makefile.libSensors_blind_debug reset
	$(MAKE) --file=Makefile.promethe_blind_debug reset

	$(MAKE) --file=src/SigProc/Makefile.libSigProc_gui_release reset
	$(MAKE) --file=src/IHM/Makefile.libIHM_gui_release reset
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_gui_release reset
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_gui_release reset
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_gui_release reset
	$(MAKE) --file=src/Sensors/Makefile.libSensors_gui_release reset
	$(MAKE) --file=Makefile.promethe_gui_release reset

	$(MAKE) --file=src/SigProc/Makefile.libSigProc_blind_release reset
	$(MAKE) --file=src/IHM/Makefile.libIHM_blind_release reset
	$(MAKE) --file=src/IO_Robot/Makefile.libIO_Robot_blind_release reset
	$(MAKE) --file=src/Parallel_Comm/Makefile.libParallel_Comm_blind_release reset
	$(MAKE) --file=src/NN_IO/Makefile.libNN_IO_blind_release reset
	$(MAKE) --file=src/Sensors/Makefile.libSensors_blind_release reset
	$(MAKE) --file=Makefile.promethe_blind_release reset

