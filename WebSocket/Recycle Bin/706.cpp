#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <nlohmann/json.hpp>

#include <sstream>
#include <ctime>
#include <fstream>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

std::string getCurrentUTCTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm utc_tm;
    gmtime_r(&now_time_t, &utc_tm);

    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

    return oss.str();
}
std::ofstream result_file;
void on_message(const std::string &response_data)
{
    std::string timestamp = getCurrentUTCTimestamp();
    std::cout << "Timestamp: " << timestamp << std::endl;

    nlohmann::json json_data = nlohmann::json::parse(response_data);

    if (json_data.contains("data"))
    {
        const auto &data = json_data["data"];
        if (data.is_array() && !data.empty())
        {
            const auto &asks_data = data[0]["asks"];
            std::cout << "Asks:\n";
            for (const auto &ask : asks_data)
            {
                std::cout << "  Depth Price: " << ask[0] << std::endl;
                std::cout << "  Quantity: " << ask[1] << std::endl;
                std::cout << "  Deprecated Value: " << ask[2] << std::endl;
                std::cout << "  Number of Orders: " << ask[3] << std::endl
                          << std::endl;
            }

            const auto &bids_data = data[0]["bids"];
            std::cout << "Bids:\n";
            for (const auto &bid : bids_data)
            {
                std::cout << "  Depth Price: " << bid[0] << std::endl;
                std::cout << "  Quantity: " << bid[1] << std::endl;
                std::cout << "  Deprecated Value: " << bid[2] << std::endl;
                std::cout << "  Number of Orders: " << bid[3] << std::endl
                          << std::endl;
            }
        }
        else
        {
            std::cerr << "No data available in the response." << std::endl;
        }
    }
    else
    {
        std::cerr << "Invalid response format." << std::endl;
    }
    // result_file.close(); // Close file after writing
}

static context_ptr on_tls_init()
{
    context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try
    {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
    }
    catch (std::exception &e)
    {
        std::cout << "Error in context pointer: " << e.what() << std::endl;
    }
    return ctx;
}

void on_open(client *c, websocketpp::connection_hdl hdl)
{
    // Subscription request
    std::string subscribe_msg = R"({"op":"subscribe","args":[{"channel":"bbo-tbt","instId":"BTC-USDT"}]})";
    websocketpp::lib::error_code ec;
    c->send(hdl, subscribe_msg, websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        std::cout << "subscription failed: " << ec.message() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    client c;

    std::string uri = "wss://ws.okx.com:8443/ws/v5/public";

    try
    {
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);

        c.init_asio();
        c.set_tls_init_handler(bind(&on_tls_init));

        c.set_message_handler([&c](websocketpp::connection_hdl hdl, message_ptr msg)
                              { on_message(msg->get_payload()); });
        c.set_open_handler(bind(&on_open, &c, ::_1)); // Add open handler

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec)
        {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        c.connect(con);

        c.run();
    }
    catch (websocketpp::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
}
