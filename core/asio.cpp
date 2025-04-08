#include "asio.h"

static shared_ptr<asio::io_context> shared_io_context()
{
    static uint64_t run_time = 0;
    static shared_ptr<asio::io_context> context;
    if (context == nullptr)
    {
        context = make_shared<asio::io_context>(1);
        thread([=]() {
            shared_ptr<asio::io_context> io_context = context;
            async_timer([]() { run_time++; }, 1000ms);
            io_context->run();
        }).detach();
    }
    return context;
}

static void timer_async_wait(shared_ptr<asio::steady_timer> timer, function<void()> task, milliseconds interval)
{
    timer->async_wait([=](asio::error_code error) {
        task();
        timer->expires_after(interval);
        timer_async_wait(timer, task, interval);
    });
}

static void udp_async_recv(
    shared_ptr<asio::ip::udp::socket> socket,
    shared_ptr<vector<char>> buffer,
    shared_ptr<asio::ip::udp::endpoint> address,
    function<void(udp socket, uint32_t ip, uint16_t port, char* data, uint32_t size)> recved)
{
    socket->async_receive_from(
        asio::mutable_buffer(buffer->data(), buffer->size()),
        *address,
        [=](error_code error, size_t size) {
        if (!error)
        {
            uint32_t ip = address->address().to_v4().to_ulong();
            uint16_t port = address->port();
            if (recved)
            {
                recved(socket, ip, port, buffer->data(), (uint32_t)size);
            }
        }
        if (error != asio::error::operation_aborted && error != asio::error::bad_descriptor)
        {
            udp_async_recv(socket, buffer, address, recved);
        }
    });
}

void async_task(function<void()> task)
{
    shared_io_context()->post(task);
}

void async_timer(function<void()> task, milliseconds interval)
{
    auto timer = make_shared<asio::steady_timer>(*shared_io_context());
    timer_async_wait(timer, task, interval);
}

uint32_t ip_from_string(string ip)
{
    if (ip.empty())
    {
        return 0;
    }
    return asio::ip::address_v4::from_string(ip).to_ulong();
}

string ip_to_string(uint32_t ip)
{
    return asio::ip::address_v4(ip).to_string();
}

uint32_t udp_get_host_ip()
{
    try
    {
        asio::ip::udp::socket udp(*shared_io_context());
        udp.open(asio::ip::udp::v4());
        udp.connect(asio::ip::udp::endpoint(asio::ip::address::from_string("1.2.1.3"), 1988));
        auto local_endpoint = udp.local_endpoint();
        return local_endpoint.address().to_v4().to_ulong();
    }
    catch (...)
    {
        return 0;
    }
}

uint16_t udp_get_host_port(udp socket)
{
    return socket->local_endpoint().port();
}

udp udp_open(function<void(udp socket, uint32_t ip, uint16_t port, char* data, uint32_t size)> recved, uint16_t port)
{
    static random_device rd;
    static uniform_int_distribution<uint16_t> random(10000, 60000);

    auto socket = make_shared<asio::ip::udp::socket>(*shared_io_context());
    socket->open(asio::ip::udp::v4());
    
    try
    {
        if (port != 0)
        {
            socket->set_option(asio::socket_base::reuse_address(1));
        }
        else
        {
            port = random(rd);
        }
        socket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
    }
    catch (...)
    {
        socket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
    }

    auto buffer = make_shared<vector<char>>(2048);
    auto address = make_shared<asio::ip::udp::endpoint>();

    udp_async_recv(socket, buffer, address, recved);

    return socket;
}

bool udp_connect(udp socket, uint32_t ip, uint16_t port)
{
    try
    {
        socket->connect(asio::ip::udp::endpoint(asio::ip::address_v4(ip), port));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void udp_close(udp socket)
{
    socket->get_io_context().post([=]() {
        socket->close();
    });
}

uint32_t udp_send(udp socket, const void* data, uint32_t size)
{
    try
    {
        return (uint32_t)socket->send(asio::mutable_buffer((char*)data, size));
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t udp_send(udp socket, uint32_t ip, uint16_t port, const void* data, uint32_t size)
{
    try
    {
        asio::ip::udp::endpoint target(asio::ip::address_v4(ip), port);
        return (uint32_t)socket->send_to(asio::mutable_buffer((char*)data, size), target);
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t udp_send(udp socket, uint32_t ip, uint16_t port, const void* head, uint32_t head_size, const void* body, uint32_t body_size)
{
    try
    {
        vector<char> msg(head_size + body_size);
        memcpy(msg.data(), head, head_size);
        memcpy(msg.data() + head_size, body, body_size);
        asio::ip::udp::endpoint target(asio::ip::address_v4(ip), port);
        return (uint32_t)socket->send_to(asio::mutable_buffer((char*)msg.data(), msg.size()), target);
    }
    catch (...)
    {
        return 0;
    }
}

void udp_set_ttl(udp socket, int ttl)
{
    auto fd = socket->native_handle();
    setsockopt(fd, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl));
}
