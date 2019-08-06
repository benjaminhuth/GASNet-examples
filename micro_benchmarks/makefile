include ../Makefile-config-platform
include ../Makefile-config-$(TARGET_PLATFORM)

include $(GASNET_INSTALL_DIR)/include/$(CONDUIT)-conduit/$(CONDUIT)-par.mak

# fix C++ bugs in GASNet file
ifdef GASNET_LD_REQUIRES_MPI
GASNET_CXX = $(MPICXX)
endif
GASNET_LD = $(GASNET_CXX)

.PHONY: gasnet mpi

all: gasnet mpi

mpi:
	$(MPICXX) $(STD) pingpong_mpi.cpp -o pingpong_mpi.out

gasnet:
	$(GASNET_CXX) $(STD) $(GASNET_CXXCPPFLAGS) $(GASNET_CXXFLAGS) pingpong_gasnet.cpp -c -o pingpong_gasnet-$(CONDUIT).o
	$(GASNET_LD) $(GASNET_LDFLAGS) pingpong_gasnet-$(CONDUIT).o $(GASNET_LIBS) -o pingpong_gasnet-$(CONDUIT).out
	rm pingpong_gasnet-$(CONDUIT).o
	
clean:
	rm -f *.out
	rm -f *.o

