#include <mpi.h>
#include <gasnet.h>
#include <iostream>

int main(int argc, char ** argv)
{
    gasnet_init(&argc, &argv);
    
    gasnet_attach(nullptr, 0, gasnet_getMaxGlobalSegmentSize(), 0);
    int gasnet_rank = gasnet_mynode();
    
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    
    int mpi_is_initialized;
    MPI_Initialized(&mpi_is_initialized);
    
    if(!mpi_is_initialized) 
        MPI_Init(&argc, &argv);
    
    int mpi_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    
    std::cout << "MPI_rank = " << mpi_rank << ", gasnet_rank = " << gasnet_rank << std::endl;
    
    if(!mpi_is_initialized) 
        MPI_Finalize();
    
    gasnet_exit(0);
}
        
