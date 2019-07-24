#ifndef TEST_LONG_HPP
#define TEST_LONG_HPP

#include <vector>
#include <mutex>
#include <cstring>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <atomic>

#include <gasnet.h>

#ifndef BARRIER
#define BARRIER()                                           \
do {                                                        \
  gasnet_barrier_notify(0,GASNET_BARRIERFLAG_ANONYMOUS);    \
  gasnet_barrier_wait(0,GASNET_BARRIERFLAG_ANONYMOUS);      \
} while (0)
#endif

typedef char byte_t;

namespace l
{
    const int N = 500;
    const int N_warmup = 50;
    
    byte_t *data;
    int reply_number = 0;
    std::atomic<int> local_number{ 0 };
    std::atomic<bool> msg_recieved{ false };
}

const gasnet_handler_t long_req_id = 204;
const gasnet_handler_t long_rep_id = 205;

void long_request_handler(gasnet_token_t token, void *buf, size_t size, int recv_number)
{    
    if( recv_number - l::local_number != 2 )
        throw std::runtime_error("number difference != 2");
    
    std::memcpy(l::data, buf, size);
    l::msg_recieved = true;   
    l::local_number = recv_number;
    
#ifdef DO_REPLY
    gasnet_AMReplyShort0(token, medium_rep_id);
#endif
}

void long_reply_handler(gasnet_token_t token)
{
    l::reply_number++;
}

void send_long(byte_t *data, std::size_t size, int dest, void *dest_mem)
{
    GASNET_BLOCKUNTIL(l::msg_recieved == true);
    l::msg_recieved = false;
    gasnet_AMRequestLong1(dest, long_req_id, data, size, dest_mem, l::local_number+1);
}


double benchmark_long(int message_size)
{    
    if( message_size > gasnet_AMMaxLongRequest() )
        throw std::runtime_error("message_size for medium must not be greater than gasnet_AMMaxLongRequest()");
    
    int neighbour = (gasnet_mynode() == 0 ? 1 : 0);
    
    std::vector<gasnet_seginfo_t> seginfo_table(gasnet_nodes());
    gasnet_getSegmentInfo(seginfo_table.data(), seginfo_table.size());
    auto neighbour_dest_addr = seginfo_table[neighbour].addr;
    
    l::data = new byte_t[message_size];

#ifdef DO_WARMUP        
    // warm up
    l::local_number = ( gasnet_mynode() == 0 ? 0 : -1 ); // start #1 with -1 since difference after each ping-pong should be 2
    l::msg_recieved = ( gasnet_mynode() == 0 ? true : false ); // start chain with rank 0
    
    BARRIER();
    for(std::size_t n=0; n<l::N_warmup; ++n)
    {
        send_long(l::data, message_size, neighbour, neighbour_dest_addr);
    }
    BARRIER();
    
    if( gasnet_mynode() == 0 )
        GASNET_BLOCKUNTIL( l::local_number == 2*l::N_warmup );
    else
        GASNET_BLOCKUNTIL( l::local_number == 2*l::N_warmup -1 );
#endif
    
    // benchmark
    l::local_number = ( gasnet_mynode() == 0 ? 0 : -1 ); // start #1 with -1 since difference after each ping-pong should be 2
    l::msg_recieved = ( gasnet_mynode() == 0 ? true : false ); // start chain with rank 0
    
    BARRIER();
    auto t_0 = std::chrono::high_resolution_clock::now();
    
    for(std::size_t n=0; n<l::N; ++n)
    {
        send_long(l::data, message_size, neighbour, seginfo_table[neighbour].addr);
    }

    BARRIER();
    auto t_1 = std::chrono::high_resolution_clock::now();
    
    if( gasnet_mynode() == 0 )
        GASNET_BLOCKUNTIL( l::local_number == 2*l::N );
    else
        GASNET_BLOCKUNTIL( l::local_number == 2*l::N -1 );
    
    delete[] l::data;
    
    return std::chrono::duration<double>(t_1 - t_0).count() / (2 * l::N);
}

#endif
