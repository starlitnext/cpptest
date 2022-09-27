
#include <iostream>
#include <asio/as_tuple.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>

using asio::as_tuple_t;
using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable_t;
using default_token = as_tuple_t<use_awaitable_t<>>;
using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
using tcp_socket = default_token::as_default_on_t<tcp::socket>;
namespace this_coro = asio::this_coro;

awaitable<void> echo(tcp_socket socket)
{
    char data[1024];
    for (;;)
    {
        auto [e1, nread] = co_await socket.async_read_some(asio::buffer(data));
        if (nread == 0)
            break;
        auto [e2, nwritten] = co_await async_write(socket, asio::buffer(data, nread));
        if (nwritten != nread)
            break;
    }
}

awaitable<void> listener(asio::ip::port_type port)
{
    auto executor = co_await this_coro::executor;
    tcp_acceptor acceptor(executor, {tcp::v4(), port});
    for (;;)
    {
        if (auto [e, socket] = co_await acceptor.async_accept(); socket.is_open())
            co_spawn(executor, echo(std::move(socket)), detached);
    }
}

int main(int argc, char const *argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cout << "Usage: co_echo_srv <port>" << std::endl;
            return 1;
        }
        asio::io_context io_context(1);

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io_context.stop(); });

        co_spawn(io_context, listener(std::atoi(argv[1])), detached);

        io_context.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}
