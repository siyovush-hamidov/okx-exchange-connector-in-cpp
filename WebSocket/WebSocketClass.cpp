#include "WebSocketClass.h"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

std::mutex WebSocketClass::m_mutex;
std::atomic<int> WebSocketClass::m_WebSocketRequestsCount(0);

std::string WebSocketClass::getCurrentUTCTimestamp()
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

void WebSocketClass::on_message(const std::string &response_data)
{
    std::string timestamp = getCurrentUTCTimestamp();
    std::cout << "WebSocketClass: Timestamp: " << timestamp << std::endl;

    nlohmann::json json_data = nlohmann::json::parse(response_data);

    if (json_data.contains("data"))
    {
        const auto &data = json_data["data"];
        if (data.is_array() && !data.empty())
        {
            const auto &asks_data = data[0]["asks"];
            std::cout << "WebSocketClass: Asks:\n";
            for (const auto &ask : asks_data)
            {
                std::cout << "  Depth Price: " << ask[0] << std::endl;
                std::cout << "  Quantity: " << ask[1] << std::endl;
                std::cout << "  Deprecated Value: " << ask[2] << std::endl;
                std::cout << "  Number of Orders: " << ask[3] << std::endl;
            }

            const auto &bids_data = data[0]["bids"];
            std::cout << "WebSocketClass: Bids:\n";
            for (const auto &bid : bids_data)
            {
                std::cout << "  Depth Price: " << bid[0] << std::endl;
                std::cout << "  Quantity: " << bid[1] << std::endl;
                std::cout << "  Deprecated Value: " << bid[2] << std::endl;
                std::cout << "  Number of Orders: " << bid[3] << std::endl;
            }
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_WebSocketRequestsCount++;
                std::cout << "WebSocketClass: Request # " << m_WebSocketRequestsCount << " completed\n\n";
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
}

context_ptr WebSocketClass::on_tls_init()
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

void WebSocketClass::on_open(client *m_client, websocketpp::connection_hdl hdl)
{
    std::string subscribe_msg = R"({"op":"subscribe","args":[{"channel":"bbo-tbt","instId":"BTC-USDT"}]})";
    websocketpp::lib::error_code ec;
    m_client->send(hdl, subscribe_msg, websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        std::cout << "subscription failed: " << ec.message() << std::endl;
    }
}

WebSocketClass::WebSocketClass(const std::string &uri, std::atomic<int> &WebSocketRequestsCount, std::mutex &mutex) : m_uri(uri)
{
    m_client.set_access_channels(websocketpp::log::alevel::all);
    m_client.clear_access_channels(websocketpp::log::alevel::frame_payload);

    m_client.init_asio();
    m_client.set_tls_init_handler(bind(&WebSocketClass::on_tls_init));

    m_client.set_message_handler([this](websocketpp::connection_hdl hdl, message_ptr msg)
                                 { on_message(msg->get_payload()); });
    m_client.set_open_handler(bind(&on_open, &m_client, ::_1));
}

void WebSocketClass::wsrun(std::atomic<bool> &flag)
{
    try
    {
        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_client.get_connection(m_uri, ec);
        if (ec)
        {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return;
        }
        m_client.connect(con);

        while (!flag)
        {
            m_client.run_one();
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
            m_client.get_io_service().run_one();
        }

        m_client.close(con->get_handle(), websocketpp::close::status::normal, "Closing connection");
        std::cout << "WebSocketClass has finished the work!\n";
    }
    catch (websocketpp::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
}
