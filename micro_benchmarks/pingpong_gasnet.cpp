#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <gasnet.h>
#include <chrono>

#include "mcl.hpp"
#include "result.hpp"

#define BARRIER()                                           \
do {                                                        \
  gasnet_barrier_notify(0,GASNET_BARRIERFLAG_ANONYMOUS);    \
  gasnet_barrier_wait(0,GASNET_BARRIERFLAG_ANONYMOUS);      \
} while (0)

typedef char byte_t;

const int N = 500;
const int N_warmup = 50;

int rank, nodes;
bool msg_pending = false;
bool msg_recieved = false;

byte_t *data;
int local_number;

void medium_request_handler(gasnet_token_t token, void *buf, size_t size)
{
    std::memcpy(data, buf, size);
    msg_recieved = true;
    gasnet_AMReplyShort0(token, 252);
}

void short_request_handler(gasnet_token_t token, gasnet_handlerarg_t recv_number)
{
    if( recv_number - local_number != 2 )
        throw std::runtime_error("number difference != 2");
    
    local_number = recv_number;
    msg_recieved = true;
    
    gasnet_AMReplyShort0(token, 252);
}

void reply_handler(gasnet_token_t token)
{
    msg_pending = false;
}

void send_medium(byte_t *data, std::size_t size, int dest)
{
    GASNET_BLOCKUNTIL(msg_recieved == true);
    msg_recieved = false;
    msg_pending = true;
    gasnet_AMRequestMedium0(dest, 250, data, size);
    GASNET_BLOCKUNTIL(msg_pending == false);
}

void send_short(int dest)
{    
    GASNET_BLOCKUNTIL(msg_recieved == true);
//     std::cout << (rank == 0 ? "ping" : "pong") << std::endl;
    msg_recieved = false;
    msg_pending = true;
    gasnet_AMRequestShort1(dest, 251, local_number+1);
    GASNET_BLOCKUNTIL(msg_pending == false);
}

int main(int argc, char ** argv)    
{
    std::vector<gasnet_handlerentry_t> handlers = { 
        { 250, (void(*)())medium_request_handler }, 
        { 251, (void(*)())short_request_handler },
        { 252, (void(*)())reply_handler },
    };
    
    gasnet_init(&argc, &argv);
    std::size_t segment_size = 1024*1024*128; // 128 Mbyte
    gasnet_attach(handlers.data(), handlers.size(), segment_size, 0);
    
    rank = gasnet_mynode();
    nodes = gasnet_nodes();
    
    if( nodes != 2 ) throw std::runtime_error("Must run with 2 processes!");
    int neighbour = (rank == 0 ? 1 : 0);
    
    const int message_size_min = 8;
    int message_size_max = gasnet_AMMaxMedium(); // ca. 64 Mbyte
    
    std::vector<int> medium_sizes;
    std::vector<double> medium_times;
    double short_time;
    
    if(rank == 0 ) std::cout << "PING-PONG BENCHMARK" << std::endl;
    
    // test short messages
    if(rank == 0 ) std::cout << "ping-pong on short" << std::endl;
    
    {
        // warm up
        local_number = ( rank == 0 ? 0 : -1 ); // start #1 with -1 since difference after each ping-pong should be 2
        msg_recieved = ( rank == 0 ? true : false ); // start chain with rank 0
        msg_pending = false;
        
        BARRIER();
        for(std::size_t n=0; n<N_warmup; ++n)
        {
            send_short(neighbour);
        }
        BARRIER();
        
        // benchmark
        local_number = ( rank == 0 ? 0 : -1 ); // start #1 with -1 since difference after each ping-pong should be 2
        msg_recieved = ( rank == 0 ? true : false ); // start chain with rank 0
        msg_pending = false;
        
        BARRIER();
        auto t_0 = std::chrono::high_resolution_clock::now();
            
        for(std::size_t n=0; n<N; ++n)
        {
            send_short(neighbour);
        }
            
        BARRIER();
        auto t_1 = std::chrono::high_resolution_clock::now();
        
        short_time = std::chrono::duration<double>(t_1 - t_0).count() / N;
    }
    
    // test medium messages
    data = new byte_t[message_size_max];
    
    if(rank == 0 ) std::cout << "ping-pong on medium [ " << message_size_min << ", " << message_size_max << "] " << std::endl;
    
    for(int message_size = message_size_min; message_size <= message_size_max; message_size*=2)
    {
//         if(rank == 0 ) std::cout << "message_size = " << message_size << std::endl;
        
        msg_recieved = ( rank == 0 ? true : false ); // start chain with rank 0
        msg_pending = false;
        
        BARRIER();
        auto t_0 = std::chrono::high_resolution_clock::now();
        
        for(std::size_t n=0; n<N; ++n)
        {
            send_medium(data, message_size, neighbour);
        }
        
        BARRIER();
        auto t_1 = std::chrono::high_resolution_clock::now();
        
        medium_sizes.push_back( message_size );
        medium_times.push_back( std::chrono::duration<double>(t_1 - t_0).count() / N );
    }
    
    gasnet_AMPoll();
    
    delete[] data;
    
    if( rank == 0 )
    {
        std::cout << std::endl;
        std::cout << "RESULTS:" << std::endl;
        auto latency_short = short_time / 4; // (req + rec) * 2
        std::cout << "min_time (short) = " << short_time * 1.0e6 << " us" << std::endl;
        std::cout << "latency (short) = " << latency_short * 1.0e6 << " us" << std::endl;
        
        auto min_time_it = std::min_element(medium_times.begin(), medium_times.end());
        std::cout << "min_time achieved at " << medium_sizes.at(medium_times.end() - min_time_it) << " bytes" << std::endl;
        auto latency_medium = *min_time_it / 4;
        
        std::cout << "min_time (medium) = " << *min_time_it * 1.0e6 << " us" << std::endl;
        std::cout << "latency (medium) = " << latency_medium * 1.0e6 << " us" << std::endl;
    }
    
    BARRIER();
    gasnet_exit(0);
    return 0;
}
