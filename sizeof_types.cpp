#include <gasnet.h>
#include <iostream>

int main()
{
    std::cout << "sizeof(int)                 = " << sizeof(int) << std::endl;
    std::cout << "sizeof(int *)               = " << sizeof(int *) << std::endl;
    std::cout << "sizeof(gasnet_handlerarg_t) = " << sizeof(gasnet_handlerarg_t) << std::endl;
}
