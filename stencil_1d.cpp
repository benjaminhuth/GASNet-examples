#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstring>

#include <gasnet.h>

#include <mcl/basic.hpp>

#define BARRIER BARRIER_WAIT 

#define BARRIER_WAIT(NUM) \
    do { \
        gasnet_barrier_notify(NUM,0); \
        gasnet_barrier_wait(NUM,0); \
    } while (0); \
    /*printf("Rank #%d passed barrier %d\n", rank, debug_barrier_count++);*/   

int rank, nodes;
int debug_barrier_count = 0;

const int iterations        = 10000;
const int points_per_node   = 20;
const double x_0            = -5;
const double x_1            = 5;
const auto func             = [](double x){ return x*x; };

double dx;
int num_collected = 0;
int computed = 0;
int correct_runs = 0;

std::vector<double> chain(points_per_node, 0.0);
std::vector<double> new_chain(points_per_node, 0.0);
std::vector<double> total_chain;

void req_compute_right_border(gasnet_token_t token)
{
    double el = chain.front();
    gasnet_AMReplyMedium0(token, 201, &el, sizeof(double));
}

void rep_compute_right_border(gasnet_token_t token, void *buf, size_t nbytes) 
{ 
    int i = chain.size()-1;
    double r = *(double *)buf; 
    new_chain[i] = ( r - chain[i-1] ) / (2*dx);
    computed++;
}

void req_compute_left_border(gasnet_token_t token)
{
    double el = chain.back();
    gasnet_AMReplyMedium0(token, 203, &el, sizeof(double));
}

void rep_compute_left_border(gasnet_token_t token, void *buf, size_t nbytes)
{
    double l = *(double *)buf;
    new_chain[0] = ( chain[1] - l ) / (2*dx);
    computed++;
}

void req_collect_chain(gasnet_token_t token)
{
    gasnet_AMReplyMedium1(token, 205, new_chain.data(), sizeof(double)*chain.size(), rank*points_per_node);
}

void rep_collect_chain(gasnet_token_t token, void *buf, size_t nbytes, gasnet_handlerarg_t offset)
{
    std::memcpy(reinterpret_cast<void *>(total_chain.data()+offset), buf, nbytes);
    num_collected++;
}
    
int main(int argc, char ** argv)    
{
    gasnet_handlerentry_t handlers[] = { 
        { 200, (void(*)())req_compute_right_border },
        { 201, (void(*)())rep_compute_right_border }, 
        { 202, (void(*)())req_compute_left_border },
        { 203, (void(*)())rep_compute_left_border },
        { 204, (void(*)())req_collect_chain },
        { 205, (void(*)())rep_collect_chain }
    };
    
    gasnet_init(&argc, &argv);
    gasnet_attach(handlers, sizeof(handlers)/sizeof(gasnet_handlerentry_t), 16711680, 524288);
    
    rank = gasnet_mynode();
    nodes = gasnet_nodes();
    
    // setup local chain
    dx = std::abs(x_1 - x_0) / (points_per_node * nodes);
    double x = x_0 + rank * points_per_node * dx;
    
    for(std::size_t i=0; i<points_per_node; ++i)
    {
        chain[i] = func(x);
        x += dx;
    }
    
    for(int iter=0; iter < iterations; ++iter)
    {        
        // compute inner part
        for(std::size_t j=1; j<points_per_node-1; ++j)
        {
            new_chain[j] = ( chain[j+1] - chain[j-1] ) / (2 * dx);
        }
        
        // compute right border
        if( rank < nodes-1 )
        {
            gasnet_AMRequestShort0(rank+1, 200);
        }
        else
        {
            new_chain.back() = *(new_chain.end()-2);
            computed++;
        }

        // compute left border
        if( rank > 0 )
        {
            gasnet_AMRequestShort0(rank-1, 202);
        }
        else
        {
            new_chain.front() = *(new_chain.begin()+1);
            computed++;
        }
        
        GASNET_BLOCKUNTIL(computed == 2);
        BARRIER(2*iter+1);
        
        // collect all points
        if( rank == 0 )
        {
            total_chain.resize(points_per_node*nodes);
            
            for(std::size_t i=0; i<nodes; ++i)
            {
                gasnet_AMRequestShort0(i, 204);
            }

            GASNET_BLOCKUNTIL(num_collected == nodes);
            
            std::vector<double> x_vals;
            double x=x_0;
            double sum = std::accumulate(total_chain.begin(), total_chain.end(), 0.0);
            
            std::cout << "iteration #" << iter << ", sum = " << sum << ", correct_runs = " << correct_runs << std::endl;
            
            if( (int)sum == -10 ) 
                ++correct_runs;
        }
        
        BARRIER(2*iter);
        
        computed = 0;
        num_collected = 0;
        std::fill(new_chain.begin(), new_chain.end(), 0.0);
    }
        
    if( rank == 0 ) std::cout << (double)correct_runs / iterations * 100.0 << "% of runs correct, incorrect: " << iterations - correct_runs << std::endl;
    
    BARRIER(iterations*3);
    
    gasnet_exit(0);
    return 0;
}
