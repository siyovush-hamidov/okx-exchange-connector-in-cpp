#include "OKXWebSocket.h"
#include <iostream>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>


void OKXWebSocket::on_open(websocketpp::connection_hdl hdl) {
    std::cout << "Connected to OKX WebSocket" << std::endl;
    // Perform authentication if needed
}

void OKXWebSocket::on_message(websocketpp::connection_hdl hdl, websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr msg) {
    std::cout << "Received message: " << msg->get_payload() << std::endl;
    // Process received message
}

void OKXWebSocket::on_close(websocketpp::connection_hdl hdl) {
    std::cout << "Connection to OKX WebSocket closed" << std::endl;
}

void OKXWebSocket::connect() {
    websocketpp::lib::error_code ec;
    auto con = client_.get_connection(url_, ec);
    if (ec) {
        std::cerr << "Failed to create connection: " << ec.message() << std::endl;
        return;
    }
    handle_ = con->get_handle();
    client_.connect(con);
}

void OKXWebSocket::run() {
    client_.run();
}

void OKXWebSocket::subscribe(const std::string& channel, const std::string& instType, const std::string& instFamily, const std::string& instId) {
    // Construct subscribe message and send it
    std::string subscribe_msg = "{\"op\": \"subscribe\", \"args\": [{\"channel\": \"" + channel + "\", \"instType\": \"" + instType + "\", \"instFamily\": \"" + instFamily + "\", \"instId\": \"" + instId + "\"}]}";
    client_.send(handle_, subscribe_msg, websocketpp::frame::opcode::text);
}

void OKXWebSocket::unsubscribe(const std::string& channel, const std::string& instType, const std::string& instFamily, const std::string& instId) {
    // Construct unsubscribe message and send it
    std::string unsubscribe_msg = "{\"op\": \"unsubscribe\", \"args\": [{\"channel\": \"" + channel + "\", \"instType\": \"" + instType + "\", \"instFamily\": \"" + instFamily + "\", \"instId\": \"" + instId + "\"}]}";
    client_.send(handle_, unsubscribe_msg, websocketpp::frame::opcode::text);
}
