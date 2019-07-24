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
const int iterations = 200;

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

double benchmark(int rank, size_t message_size)
{   
    double t_0, t_1;
    
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
        
    return (t_1 - t_0) / (4*N);
}

int main(int argc, char ** argv)
{
    MPI_Init(nullptr, nullptr);
    MPI_Status status;
    
    const int message_size_min = 8;
    int message_size_max = 1024 * 64; // ca. 64 kB
    
    if(argc == 2) message_size_max = std::atoi(argv[1]);
    
    int rank, size;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    std::vector<int> sizes;
    std::vector<double> times;
    std::vector<double> times_err;
    
    if(rank == 0 )
    {
        std::cout << "PING-PONG BENCHMARK" << std::endl;
        std::cout << "- ping-pong sizes: [ " << message_size_min << " B, " << message_size_max/1.0e3 << " kB ]" << std::endl;
    }
    
    for(size_t message_size = message_size_min; message_size < message_size_max; message_size *= 2)
    {
        std::vector<double> t;
        
        for(int i=0; i<iterations; ++i)
            t.push_back( benchmark(rank, message_size) );
        
        sizes.push_back(message_size);
        times.push_back(mc::average(t));
        times_err.push_back(mc::standard_deviation(t));
    }
    
    if( rank == 0 )
    {
        std::cout << std::fixed;
        std::cout << "RESULTS:" << std::endl; 
        
        auto timedat = compute_time_data(times, times_err, sizes);
        auto latency = timedat.min_avg;
        
        std::cout << "- min_time         = ( " << timedat.min_avg*1.0e6 << " +- " << timedat.min_err*1.0e6 << " ) us    => latency" << std::endl;
        std::cout << "- size @min_time   = " << timedat.min_size << " B" << std::endl;
        
        std::cout << "- max_time         = ( " << timedat.max_avg*1.0e6 << " +- " << timedat.max_err*1.0e6 << " ) us" << std::endl;
        std::cout << "- size @max_time   = " << timedat.max_size << " B" << std::endl;
        
        auto mbndws = compute_bandwidth_data(latency, times, sizes);
        
        std::cout << "- bandwidth range  = [ " << mbndws.min/1.0e9 << ", " << mbndws.max/1.0e9 << " ] GB/s" << std::endl;
        std::cout << "- bandwidth value  = ( " << mbndws.avg/1.0e9 << " +- " << mbndws.err/1.0e9 << " ) GB/s" << std::endl;
        
        export_3_vectors({"size", "time", "error"}, sizes, times, times_err, "mpi_pingpong.txt");
    }
    
    MPI_Finalize();
}

