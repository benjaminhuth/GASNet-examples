#ifndef TEST_MEDIUM_HPP
#define TEST_MEDIUM_HPP

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

// global data for medium message
namespace m
{
    const int N = 500;
    const int N_warmup = 50;
    
    byte_t *data;
    int reply_number = 0;
    std::atomic<int> local_number{ 0 };
    std::atomic<bool> msg_recieved{ false };
}

const gasnet_handler_t medium_req_id = 202;
const gasnet_handler_t medium_rep_id = 203;

void medium_request_handler(gasnet_token_t token, void *buf, size_t size, int recv_number)
{
    if( recv_number - m::local_number != 2 )
        throw std::runtime_error("number difference != 2");
    
    std::memcpy(m::data, buf, size);
    m::msg_recieved = true;   
    m::local_number = recv_number;
    
#ifdef DO_REPLY
    gasnet_AMReplyShort0(token, medium_rep_id);
#endif
}

void medium_reply_handler(gasnet_token_t token)
{
    m::reply_number++;
}

// send next number to 
void send_medium(byte_t *data, std::size_t size, int dest)
{
    GASNET_BLOCKUNTIL(m::msg_recieved == true);
    m::msg_recieved = false;
    gasnet_AMRequestMedium1(dest, medium_req_id, data, size, m::local_number+1);
}


double benchmark_medium(int message_size)
{    
    if( message_size > gasnet_AMMaxMedium() )
        throw std::runtime_error("message_size for medium must not be greater than gasnet_AMMaxMedium()");
    
    int neighbour = (gasnet_mynode() == 0 ? 1 : 0);
    
    m::data = new byte_t[message_size];

#ifdef DO_WARMUP        
    // warm up
    m::local_number = ( gasnet_mynode() == 0 ? 0 : -1 ); // start #1 with -1 since difference after each ping-pong should be 2
    m::msg_recieved = ( gasnet_mynode() == 0 ? true : false ); // start chain with rank 0
    
    BARRIER();
    for(std::size_t n=0; n<m::N_warmup; ++n)
    {
        send_medium(m::data, message_size, neighbour);
    }
    BARRIER();
    
    if( gasnet_mynode() == 0 )
        GASNET_BLOCKUNTIL( m::local_number == 2*m::N_warmup );
    else
        GASNET_BLOCKUNTIL( m::local_number == 2*m::N_warmup -1 );
#endif
    
    // benchmark
    m::local_number = ( gasnet_mynode() == 0 ? 0 : -1 ); // start #1 with -1 since difference after each ping-pong should be 2
    m::msg_recieved = ( gasnet_mynode() == 0 ? true : false ); // start chain with rank 0
    
    BARRIER();
    auto t_0 = std::chrono::high_resolution_clock::now();
    
    for(std::size_t n=0; n<m::N; ++n)
    {
        send_medium(m::data, message_size, neighbour);
    }

    BARRIER();
    auto t_1 = std::chrono::high_resolution_clock::now();
    
    if( gasnet_mynode() == 0 )
        GASNET_BLOCKUNTIL( m::local_number == 2*m::N );
    else
        GASNET_BLOCKUNTIL( m::local_number == 2*m::N -1 );
    
    delete[] m::data;
    
    return std::chrono::duration<double>(t_1 - t_0).count() / (2 * m::N);
}

#endif
