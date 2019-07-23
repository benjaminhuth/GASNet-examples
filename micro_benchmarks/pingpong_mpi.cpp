#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <string>
#include <cmath>

#include <mpi.h>

#include "mcl.hpp"
#include "result.hpp"

#define STANDARD_TAG 10

const int N = 500;
const int N_warmup = 50;

typedef char byte_t;

double my_time()
{
    MPI_Barrier(MPI_COMM_WORLD);
    return MPI_Wtime();
}

void send(byte_t *msg, size_t size, int dest, std::string name)
{
    MPI_Ssend(msg, size, MPI_BYTE, dest, STANDARD_TAG, MPI_COMM_WORLD);
}

void recv(byte_t *msg, size_t size, int src, std::string name)
{        
    MPI_Recv(msg, size, MPI_BYTE, src, STANDARD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

int main(int argc, char ** argv)
{
    MPI_Init(nullptr, nullptr);
    MPI_Status status;
    
    const int message_size_min = 8;
    int message_size_max = 1024 * 1024 * 64; // ca. 64 Mbyte
    
    if(argc == 2) message_size_max = std::atoi(argv[1]);
    
    int rank, size;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    std::vector<int> sizes;
    std::vector<double> times;
    
    for(int message_size = message_size_min; message_size <= message_size_max; message_size*=2)
    {
        if( rank == 0 ) std::cout << "measure message size " << message_size << " (" << message_size/1.0e6 << " Mb)" << std::endl;
        double t_1, t_0;
        
        if( rank == 0 )
        {
            auto message = new byte_t[message_size];
            
            for(int n=0; n<N_warmup; ++n)
            {
                send(message, message_size, 1, "ping");
                recv(message, message_size, 1, "ping");
            }
            
            t_0 = my_time();
            for(int n=0; n<N; ++n)
            {
                send(message, message_size, 1, "ping");
                recv(message, message_size, 1, "ping");
            }
            t_1 = my_time();
            
            delete[] message;
        }
        else if( rank == 1 )
        {
            auto message = new byte_t[message_size];
            
            for(int n=0; n<N_warmup; ++n)
            {
                recv(message, message_size, 0, "pong");
                send(message, message_size, 0, "pong");
            }
            
            t_0 = my_time();
            for(int n=0; n<N; ++n)
            { 
                recv(message, message_size, 0, "pong");
                send(message, message_size, 0, "pong");
            }
            t_1 = my_time();
            
            delete[] message;
        }
        else
        {
            // do nothing
        }
        
        sizes.push_back( message_size );
        times.push_back( (t_1 - t_0) / (4*N) );
    }
    
    if( rank == 0 )
    {        
        print_results(compute_latency_bandwidth(sizes, times));
    }
    
    MPI_Finalize();
}

