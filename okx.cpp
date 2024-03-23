#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include "CalculationClass.h"
#include "OKXClass.h"

int main()
{
   std::atomic<bool> flag(false);
   std::atomic<int> okxRequestsCount(0);
   std::atomic<int> heavyTasksCount(0);
   std::mutex mutex;
   CalculationClass CalculationClass(1000);

   // Retrieve environment variables
   char *api_key_env = std::getenv("OKX_API_KEY");
   char *secret_key_env = std::getenv("OKX_SECRET_KEY");
   char *passphrase_env = std::getenv("OKX_PASSPHRASE");

   // Check if environment variables are set
   if (!api_key_env || !secret_key_env || !passphrase_env)
   {
      std::cerr << "OKXClass: One or more required environment variables are not set." << std::endl;
      return 1;
   }

   // Convert environment variables to std::string
   std::string api_key(api_key_env);
   std::string secret_key(secret_key_env);
   std::string passphrase(passphrase_env);

   std::string url = "https://www.okx.com/api/v5/account/bills"; // Replace with your actual API endpoint

   OKXClass OKX(api_key, secret_key, passphrase, url);
   std::thread OKXClassThread([&]()
                              { OKX.run(OKX, okxRequestsCount, mutex, flag); });

   std::thread calculationThread([&]()
                                 { CalculationClass.run(flag, heavyTasksCount, mutex); });

   std::this_thread::sleep_for(std::chrono::minutes(1));
   flag.store(true);

   calculationThread.join();
   OKXClassThread.join();

   std::cout << "Total OKX requests made: " << okxRequestsCount << std::endl;
   std::cout << "Total heavy tasks completed: " << heavyTasksCount << std::endl;

   return 0;
}
