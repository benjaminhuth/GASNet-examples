include ../Makefile-config-platform
include ../Makefile-config-$(TARGET_PLATFORM)

include $(GASNET_INSTALL_DIR)/include/$(CONDUIT)-conduit/$(CONDUIT)-par.mak

# fix C++ bugs in GASNet file
ifdef GASNET_LD_REQUIRES_MPI
GASNET_CXX = $(MPICXX)
endif
GASNET_LD = $(GASNET_CXX)

.PHONY: gasnet mpi

all: 	
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) test.cpp   -c -o test-$(CONDUIT).o
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) my_mpi.cpp -c -o my_mpi-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) test-$(CONDUIT).o my_mpi-$(CONDUIT).o $(GASNET_LIBS) -o test.out
	rm test-$(CONDUIT).o my_mpi-$(CONDUIT).o
	
clean:
	rm -f *.out
	rm -f *.o

