#define ASIO_ENABLE_HANDLER_TRACKING

#include <array>
#include <iostream>
#include <memory>
#include <asio.hpp>
#include <asio/as_tuple.hpp>

using asio::buffer;
using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;

constexpr auto use_no_throw_awaitable = asio::as_tuple(asio::use_awaitable);

struct proxy_state
{
    proxy_state(tcp::socket client)
        : client(std::move(client))
    {
    }

    tcp::socket client;
    tcp::socket server{client.get_executor()};
};

using proxy_state_ptr = std::shared_ptr<proxy_state>;

awaitable<void> client_to_server(proxy_state_ptr state)
{
    std::array<char, 1024> data;

    for (;;)
    {
        auto [e1, n1] = co_await state->client.async_read_some(buffer(data), use_no_throw_awaitable);
        if (e1)
            break;

        auto [e2, n2] = co_await asio::async_write(state->server, buffer(data, n1), use_no_throw_awaitable);
        if (e2)
            break;
    }
    state->client.close();
    state->server.close();
}

awaitable<void> server_to_client(proxy_state_ptr state)
{
    std::array<char, 1024> data;

    for (;;)
    {
        auto [e1, n1] = co_await state->server.async_read_some(buffer(data), use_no_throw_awaitable);
        if (e1)
            break;

        auto [e2, n2] = co_await asio::async_write(state->client, buffer(data, n1), use_no_throw_awaitable);
        if (e2)
            break;
    }
    state->client.close();
    state->server.close();
}

awaitable<void> proxy(tcp::socket client, tcp::endpoint target)
{
    auto state = std::make_shared<proxy_state>(std::move(client));

    auto [e] = co_await state->server.async_connect(target, use_no_throw_awaitable);
    if (!e)
    {
        auto ex = state->client.get_executor();
        co_spawn(ex, client_to_server(state), detached);

        co_await server_to_client(state);
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
