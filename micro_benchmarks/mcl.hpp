#ifndef BASIC_HPP
#define BASIC_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <functional>
#include <string>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <sstream>
#include <exception>
    

namespace mc
{
    
    // Average
    template<class iterator_t, class number_t = typename std::iterator_traits<iterator_t>::value_type>
    inline number_t average(const iterator_t begin, const iterator_t end)
    {
        static_assert( std::is_arithmetic<number_t>::value, "Container must store arithmetic type");
        
        auto sum = std::accumulate(begin, end, static_cast<number_t>(0));
        return sum / (end - begin);
    }
    
    template<class container_t, class number_t = typename container_t::value_type>
    inline number_t average(const container_t &data)
    {
        return average(data.begin(), data.end());
    }
    
    // Standard deviation
    
    template<class iterator_t, class number_t = typename std::iterator_traits<iterator_t>::value_type>
    inline number_t standard_deviation(const iterator_t begin, const iterator_t end)
    {
        static_assert( std::is_arithmetic<number_t>::value, "Container must store arithmetic type");
        
        auto data_average = average(begin, end);
        number_t sum = 0;
        
        for(auto it = begin; it != end; ++it) 
            sum += std::pow(*it - data_average, 2);
        
        return std::sqrt( sum / (end - begin));
    }
    
    template<class container_t, class number_t = typename container_t::value_type>
    inline number_t standard_deviation(const container_t &data)
    {
        return standard_deviation(data.begin(), data.end());
    }
    
    // Vector operations
    
    template<class iterator_t, 
             class container_t = std::vector<typename std::iterator_traits<iterator_t>::value_type>, 
             class number_t = typename std::iterator_traits<iterator_t>::value_type>
    inline container_t square_container(const iterator_t begin, const iterator_t end)
    {
        static_assert( std::is_arithmetic<number_t>::value, "Container must store arithmetic type");
        container_t squared_container(end - begin);
        
        std::transform(begin, end, squared_container.begin(), [](const number_t &el){ return el*el; });
        
        return squared_container;
    }
    
    template<class container_t , class number_t = typename container_t::value_type>
    inline container_t square_container(const container_t &data)
    {
        return square_container<typename container_t::const_iterator, container_t>(data.begin(), data.end());
    }
    
    template<class iterator_t, 
             class container_t = std::vector<typename std::iterator_traits<iterator_t>::value_type>, 
             class number_t = typename std::iterator_traits<iterator_t>::value_type>
    inline container_t abs_container(const iterator_t begin, const iterator_t end)
    {
        static_assert( std::is_arithmetic<number_t>::value, "Container must store arithmetic type");
        container_t retcont(end - begin);
        
        std::transform(begin, end, retcont.begin(), [](const number_t &el){ return std::abs(el); });
        
        return retcont;
    }

    template<class container_t , class number_t = typename container_t::value_type>
    inline container_t abs_container(const container_t &data)
    {
        return abs_container<typename container_t::const_iterator, container_t>(data.begin(), data.end());
    }

    template<class container_t>
    inline std::string stringify_container(const container_t &c)
    {
        std::stringstream stream;
        
        stream << "[ ";
        for(auto it = c.begin(); it != c.end(); it++)
        {
            stream << *it << " ";
        }
        stream << "]";
        
        return stream.str();
    }

    template<class container_t>
    inline void print_container(const container_t &c, std::string name = "")
    {
        std::string name_str = name.empty() ? "" : name + " = ";
        std::cout << name_str << stringify_container(c) << std::endl;
    }
    
    inline void export_containers(std::string filename, std::vector<std::string> headers) 
    {
        if( headers.size() != 0 )
            throw std::runtime_error("number of headers must match number of containers");
    }
        
    template<class container_t, class ... containers_t>
    inline void export_containers(std::string filename, std::vector<std::string> headers, 
                                  const container_t &container, const containers_t& ... remaining)
    {
        std::ifstream old_file(filename, std::ios::in);
        std::stringstream file_buffer;
        
        // file is empty
        if(old_file.peek() == std::ifstream::traits_type::eof() )
        {
            // write first header and erase afterwards
            file_buffer << headers.front() << '\n';
            headers.erase(headers.begin());
            
            // write data of first container
            for(auto &el : container)
            {
                file_buffer << el << '\n';
            }
        }
        else
        {
            bool has_header = false;
            auto it = container.begin();
            
            for(char next = old_file.get(); !old_file.eof(); old_file.get(next))
            {
                if( next == '\n' )
                {
                    if( !has_header )
                    {
                        if( !headers.empty() )
                            file_buffer << '\t' << headers.front();
                        else
                            throw std::runtime_error("number of headers must match number of containers");
                        
                        headers.erase(headers.begin());
                        has_header = true;
                    }
                    else
                    {
                        if( it != container.end() )
                            file_buffer << '\t' << *it++;
                        else
                            throw std::runtime_error("vector sizes do not match!");
                    }
                }
                file_buffer << next;
            }
        }
        
        old_file.close();
        
        std::ofstream new_file(filename, std::ios::out  | std::ios::trunc);
        new_file << file_buffer.str();
        new_file.close();
        
        export_containers(filename, headers, remaining...);
    }
    
    inline void clear_file(std::string filename)
    {
        std::ofstream file(filename, std::ios::out | std::ios::trunc);
    }
        
    
    template<typename T>
    inline std::vector<T> range(T start, T step, int steps)
    {
        std::vector<T> range;
        
        for(int i{0}; i != steps; ++i)
        {
            range.push_back(start + i * step);
        }
        
        return range;
    }
}

#endif // AVERAGE_HPP
