#include <iostream>
#include <cstring>
#include <asio.hpp>

using asio::ip::tcp;

enum { max_length = 1024 };

int main(int argc, char const *argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: echo_cli <host> <port>" << std::endl;
            return 1;
        }
        asio::io_context io_context;
        
        tcp::socket socket(io_context);
        tcp::resolver resolver(io_context);
        asio::connect(socket, resolver.resolve(argv[1], argv[2]));

        std::cout << "Enter message: ";
        char request[max_length];
        std::cin.getline(request, max_length);
        size_t request_length = std::strlen(request);
        asio::write(socket, asio::buffer(request, request_length));

        char reply[max_length];
        size_t reply_length = asio::read(socket, asio::buffer(reply, request_length));
        std::cout << "Reply is: ";
        std::cout.write(reply, reply_length);
        std::cout << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}
