/*
 * implements MPI simalar api with gasnet
 */

#include "my_mpi.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>

#include <gasnet.h>

#define BARRIER() \
    do { \
        gasnet_barrier_notify(0,GASNET_BARRIERFLAG_ANONYMOUS); \
        gasnet_barrier_wait(0,GASNET_BARRIERFLAG_ANONYMOUS); \
    } while (0); \
    
struct message_t
{
    message_t(int _id, std::size_t _size, const void *buf) : id(_id), size(_size) 
    { 
        data = new char[size]; 
        std::memcpy(data, buf, size);
    }
    ~message_t() { /*delete[] data;*/ }
    int id;
    char *data;
    std::size_t size;
};

std::vector<message_t> g_recv_messages;
int g_pending_messages{ 0 };

void req_message_transfer(gasnet_token_t token, void *buf, size_t size, int id)
{
    g_recv_messages.push_back( message_t(id, size, buf) );
    gasnet_AMReplyShort0(token, 201);
}

void rep_message_transfer(gasnet_token_t token) 
{
    g_pending_messages--;
}

my_mpi::my_mpi()
{    
    std::vector<gasnet_handlerentry_t> handlers = {
        { 200, (void(*)())req_message_transfer },
        { 201, (void(*)())rep_message_transfer },
    };
    
    gasnet_init(nullptr, nullptr);
    gasnet_attach(handlers.data(), handlers.size(), 16711680, 524288);
}

void my_mpi::send_gasnet_request(int dest_node, int id, char* data, std::size_t size)
{
    if( data == nullptr ) std::cout << "nullptr error" << std::endl;
    gasnet_AMRequestMedium1(dest_node, 200, data, size, id);
    g_pending_messages++;
}

std::pair<char *, std::size_t> my_mpi::wait_for_message_arrival(int id)
{
    auto found = g_recv_messages.begin();
    
    do
    {
#ifdef USE_AMPOLL
        gasnet_AMPoll();
#endif
        found = std::find_if(g_recv_messages.begin(), g_recv_messages.end(), [&](auto &msg){ return msg.id == id; });
    }
    while( found == g_recv_messages.end() );
    auto ret_val = std::make_pair(found->data, found->size);
    
    g_recv_messages.erase(found);
    
    return ret_val;
}


my_mpi::~my_mpi()
{
    GASNET_BLOCKUNTIL(g_pending_messages == 0);
    BARRIER();    
    gasnet_exit(0);
}

int my_mpi::rank()
{
    return gasnet_mynode();
}

int my_mpi::world_size()
{
    return gasnet_nodes();
}

std::string my_mpi::hostename()
{
    return gasneti_gethostname();
}

void my_mpi::barrier()
{
    BARRIER();
}



