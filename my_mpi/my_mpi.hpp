/*
 * implements MPI simalar api with gasnet
 */

#ifndef MY_MPI_H
#define MY_MPI_H

#include <vector>
#include <utility>
#include <cstddef>
#include <iostream>

class my_mpi
{
public:
    my_mpi();
    ~my_mpi();
    
    int rank();
    int world_size();
    std::string hostename();
    
    template<typename datatype_t> void send_data(int dest_node, int id, std::vector<datatype_t> &data);
    template<typename datatype_t> std::vector<datatype_t> recv_data(int id);
    
    void barrier();
    
private:
    void send_gasnet_request(int dest_node, int id, char *data, std::size_t size);
    std::pair<char *, std::size_t> wait_for_message_arrival(int id);
};
    
template<typename datatype_t>
auto my_mpi::send_data(int dest_node, int id, std::vector<datatype_t> &data) -> void
{
    send_gasnet_request(dest_node, id, reinterpret_cast<char *>(data.data()), data.size()*sizeof(datatype_t) );
}
    
template<typename datatype_t>
auto my_mpi::recv_data(int id) -> std::vector<datatype_t>
{
    auto msg_data = wait_for_message_arrival(id);
    
    auto ptr = reinterpret_cast<datatype_t *>(msg_data.first);
    auto size = msg_data.second / sizeof(datatype_t);
    
    return std::move(std::vector<datatype_t>(ptr, ptr+size));
}

#endif // MY_MPI_H
