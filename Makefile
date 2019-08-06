# include either Makefile-config-archer or Makefile-config-local
include Makefile-config-platform
include Makefile-config-$(TARGET_PLATFORM)

include $(GASNET_INSTALL_DIR)/include/$(CONDUIT)-conduit/$(CONDUIT)-par.mak

# fix C++ bugs in GASNet file
ifdef GASNET_LD_REQUIRES_MPI
GASNET_CXX = $(MPICXX)
endif
GASNET_LD = $(GASNET_CXX)

.PHONY: all small_tests preproc_defines pingpong my_mpi clean stencil

all: small_tests my_mpi pingpong stencil

small_tests:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) hello_world.cpp -c -o hello_world-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) hello_world-$(CONDUIT).o $(GASNET_LIBS) -o hello_world-$(CONDUIT).out
	rm hello_world-$(CONDUIT).o
	
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) first_comm.cpp -c -o first_comm-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) first_comm-$(CONDUIT).o  $(GASNET_LIBS) -o first_comm-$(CONDUIT).out 
	rm first_comm-$(CONDUIT).o
	
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) stencil_1d.cpp -c -o stencil_1d-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) stencil_1d-$(CONDUIT).o  $(GASNET_LIBS) -o stencil_1d-$(CONDUIT).out
	rm stencil_1d-$(CONDUIT).o

	$(MPICXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) gasnet_mpi_ranks.cpp -c -o gasnet_mpi_ranks-$(CONDUIT).o
	$(MPICXX) $(GASNET_LDFLAGS) gasnet_mpi_ranks-$(CONDUIT).o $(GASNET_LIBS) -o gasnet_mpi_ranks-$(CONDUIT).out
	rm gasnet_mpi_ranks-$(CONDUIT).o

preproc_defines:
	$(GASNET_CXX) $(GASNET_CXXCPPFLAGS) -E -dM -x c++ - < /dev/null

my_mpi:
	cd my_mpi && $(MAKE)
	
pingpong:
	cd micro_benchmarks && $(MAKE)
	
stencil:
	cd stencil/gasnet && $(MAKE) stencil
	cd stencil/mpi    && $(MAKE) stencil
	
clean:
	rm -f *.out
	rm -f *.o
	cd my_mpi 	    && $(MAKE) clean
	cd micro_benchmarks && $(MAKE) clean
	cd stencil/gasnet   && $(MAKE) clean
	cd stencil/mpi      && $(MAKE) clean

