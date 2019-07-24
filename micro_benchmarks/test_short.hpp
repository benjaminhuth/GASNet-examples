#ifndef TEST_SHORT_HPP
#define TEST_SHORT_HPP

#include <chrono>
#include <stdexcept>
#include <iostream>
#include <atomic>

#include <gasnet.h>

#include "mcl.hpp"

#ifndef BARRIER
#define BARRIER()                                           \
do {                                                        \
  gasnet_barrier_notify(0,GASNET_BARRIERFLAG_ANONYMOUS);    \
  gasnet_barrier_wait(0,GASNET_BARRIERFLAG_ANONYMOUS);      \
} while (0)
#endif

namespace s
{
    const int N = 500;
    const int N_warmup = 0;
    
    std::atomic<int> local_number{ 0 };
    std::atomic<int> reply_number{ 0 };
    std::atomic<bool> msg_received{ false };
}

const gasnet_handler_t short_req_id = 200;
const gasnet_handler_t short_rep_id = 201;

void short_request_handler(gasnet_token_t token, gasnet_handlerarg_t recv_number)
{
    if( recv_number - s::local_number != 2 )
        throw std::runtime_error("number difference != 2");
    
    s::local_number = recv_number;
    s::msg_received = true;
    
#ifdef DO_REPLY
    gasnet_AMReplyShort0(token, short_rep_id);
#endif
}

void short_reply_handler(gasnet_token_t token)
{
    s::reply_number++;
}

void send_short(int dest)
{    
    GASNET_BLOCKUNTIL(s::msg_received == true);
    s::msg_received = false;
    gasnet_AMRequestShort1(dest, short_req_id, s::local_number+1);
} 

double benchmark_short()
{
    int neighbour = (gasnet_mynode() == 0 ? 1 : 0);
    
#ifdef DO_WARMUP 
    // warm up
    s::local_number = ( gasnet_mynode() == 0 ? 0 : -1 ); // start #1 with -1 since difference after each ping-pong should be 2
    s::msg_received = ( gasnet_mynode() == 0 ? true : false ); // start chain with rank 0
    
    BARRIER();
    for(std::size_t n=0; n<s::N_warmup; ++n)
    {
        send_short(neighbour);
    }
    BARRIER();
    
    if( gasnet_mynode() == 0 )
        GASNET_BLOCKUNTIL( s::local_number == 2*s::N_warmup );
    else
        GASNET_BLOCKUNTIL( s::local_number == 2*s::N_warmup -1 );
#endif
    
    // benchmark
    s::local_number = ( gasnet_mynode() == 0 ? 0 : -1 ); // start #1 with -1 since difference after each ping-pong should be 2
    s::msg_received = ( gasnet_mynode() == 0 ? true : false ); // start chain with rank 0
    
    BARRIER();
    auto t_0 = std::chrono::high_resolution_clock::now();
        
    for(std::size_t n=0; n<s::N; ++n)
    {
        send_short(neighbour);
    }
        
    BARRIER();
    auto t_1 = std::chrono::high_resolution_clock::now();
    
    if( gasnet_mynode() == 0 )
        GASNET_BLOCKUNTIL( s::local_number == 2*s::N );
    else
        GASNET_BLOCKUNTIL( s::local_number == 2*s::N -1 );
    
    return std::chrono::duration<double>(t_1 - t_0).count() / (2 * s::N);
}

#endif
