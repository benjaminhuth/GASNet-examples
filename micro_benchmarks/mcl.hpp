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
    
    template<class iterator_t, class container_t = std::vector<typename iterator_t::value_type>, class number_t = typename iterator_t::value_type>
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
    
    template<class iterator_t, class container_t = std::vector<typename iterator_t::value_type>, class number_t = typename iterator_t::value_type>
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


    template<typename container_t>
    inline void export_container(std::string filename, const container_t & c)
    {
        std::fstream file(filename, file.out);
        if(!file.is_open()) throw std::runtime_error("Could not open file \"" + filename + "\"");
        
        for(auto &element : c)
        {
            file << element << "\n";
        }
        
        file.close();
    }
    
    inline void export_containers(std::string filename) { }
        
    template<class container_t, class ... containers_t>
    inline void export_containers(std::string filename, const container_t &container, const containers_t& ... remaining)
    {    
        std::fstream file(filename, file.in);
        
        // file is empty
        if(!file.is_open())
        {
            std::cout << "first" << std::endl;
            file.clear();
            file.open(filename, file.out);
        
            if(!file.is_open())
                throw std::runtime_error("Could not open file \"" + filename + "\"");
            
            for(auto &el : container)
            {
                file << el << "\n";
            }
            
            file.close();
        }
        else
        {
            std::cout << "not first" << std::endl;
            
            file.close();
            file.open(filename, file.in | file.out);
            
            typename container_t::const_iterator it;
            for(it = container.begin(); it != container.end(); ++it)
            {
                while( file.peek() != '\n' )
                {
                    file.get();
                }
                file.seekp(file.tellg());
                file << '\t' << *it;
                file.seekg(file.tellp());
                file.get();
                
                if(file.get() == std::ifstream::traits_type::eof() )
                    break;
            }
            
            file.close();
            
            if( it != container.end()-1 )
                throw std::runtime_error("container sizes do not match");
            
            
            exit(1);
        }
        export_containers(filename, remaining...);
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
