#ifndef RESULTS_HPP
#define RESULTS_HPP

#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "mcl.hpp"

struct bandwidth_data_t
{
    double min, max, avg, err;
};

struct time_data_t
{
    double min_avg, min_err; int min_size;
    double max_avg, max_err; int max_size;
};

time_data_t compute_time_data(const std::vector<double> &times, const std::vector<double> &times_err, const std::vector<int> &sizes)
{        
    auto min_time_it   = std::min_element(times.begin(), times.end());
    auto min_time_size = sizes.at( min_time_it - times.begin() );
    auto min_time_err  = times_err.at( min_time_it - times.begin() );
    
    auto max_time_it   = std::max_element(times.begin(), times.end());
    auto max_time_size = sizes.at( max_time_it - times.begin() );
    auto max_time_err  = times_err.at( max_time_it - times.begin() );
    
    return { *min_time_it, min_time_err, min_time_size, *max_time_it, max_time_err, max_time_size };
}
    

bandwidth_data_t compute_bandwidth_data(double latency, const std::vector<double> &times, const std::vector<int> &sizes)
{        
    std::vector<double> bandwidths(times.size());
    std::transform(times.begin(), times.end(), sizes.begin(), bandwidths.begin(),
                       [&](const double &time, const int &size){ return (time - latency > latency ? static_cast<double>(size)/time : 0.0); });
        
    bandwidths.erase( std::remove(bandwidths.begin(), bandwidths.end(), 0.0), bandwidths.end() );
        
    auto bndw_min = *std::min_element(bandwidths.begin(), bandwidths.end());
    auto bndw_max = *std::max_element(bandwidths.begin(), bandwidths.end());
    auto bndw_avg = mc::average(bandwidths);
    auto bndw_err = mc::standard_deviation(bandwidths);
    
    return { bndw_min, bndw_max, bndw_avg, bndw_err };
}
    

#endif
