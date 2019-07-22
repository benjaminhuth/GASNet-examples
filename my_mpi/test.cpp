#include <iostream>
#include <numeric>
#include "my_mpi.hpp"

int main(int argc, char **argv) 
{
    my_mpi mpi;

    int array_size = 3;
    if(argc == 2) array_size = std::atoi(argv[1]); 
    
    auto rank = mpi.rank();
    
    std::cout << "Rank #" << rank << " runs on host " << mpi.hostename() << std::endl;
    
    mpi.barrier();
    
    int right_rank = (rank + 1) % mpi.world_size();
    int left_rank = (rank - 1 + mpi.world_size()) % mpi.world_size();
    
    std::vector<double> a(array_size);
    std::iota(a.begin(), a.end(), array_size*rank);
    
    mpi.send_data(right_rank, 10, a);
    auto b = mpi.recv_data<double>(10);
    
    std::cout << "Rank #" << rank << " recieved data from rank #" << left_rank << std::endl;

    if( array_size < 20 )
    {	
	std::cout << "data = [ "; 
    	for(auto el : b) { std::cout  << el << " "; } std::cout << "]" << std::endl;
    }
}
