#ifndef RESULTS_HPP
#define RESULTS_HPP

#include <vector>
#include <algorithm>
#include <iostream>
#include "mcl.hpp"

struct performance_data_t
{
    double latency, bandwidth_min, bandwidth_max, bandwidth_avg, bandwidth_err;
};

performance_data_t compute_latency_bandwidth(std::vector<int> sizes, std::vector<double> times)
{        
    auto latency = *std::min_element(times.begin(), times.end());        
    
    // cut latency bound part
    auto latency_limit = std::find_if(times.begin(), times.end(), [&](double &el){ return el > 5*latency; });
    sizes.erase(sizes.begin(), sizes.begin() + (latency_limit - times.begin()));        
    times.erase(times.begin(), latency_limit);
    
    // compute bandwidths
    std::vector<double> bandwidths(times.size());
    std::transform(times.begin(), times.end(), sizes.begin(), bandwidths.begin(), [](double time, double size){ return size / time; });
    
    auto max_bandwidth = *std::max_element(bandwidths.begin(), bandwidths.end());
    auto min_bandwidth = *std::min_element(bandwidths.begin(), bandwidths.end());
    auto avg_bandwidth = mc::average(bandwidths);
    auto err_bandwidth = mc::standard_deviation(bandwidths);
    
    return { latency, min_bandwidth, max_bandwidth, avg_bandwidth, err_bandwidth };
}

void print_results(const performance_data_t &results)
{
    std::cout << "latency   = " << results.latency * 1.0e6 << " us" << std::endl;
    std::cout << "bandwidth = " << results.bandwidth_avg/1.0e9 << " +- " << results.bandwidth_err/1.0e9 << " Gb/s" << std::endl; 
    std::cout << "bandwidth range = [ " << results.bandwidth_min/1.0e9 << ", " << results.bandwidth_max/1.0e9 << " ] Gb/s" << std::endl;
}
    

#endif
