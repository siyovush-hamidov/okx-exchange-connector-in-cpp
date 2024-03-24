#include "OKXClass.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

size_t OKXClass::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
   size_t total_size = size * nmemb;
   output->append((char *)contents, total_size);
   return total_size;
}
std::string OKXClass::generateSignature(const std::string &timestamp, const std::string &method, const std::string &requestPath, const std::string &secretKey)
{
   std::string message = timestamp + method + requestPath;

   unsigned char digest[EVP_MAX_MD_SIZE];
   unsigned int digestLength;

   HMAC(EVP_sha256(), secretKey.c_str(), secretKey.length(),
        reinterpret_cast<const unsigned char *>(message.c_str()), message.length(), digest, &digestLength);

   BIO *b64 = BIO_new(BIO_f_base64());
   BIO *bio = BIO_new(BIO_s_mem());
   bio = BIO_push(b64, bio);
   BIO_write(bio, digest, digestLength);
   BIO_flush(bio);

   BUF_MEM *bufferPtr;
   BIO_get_mem_ptr(bio, &bufferPtr);

   std::string base64String(bufferPtr->data, bufferPtr->length - 1);
   BIO_free_all(bio);

   return base64String;
}
std::string OKXClass::getCurrentUTCTimestamp()
{
   auto now = std::chrono::system_clock::now();
   auto now_time_t = std::chrono::system_clock::to_time_t(now);
   auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

   std::tm utc_tm;
   gmtime_r(&now_time_t, &utc_tm);

   std::ostringstream oss;
   oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%S");
   oss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
   oss << "Z";

   return oss.str();
}

void OKXClass::formatAndPrintResponse(const std::string &response_data) const
{
   nlohmann::json json_data = nlohmann::json::parse(response_data);

   if (json_data["code"] == "0")
   {
      const auto &asks_data = json_data["data"][0]["asks"];
      std::cout << "OKXClass: Asks:\n";
      for (const auto &ask : asks_data)
      {
         std::cout << "  Depth Price: " << ask[0] << std::endl;
         std::cout << "  Quantity: " << ask[1] << std::endl;
         std::cout << "  Deprecated Value: " << ask[2] << std::endl;
         std::cout << "  Number of Orders: " << ask[3] << std::endl;
      }

      const auto &bids_data = json_data["data"][0]["bids"];
      std::cout << "OKXClass: Bids:\n";
      for (const auto &bid : bids_data)
      {
         std::cout << "  Depth Price: " << bid[0] << std::endl;
         std::cout << "  Quantity: " << bid[1] << std::endl;
         std::cout << "  Deprecated Value: " << bid[2] << std::endl;
         std::cout << "  Number of Orders: " << bid[3] << std::endl;
      }
   }
   else
   {
      std::cerr << "OKXClass: Response Error: " << json_data["msg"] << std::endl;
   }
}
void OKXClass::makeRequest() const
{
   CURL *curl;
   CURLcode res;
   const std::string response_data;

   curl_global_init(CURL_GLOBAL_ALL);
   curl = curl_easy_init();
   if (curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

      std::string timestamp = getCurrentUTCTimestamp();
      if (!timestamp.empty())
      {
         std::cout << "OKXClass: Timestamp: " << timestamp << std::endl;
      }
      else
      {
         std::cerr << "OKXClass: Failed to get timestamp" << std::endl;
         return;
      }

      std::string method = "GET";                        
      std::string requestPath = "/api/v5/market/books?instId=" + instId_;                           

      std::string signature = generateSignature(timestamp, method, requestPath, secret_key_);

      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, ("OK-ACCESS-KEY: " + api_key_).c_str());
      headers = curl_slist_append(headers, ("OK-ACCESS-SIGN: " + signature).c_str());
      headers = curl_slist_append(headers, ("OK-ACCESS-TIMESTAMP: " + timestamp).c_str());
      headers = curl_slist_append(headers, ("OK-ACCESS-PASSPHRASE: " + passphrase_).c_str());
   
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

      res = curl_easy_perform(curl);
      if (res != CURLE_OK)
      {
         std::cerr << "OKXClass: Failed to perform request: " << curl_easy_strerror(res) << std::endl;
      }
      else
      {
         std::cout << "OKXClass: Response received for " + instId_ + '\n';
         formatAndPrintResponse(response_data);
      }

      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
   }
   else
   {
      std::cerr << "OKXClass: Failed to initialize libcurl" << std::endl;
      return;
   }

   curl_global_cleanup();
}
void OKXClass::run(const OKXClass &client, std::atomic<int> &okxRequestsCount, std::mutex &mutex, std::atomic<bool> &flag)
{
   auto startTime = std::chrono::steady_clock::now();
   while (!flag)
   {
      auto currentTime = std::chrono::steady_clock::now();
      auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);
      if (elapsedTime >= std::chrono::seconds(60))
      {
         std::cout << "OKXClass: Maximum duration reached. Stopping...\n";
         break;
      }

      makeRequest();
      {
         std::lock_guard<std::mutex> lock(mutex);
         std::cout << "OKXClass: OKX request #" << okxRequestsCount++ << " completed.\n\n";
      }
   }
}
