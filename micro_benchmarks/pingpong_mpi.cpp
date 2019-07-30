#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <string>
#include <cmath>
#include <cstring>

#include <mpi.h>
#include "mcl.hpp"
#include "result.hpp"

#define STANDARD_TAG 10
#define DO_WARMUP
// #define NON_BLOCKING

const int N = 500;
const int N_warmup = 50;
const int iterations = 200;

typedef char byte_t;

double my_time()
{
    MPI_Barrier(MPI_COMM_WORLD);
    return MPI_Wtime();
}

double benchmark_loop_block(byte_t *data, size_t size, int destination)
{
#ifdef DO_WARMUP
    for(int n=0; n<N_warmup; ++n)
    {
        if(destination % 2 == 0)
        {
            MPI_Ssend(data, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD);
            MPI_Recv(data, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else
        {
            MPI_Recv(data, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Ssend(data, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD);
            
        }
    }
#endif
        
    MPI_Barrier(MPI_COMM_WORLD);
    auto t_0 = my_time();
    for(int n=0; n<N; ++n)
    {                
        if(destination % 2 == 0)
        {
            MPI_Ssend(data, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD);
            MPI_Recv(data, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else
        {
            MPI_Recv(data, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Ssend(data, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD);
            
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    auto t_1 = my_time();
    
    return (t_1 - t_0) / (2*N);
}

double benchmark_loop_nonblock(byte_t *send_buffer, byte_t *recv_buffer, size_t size, int destination)
{
    MPI_Request req_array[2];
    MPI_Status  stat_array[2];
    
#ifdef DO_WARMUP
    for(int n=0; n<N_warmup; ++n)
    {
        MPI_Isend(send_buffer, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD, &req_array[0]);
        MPI_Irecv(recv_buffer, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD, &req_array[1]);
        MPI_Waitall(2, req_array, stat_array);
        std::memcpy(recv_buffer, send_buffer, size);
    }
#endif
    
    MPI_Barrier(MPI_COMM_WORLD);
    auto t_0 = my_time();
    for(int n=0; n<N; ++n)
    {
        MPI_Isend(send_buffer, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD, &req_array[0]);
        MPI_Irecv(recv_buffer, size, MPI_BYTE, destination, STANDARD_TAG, MPI_COMM_WORLD, &req_array[1]);
        MPI_Waitall(2, req_array, stat_array);
        std::memcpy(recv_buffer, send_buffer, size);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    auto t_1 = my_time();

    return (t_1 - t_0) / (2*N);    
}

double benchmark(int rank, size_t message_size)
{
    double time = 0.0;
    
    if( rank == 0 )
    {
        int destination = 1;
#ifdef NON_BLOCKING
        auto send_buffer = new byte_t[message_size];
        auto recv_buffer = new byte_t[message_size];
        
        time = benchmark_loop_nonblock(send_buffer, recv_buffer, message_size, destination);
        
        delete[] send_buffer;
        delete[] recv_buffer;
#else
        auto message = new byte_t[message_size];
        
        time = benchmark_loop_block(message, message_size, destination);
        
        delete[] message;
#endif
    }
    else if( rank == 1 )
    {
        int destination = 0;
#ifdef NON_BLOCKING
        auto send_buffer = new byte_t[message_size];
        auto recv_buffer = new byte_t[message_size];
        
        time = benchmark_loop_nonblock(send_buffer, recv_buffer, message_size, destination);
        
        delete[] send_buffer;
        delete[] recv_buffer;
#else
        auto message = new byte_t[message_size];
        
        time = benchmark_loop_block(message, message_size, destination);
        
        delete[] message;
#endif
    }
    else
    {
        throw std::runtime_error("must be run with 2 processes!");
    }
        
    return time;
}

int main(int argc, char ** argv)
{
    MPI_Init(nullptr, nullptr);
    MPI_Status status;
    
    const int message_size_min = 8;
    int message_size_max = 1024 * 1024; // ca. 1 MB
    
    if(argc == 2) message_size_max = std::atoi(argv[1]);
    
    int rank, size;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    std::vector<int> sizes;
    std::vector<double> times;
    std::vector<double> times_err;
    
    if(rank == 0 )
    {
        std::cout << "PING-PONG BENCHMARK";
#ifdef DO_WARMUP
        std::cout << " \\w warmup";
#else
        std::cout << " \\wo warmup";
#endif
        
#ifdef NON_BLOCKING
        std::cout << " \\non blocking";
#else
        std::cout << " \\blocking";
#endif
        std::cout <<std::endl;
        std::cout << "- ping-pong sizes: [ " << message_size_min << " B, " << message_size_max/1.0e6 << " MB ]" << std::endl;
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
        
        mc::clear_file("mpi_pingpong.txt");
        mc::export_containers("mpi_pingpong.txt", {"size", "time", "error"}, sizes, times, times_err);
    }
    
    MPI_Finalize();
}

