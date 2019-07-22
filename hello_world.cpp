#include <iostream>
#include <gasnet.h>

int main(int argc, char ** argv)
{
    gasnet_init(&argc, &argv);
    
    int rank = gasnet_mynode();
    int nodes = gasnet_nodes();
    
    if( rank == 0 ) std::cout << "Launching GASNet with " << nodes << " nodes!" << std::endl;
    
    std::cout << "Hello World from GASNet rank #" << rank << std::endl;
    
    gasnet_exit(0);
    return 0;
}
