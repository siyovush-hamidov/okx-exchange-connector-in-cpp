#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include "OKXWebSocket.h"

int main()
{
    std::atomic<bool> flag(false);
    std::atomic<int> okxRequestsCount(0);
    std::mutex mutex;

    char *api_key_env = std::getenv("OKX_API_KEY");
    char *secret_key_env = std::getenv("OKX_SECRET_KEY");
    char *passphrase_env = std::getenv("OKX_PASSPHRASE");

    if (!api_key_env || !secret_key_env || !passphrase_env)
    {
        std::cerr << "OKXWebSocket: One or more required environment variables are not set." << std::endl;
        return 1;
    }

    std::string api_key(api_key_env);
    std::string secret_key(secret_key_env);
    std::string passphrase(passphrase_env);
    std::string instId = "BTC-USDT";
    std::string url = "wss://ws.okx.com:8443/ws/v5/public";
    
    OKXWebSocket okxWebSocket(url, api_key, passphrase, secret_key);
    okxWebSocket.connect();
    okxWebSocket.subscribe("books", "", "", instId);
    
    // Allow some time for the subscription to be processed
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Run indefinitely or until a flag is set
    while (!flag)
    {
        // Perform other tasks if needed
        
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Example: Sleep for 1 second
    }

    // Unsubscribe and disconnect
    okxWebSocket.unsubscribe("books", "", "", instId);

    return 0;
}
