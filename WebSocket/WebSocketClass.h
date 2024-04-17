#ifndef WEBSOCKET_CLASS_H
#define WEBSOCKET_CLASS_H

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <nlohmann/json.hpp>

#include <sstream>
#include <ctime>

#include <atomic>
#include <mutex>

using client = websocketpp::client<websocketpp::config::asio_tls_client>;
using context_ptr = std::shared_ptr<boost::asio::ssl::context>;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using message_ptr = websocketpp::config::asio_client::message_type::ptr; 

class WebSocketClass
{
private:
    client m_client;
    std::string m_uri;

    static std::string getCurrentUTCTimestamp();
    static void on_message(const std::string &response_data);
    static context_ptr on_tls_init();
    static void on_open(client *m_client, websocketpp::connection_hdl hdl);

public:
    WebSocketClass(const std::string &uri, std::atomic<int> &WebSocketRequestsCount, std::mutex &mutex);
    void wsrun(std::atomic<bool> &flag);
    static std::mutex m_mutex;
    static std::atomic<int> m_WebSocketRequestsCount;
};

#endif // WEBSOCKET_CLASS_H
