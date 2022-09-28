#define ASIO_ENABLE_HANDLER_TRACKING

#include <array>
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <asio/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>

using asio::buffer;
using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
namespace this_coro = asio::this_coro;
using namespace asio::experimental::awaitable_operators;
using std::chrono::steady_clock;
using namespace std::literals::chrono_literals;

constexpr auto use_no_throw_awaitable = asio::as_tuple(asio::use_awaitable);

awaitable<void> timeout(steady_clock::duration duration)
{
    asio::steady_timer timer(co_await this_coro::executor);
    timer.expires_after(duration);
    co_await timer.async_wait(use_no_throw_awaitable);
}

awaitable<void> transfer(tcp::socket& from, tcp::socket& to)
{
    std::array<char, 1024> data;

    for (;;)
    {
        auto result1 = co_await (
            from.async_read_some(buffer(data), use_no_throw_awaitable) ||
            timeout(5s)
        );
        if (result1.index() == 1)
            co_return;  // timed out

        auto [e1, n1] = std::get<0>(result1);
        if (e1)
            break;

        auto result2 = co_await (
            asio::async_write(to, buffer(data, n1), use_no_throw_awaitable) ||
            timeout(1s)
        );
        if (result2.index() == 1)
            co_return;  // timed out

        auto [e2, n2] = std::get<0>(result1);
        if (e2)
            break;
    }
}

awaitable<void> proxy(tcp::socket client, tcp::endpoint target)
{
    tcp::socket server(client.get_executor());
    steady_clock::time_point client_to_server_deadline{};
    steady_clock::time_point server_to_client_deadline{};

    auto [e] = co_await server.async_connect(target, use_no_throw_awaitable);
    if (!e)
    {
        co_await (
            transfer(client, server) ||
            transfer(server, client)
        );
    }
}

awaitable<void> listen(tcp::acceptor& acceptor, tcp::endpoint target)
{
    for (;;)
    {
        auto [e1, client] = co_await acceptor.async_accept(use_no_throw_awaitable);
        if (e1)
            break;

        auto ex = client.get_executor();
        co_spawn(ex, proxy(std::move(client), target), detached);
    }
}

int main(int argc, char const *argv[])
{
    try
    {
        if (argc != 5)
        {
            std::cerr << "Usage: proxy";
            std::cerr << " <listen_address> <listen_port>";
            std::cerr << " <target_address> <target_port>\n";
            return 1;
        }

        asio::io_context ctx;

        auto listen_endpoint = 
            *tcp::resolver(ctx).resolve(
                argv[1],
                argv[2],
                tcp::resolver::passive
            );
        auto target_endpoint = 
            *tcp::resolver(ctx).resolve(
                argv[3],
                argv[4]
            );
        
        tcp::acceptor acceptor(ctx, listen_endpoint);

        co_spawn(ctx, listen(acceptor, target_endpoint), detached);

        ctx.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}

