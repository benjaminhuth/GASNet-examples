# include either Makefile-config-archer or Makefile-config-local
include Makefile-config-archer

include $(GASNET_INSTALL_DIR)/include/$(CONDUIT)-conduit/$(CONDUIT)-par.mak

# fix C++ bugs in GASNet file
ifdef GASNET_LD_REQUIRES_MPI
GASNET_CXX = $(MPICXX)
endif
GASNET_LD = $(GASNET_CXX)

.PHONY: all hello_world first_comm stencil_1d gasnet_mpi_ranks my_mpi mpi_pingpong gasnet_pingpong clean

all: hello_world first_comm stencil_1d gasnet_mpi_ranks my_mpi mpi_pingpong gasnet_pingpong

hello_world:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) hello_world.cpp -c -o hello_world-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) hello_world-$(CONDUIT).o $(GASNET_LIBS) -o hello_world-$(CONDUIT).out
	
first_comm:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) first_comm.cpp -c -o first_comm-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) first_comm-$(CONDUIT).o  $(GASNET_LIBS) -o first_comm-$(CONDUIT).out 
	
stencil_1d:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) stencil_1d.cpp -c -o stencil_1d-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) stencil_1d-$(CONDUIT).o  $(GASNET_LIBS) -o stencil_1d-$(CONDUIT).out

gasnet_mpi_ranks:
	$(MPICXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) gasnet_mpi_ranks.cpp -c -o gasnet_mpi_ranks-$(CONDUIT).o
	$(MPICXX) $(GASNET_LDFLAGS) gasnet_mpi_ranks-$(CONDUIT).o $(GASNET_LIBS) -o gasnet_mpi_ranks-$(CONDUIT).out

preproc_defines:
	$(GASNET_CXX) $(GASNET_CXXCPPFLAGS) -E -dM -x c++ - < /dev/null

my_mpi:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) my_mpi/test.cpp   -c -o my_mpi/test-$(CONDUIT).o
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) my_mpi/my_mpi.cpp -c -o my_mpi/my_mpi-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) my_mpi/test-$(CONDUIT).o my_mpi/my_mpi-$(CONDUIT).o $(GASNET_LIBS) -o my_mpi/test.out
	
mpi_pingpong:
	$(MPICXX) $(STD) micro_benchmarks/pingpong_mpi.cpp -o micro_benchmarks/pingpong_mpi.out
	
gasnet_pingpong:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) micro_benchmarks/pingpong_gasnet.cpp -c -o micro_benchmarks/pingpong_gasnet-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) micro_benchmarks/pingpong_gasnet-$(CONDUIT).o $(GASNET_LIBS) -o micro_benchmarks/pingpong_gasnet-$(CONDUIT).out
	
clean:
	rm -f *.out
	rm -rf my_mpi/*.out
	rm -rf micro_benchmarks/*.out
	rm -f *.o
	rm -rf my_mpi/*.o
	rm -rf micro_benchmarks/*.o

