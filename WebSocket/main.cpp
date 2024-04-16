#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include "CalculationClass.h"
#include "WebSocketClass.h"

int main()
{
   srand(time(0));
   std::atomic<bool> flag(false);
   std::atomic<int> WebSocketRequestsCount(0);
   std::atomic<int> heavyTasksCount(0);
   std::mutex mutex;
   CalculationClass Calculation(1000);

   std::string uri = "wss://ws.okx.com:8443/ws/v5/public";
   WebSocketClass webSocket(uri, WebSocketRequestsCount, mutex);

   std::cout << "=====================================================\n| ORDER BOOK FOR BTC-USDT AND INVERSE MATRIX AX = E |\n=====================================================\n";

   std::thread webSocketThread([&]()
                               { webSocket.wsrun(flag); });

   std::thread calculationThread([&]()
                                 { Calculation.run(flag, heavyTasksCount, mutex); });

   std::this_thread::sleep_for(std::chrono::minutes(1));
   flag.store(true);

   calculationThread.join();
   webSocketThread.join();

   std::cout << "Total WebSocket requests made: " << webSocket.m_WebSocketRequestsCount << std::endl;
   std::cout << "Total calculations completed: " << heavyTasksCount << std::endl;

   return 0;
}
