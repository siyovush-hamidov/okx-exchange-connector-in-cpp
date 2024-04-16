#ifndef OKXWEBSOCKET_H
#define OKXWEBSOCKET_H

#include <string>
#include <atomic>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

class OKXWebSocket {
private:
    std::string url_;
    std::string api_key_;
    std::string passphrase_;
    std::string secret_key_;
    websocketpp::client<websocketpp::config::asio_tls_client> client_;
    websocketpp::connection_hdl handle_;

    void on_message(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg);
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);

public:
    OKXWebSocket(const std::string& url, const std::string& api_key, const std::string& passphrase, const std::string& secret_key)
        : url_(url), api_key_(api_key), passphrase_(passphrase), secret_key_(secret_key) {
        client_.init_asio();
        client_.set_tls_init_handler([](websocketpp::connection_hdl) {
            return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
        });
        client_.set_message_handler(std::bind(&OKXWebSocket::on_message, this, std::placeholders::_1, std::placeholders::_2));
        client_.set_open_handler(std::bind(&OKXWebSocket::on_open, this, std::placeholders::_1));
        client_.set_close_handler(std::bind(&OKXWebSocket::on_close, this, std::placeholders::_1));
    }

    void connect();
    void run();
    void subscribe(const std::string& channel, const std::string& instType = "", const std::string& instFamily = "", const std::string& instId = "");
    void unsubscribe(const std::string& channel, const std::string& instType = "", const std::string& instFamily = "", const std::string& instId = "");
};

#endif // OKXWEBSOCKET_H
