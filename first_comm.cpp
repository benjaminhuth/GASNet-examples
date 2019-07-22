#include <iostream>
#include <gasnet.h>

#define BARRIER()                                           \
do {                                                        \
  gasnet_barrier_notify(0,GASNET_BARRIERFLAG_ANONYMOUS);    \
  gasnet_barrier_wait(0,GASNET_BARRIERFLAG_ANONYMOUS);      \
} while (0)

int rank, nodes;

void request_handler(gasnet_token_t token)
{
    std::cout << "called 'request_handler' at rank #" << rank << std::endl;
    gasnet_AMReplyShort0(token, 251);
}

void reply_handler(gasnet_token_t token)
{
    std::cout << "called 'reply_handler' at rank #" << rank << std::endl;
}

int main(int argc, char ** argv)    
{
    gasnet_handlerentry_t handlers[] = { 
        { 250, (void(*)())request_handler }, 
        { 251, (void(*)())reply_handler } 
    };
    
    gasnet_init(&argc, &argv);
    gasnet_attach(handlers, 2, 16711680, 524288);
    
    rank = gasnet_mynode();
    nodes = gasnet_nodes();
    
    if( nodes != 2 ) { printf("Run with 2 nodes!\n"); exit(1); }
    
    int neighbour = (rank+1) % 2;
    
    if( rank == 1 )
    {
        std::cout << "send request from rank #" << rank << " to rank #" << neighbour << std::endl;
        gasnet_AMRequestShort0(neighbour, 250);
        sleep(1);
    }
    
    BARRIER();
    
    gasnet_exit(0);
    return 0;
}
