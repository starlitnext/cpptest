
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "RpcMeta.pb.h"
#include "google/protobuf/service.h"
#include "google/protobuf/stubs/callback.h"
#include "asio.hpp"

class RpcController
    : public ::google::protobuf::RpcController
{
public:
  virtual void Reset() { }

  virtual bool Failed() const { return false; }

  virtual std::string ErrorText() const { return ""; }

  virtual void StartCancel() { }

  virtual void SetFailed(const std::string& /*reason*/) { }

  virtual bool IsCanceled() const { return false; }

  virtual void NotifyOnCancel(::google::protobuf::Closure* callback) { }

};

class RpcChannel
    : public ::google::protobuf::RpcChannel
{
public:
    void init(const std::string& ip, const int port)
    {
        io_context_ = std::make_shared<asio::io_context>();
        sock_ = std::make_shared<asio::ip::tcp::socket>(*io_context_);
        asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(ip), port);
        // 使用同步的方式
        sock_->connect(endpoint);
    }

    virtual void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                          ::google::protobuf::RpcController* /*controller*/, 
                          const ::google::protobuf::Message* request,
                          ::google::protobuf::Message* response, 
                          ::google::protobuf::Closure*)
    {
        std::string serialized_data = request->SerializeAsString();

        rpc::RpcMeta rpc_meta;
        rpc_meta.set_service_name(method->service()->name());
        rpc_meta.set_method_name(method->name());
        rpc_meta.set_data_size(serialized_data.size());

        std::string serialized_str = rpc_meta.SerializeAsString();

        int serialized_size = serialized_str.size();
        serialized_str.insert(0, std::string((const char*)&serialized_size, sizeof(int)));
        serialized_str += serialized_data;

        sock_->send(asio::buffer(serialized_str));

        char resp_data_size[sizeof(int)];
        sock_->receive(asio::buffer(resp_data_size));
        int resp_data_len = *(int*)resp_data_size;
        std::vector<char> resp_data(resp_data_len, 0);
        sock_->receive(asio::buffer(resp_data));
        
        response->ParseFromString(std::string(&resp_data[0], resp_data.size()));
    }

private:
    std::shared_ptr<asio::io_context> io_context_;
    std::shared_ptr<asio::ip::tcp::socket> sock_;
};


class DoneParams
{
public:
    DoneParams(::google::protobuf::Message* recv_msg,
               ::google::protobuf::Message* resp_msg,
               std::shared_ptr<asio::ip::tcp::socket> sock)
        : recv_msg(recv_msg),
          resp_msg(resp_msg),
          sock(sock)
    {
    }

public:
    ::google::protobuf::Message *recv_msg;
    ::google::protobuf::Message *resp_msg;
    std::shared_ptr<asio::ip::tcp::socket> sock;
};

class RpcServer
{
public:
    void add(::google::protobuf::Service* service)
    {
        ServiceInfo service_info;
        service_info.service = service;
        service_info.sd = service->GetDescriptor();
        for (int i = 0; i < service_info.sd->method_count(); ++i)
        {
            service_info.mds[service_info.sd->method(i)->name()] = service_info.sd->method(i);
        }
        services_[service_info.sd->name()] = service_info;
    }

    void start(const std::string& ip, int port)
    {
        asio::io_context io_context;
        asio::ip::tcp::acceptor acceptor(
            io_context,
            asio::ip::tcp::endpoint(
                asio::ip::address::from_string(ip),
                port
            )
        );

        while (true)
        {
            auto sock = std::make_shared<asio::ip::tcp::socket>(io_context);
            acceptor.accept(*sock);

            std::cout << "recv from client:" << sock->remote_endpoint().address() << std::endl;

            char meta_size[sizeof(int)];
            sock->receive(asio::buffer(meta_size));
            int meta_len = *(int*)(meta_size);

            std::vector<char> meta_data(meta_len, 0);
            sock->receive(asio::buffer(meta_data));

            rpc::RpcMeta meta;
            meta.ParseFromString(std::string(&meta_data[0], meta_data.size()));

            std::vector<char> data(meta.data_size(), 0);
            sock->receive(asio::buffer(data));

            dispatch_msg(
                meta.service_name(),
                meta.method_name(),
                std::string(&data[0], data.size()),
                sock
            );
        }
        
    }

private:
    void dispatch_msg(
        const std::string& service_name,
        const std::string& method_name,
        const std::string& serialized_data,
        std::shared_ptr<asio::ip::tcp::socket>& sock)
    {
        auto service = services_[service_name].service;
        auto md = services_[service_name].mds[method_name];
        
        std::cout << "recv service_name:" << service_name << std::endl;
        std::cout << "recv method_name:" << method_name << std::endl;
        std::cout << "recv type:" << md->input_type()->name() << std::endl;
        std::cout << "resp type:" << md->output_type()->name() << std::endl;

        auto recv_msg = service->GetRequestPrototype(md).New();
        recv_msg->ParseFromString(serialized_data);
        auto resp_msg = service->GetResponsePrototype(md).New();

        RpcController controller;
        DoneParams params = {recv_msg, resp_msg, sock};
        auto done = ::google::protobuf::NewCallback(this, &RpcServer::on_resp_msg_filled, params);
        service->CallMethod(md, &controller, recv_msg, resp_msg, done);
    }

    void on_resp_msg_filled(DoneParams params)
    {
        // std::auto_ptr<::google::protobuf::Message> recv_msg_guard(params.recv_msg);
        // std::auto_ptr<::google::protobuf::Message> resp_msg_guard(params.resp_msg);
        std::string resp_str;
        pack_message(params.resp_msg, &resp_str);
        params.sock->send(asio::buffer(resp_str));
    }

    void pack_message(const ::google::protobuf::Message* msg, std::string* serialized_data)
    {
        msg->AppendToString(serialized_data);
    }

private:
    struct ServiceInfo {
        ::google::protobuf::Service* service;
        const ::google::protobuf::ServiceDescriptor* sd;
        std::map<std::string, const ::google::protobuf::MethodDescriptor*> mds;
    };

    std::map<std::string, ServiceInfo> services_;
};
