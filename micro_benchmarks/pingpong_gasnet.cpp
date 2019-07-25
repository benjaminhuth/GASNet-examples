#include <iostream>
#include <cstring>
#include <algorithm>
#include <gasnet.h>
#include <chrono>
#include <vector>
#include <array>

#include "mcl.hpp"
#include "result.hpp"

#include "test_short.hpp"
#include "test_medium.hpp"
#include "test_long.hpp"

// #define DO_REPLY
// #define DO_WARMUP

constexpr int iterations = 200;

int main(int argc, char ** argv)    
{
    std::vector<gasnet_handlerentry_t> handlers = { 
        { short_req_id,  (void(*)())short_request_handler }, 
        { short_rep_id,  (void(*)())short_reply_handler },
        { medium_req_id, (void(*)())medium_request_handler },
        { medium_rep_id, (void(*)())medium_reply_handler },
        { long_req_id,   (void(*)())long_request_handler },
        { long_rep_id,   (void(*)())long_reply_handler }
    };
    
    std::size_t segment_size = 1024*1024; // 1 MB
    if(argc == 2) segment_size = std::atoi(argv[1]);
    
    gasnet_init(&argc, &argv);
    gasnet_attach(handlers.data(), handlers.size(), segment_size, 0);
    
    int rank = gasnet_mynode();
    std::string filename_modifiers;
    
    if( gasnet_nodes() != 2 ) throw std::runtime_error("Must run with 2 processes!");
    
    if(rank == 0 ) 
    {
        std::cout << "PING-PONG BENCHMARK";
#ifdef DO_REPLY
        std::cout << " \\w reply";
        filename_modifiers += "_reply";
#else
        std::cout << " \\wo reply";
#endif
        
#ifdef DO_WARMUP
        std::cout << " \\w warmup";
        filename_modifiers += "_warmup";
#else
        std::cout << " \\wo warmup";
#endif
        std::cout << std::endl;
    }
    
    // test short messages
    if(rank == 0 ) std::cout << "- ping-pong on short" << std::endl;
    
    std::vector<double> data_short;
    for(std::size_t i=0; i<iterations; ++i)
        data_short.push_back(benchmark_short());

    // test medium messages
    int medium_msg_size_min = 8;
    int medium_msg_size_max = std::min(segment_size, gasnet_AMMaxMedium());
    if(rank == 0 ) std::cout << "- ping-pong on medium, sizes: [ " << medium_msg_size_min << " B, " << medium_msg_size_max/1.0e3 << " kB ]" << std::endl;
    
    std::vector<int> medium_sizes;
    std::vector<double> medium_times;
    std::vector<double> medium_times_err;
    
    for(std::size_t msg_size = medium_msg_size_min; msg_size < medium_msg_size_max; msg_size *= 2)
    {
        std::vector<double> times;
        
        for(std::size_t i=0; i<iterations; ++i)
            times.push_back( benchmark_medium(msg_size) );
        
        medium_sizes.push_back(msg_size);
        medium_times.push_back(mc::average(times));
        medium_times_err.push_back(mc::standard_deviation(times));
    }
    
    // test long messages
    int long_msg_size_min = 8;
    int long_msg_size_max = std::min(segment_size, gasnet_AMMaxLongRequest());
    if(rank == 0 ) std::cout << "- ping-pong on long, sizes: [ " << long_msg_size_min << " B, " << long_msg_size_max/1.0e6 << " MB ]" << std::endl;
    
    std::vector<int> long_sizes;
    std::vector<double> long_times;
    std::vector<double> long_times_err;
    
    for(std::size_t msg_size = long_msg_size_min; msg_size < long_msg_size_max; msg_size *= 2)
    {
        std::vector<double> times;
        
        for(std::size_t i=0; i<iterations; ++i)
            times.push_back( benchmark_long(msg_size) );
        
        long_sizes.push_back(msg_size);
        long_times.push_back(mc::average(times));
        long_times_err.push_back(mc::standard_deviation(times));
    }
    
    BARRIER();
    // compute results    
    if( rank == 0 )
    {
        std::cout << std::fixed;
        std::cout << "RESULTS:" << std::endl; 
        
        // short
        std::cout << "- short:  min_time         = ( " 
                  << mc::average(data_short)*1.0e6 << " +- " << mc::standard_deviation(data_short)*1.0e6 << " ) us    => latency" << std::endl;
        
        // medium
        auto mtimes = compute_time_data(medium_times, medium_times_err, medium_sizes);
        auto mlatency = mtimes.min_avg;
        
        std::cout << "- medium: min_time         = ( " << mtimes.min_avg*1.0e6 << " +- " << mtimes.min_err*1.0e6 << " ) us    => latency" << std::endl;
        std::cout << "- medium: size @min_time   = " << mtimes.min_size << " B" << std::endl;
        
        std::cout << "- medium: max_time         = ( " << mtimes.max_avg*1.0e6 << " +- " << mtimes.max_err*1.0e6 << " ) us" << std::endl;
        std::cout << "- medium: size @max_time   = " << mtimes.max_size << " B" << std::endl;
        
        auto mbndws = compute_bandwidth_data(mlatency, medium_times, medium_sizes);
        
        std::cout << "- medium: bandwidth range  = [ " << mbndws.min/1.0e9 << ", " << mbndws.max/1.0e9 << " ] GB/s" << std::endl;
        std::cout << "- medium: bandwidth value  = ( " << mbndws.avg/1.0e9 << " +- " << mbndws.err/1.0e9 << " ) GB/s" << std::endl;
        
        mc::clear_file("gasnet_pingpong_medium" + filename_modifiers + ".txt");
        mc::export_containers("gasnet_pingpong_medium" + filename_modifiers + ".txt", {"size", "time", "error"}, medium_sizes, medium_times, medium_times_err);
                
        // long
        auto ltimes = compute_time_data(long_times, long_times_err, long_sizes);
        auto llatency = ltimes.min_avg;
        
        std::cout << "- long:   min_time         = ( " << ltimes.min_avg*1.0e6 << " +- " << ltimes.min_err*1.0e6 << " ) us    => latency" << std::endl;
        std::cout << "- long:   size @min_time   = " << ltimes.min_size << " B" << std::endl;
        
        std::cout << "- long:   max_time         = ( " << ltimes.max_avg*1.0e6 << " +- " << ltimes.max_err*1.0e6 << " ) us" << std::endl;
        std::cout << "- long:   size @max_time   = " << ltimes.max_size << " B" << std::endl;
        
        auto lbndws = compute_bandwidth_data(llatency, long_times, long_sizes);
        
        std::cout << "- long:   bandwidth range  = [ " << lbndws.min/1.0e9 << ", " << lbndws.max/1.0e9 << " ] GB/s" << std::endl;
        std::cout << "- long:   bandwidth value  = ( " << lbndws.avg/1.0e9 << " +- " << lbndws.err/1.0e9 << " ) GB/s" << std::endl;
        
        mc::clear_file("gasnet_pingpong_long" + filename_modifiers + ".txt");
        mc::export_containers("gasnet_pingpong_long" + filename_modifiers + ".txt", {"size", "time", "error"}, long_sizes, long_times, long_times_err);
    }
    
    BARRIER();
    gasnet_exit(0);
    return 0;
}
