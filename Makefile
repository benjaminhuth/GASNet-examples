# include either Makefile-config-archer or Makefile-config-local
include ...
include $(GASNET_INSTALL_DIR)/include/$(CONDUIT)-conduit/$(CONDUIT)-par.mak

# fix C++ bugs in GASNet file
ifdef GASNET_LD_REQUIRES_MPI
GASNET_CXX = $(MPICXX)
endif
GASNET_LD = $(GASNET_CXX)

.PHONY: all hello_world first_comm stencil_1d gasnet_mpi_ranks preproc_defines my_mpi clean

all: hello_world first_comm stencil_1d gasnet_mpi_ranks my_mpi

hello_world:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) hello_world.cpp -c -o hello_world.o
	$(GASNET_LD) $(GASNET_LDFLAGS) hello_world.o $(GASNET_LIBS) -o hello_world.out
	
first_comm:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) first_comm.cpp -c -o first_comm.o
	$(GASNET_LD) $(GASNET_LDFLAGS) first_comm.o  $(GASNET_LIBS) -o first_comm.out 
	
stencil_1d:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) stencil_1d.cpp -c -o stencil_1d.o
	$(GASNET_LD) $(GASNET_LDFLAGS) stencil_1d.o  $(GASNET_LIBS) -o stencil_1d.out

gasnet_mpi_ranks:
	$(MPICXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) gasnet_mpi_ranks.cpp -c -o gasnet_mpi_ranks.o
	$(GASNET_LD) $(GASNET_LDFLAGS) gasnet_mpi_ranks.o $(GASNET_LIBS) -o gasnet_mpi_ranks.out

preproc_defines:
	$(GASNET_CXX) $(GASNET_CXXCPPFLAGS) -E -dM -x c++ - < /dev/null

my_mpi:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) my_mpi/test.cpp   -c -o my_mpi/test.o
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) my_mpi/my_mpi.cpp -c -o my_mpi/my_mpi.o
	$(GASNET_LD) $(GASNET_LDFLAGS) my_mpi/test.o my_mpi/my_mpi.o	 $(GASNET_LIBS) -o my_mpi/test.out
	
clean:
	rm -f *.out
	rm -rf my_mpi/*.out
	rm -f *.o
	rm -rf my_mpi/*.o

