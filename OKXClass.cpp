#include "OKXClass.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <curl/curl.h>

size_t OKXClass::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
   size_t total_size = size * nmemb;
   output->append((char *)contents, total_size);
   return total_size;
}
std::string OKXClass::generateSignature(const std::string &timestamp, const std::string &method, const std::string &requestPath, const std::string &secretKey)
{
   std::string message = timestamp + method + requestPath;

   // Create a buffer to store the HMAC SHA256 result
   unsigned char digest[EVP_MAX_MD_SIZE];
   unsigned int digestLength;

   // Compute the HMAC SHA256
   HMAC(EVP_sha256(), secretKey.c_str(), secretKey.length(),
        reinterpret_cast<const unsigned char *>(message.c_str()), message.length(), digest, &digestLength);

   // Encode the digest in Base64
   BIO *b64 = BIO_new(BIO_f_base64());
   BIO *bio = BIO_new(BIO_s_mem());
   bio = BIO_push(b64, bio);
   BIO_write(bio, digest, digestLength);
   BIO_flush(bio);

   BUF_MEM *bufferPtr;
   BIO_get_mem_ptr(bio, &bufferPtr);

   std::string base64String(bufferPtr->data, bufferPtr->length - 1); // Remove newline character
   BIO_free_all(bio);

   return base64String;
}
std::string OKXClass::getCurrentUTCTimestamp()
{
   // Get the current time in UTC
   auto now = std::chrono::system_clock::now();
   auto now_time_t = std::chrono::system_clock::to_time_t(now);
   auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

   // Convert to struct tm
   std::tm utc_tm;
   gmtime_r(&now_time_t, &utc_tm);

   // Format the timestamp string
   std::ostringstream oss;
   oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%S");
   oss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
   oss << "Z";

   return oss.str();
}
void OKXClass::makeRequest() const
{
   CURL *curl;
   CURLcode res;
   std::string response_data;

   // Initialize libcurl
   curl_global_init(CURL_GLOBAL_ALL);
   curl = curl_easy_init();
   if (curl)
   {
      // Set the URL for the request
      curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());

      // Set the callback function to handle response data
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

      // Generate timestamp
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

      // Compose message for signature
      std::string method = "GET";                        // Replace with your actual request method
      std::string requestPath = "/api/v5/account/bills"; // Replace with your actual request path
      std::string body = "";                             // Replace with your actual request body if any

      // Generate signature
      std::string signature = generateSignature(timestamp, method, requestPath, secret_key_);

      // Set the access key, signature, timestamp, and passphrase in the request headers
      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, ("OK-ACCESS-KEY: " + api_key_).c_str());
      headers = curl_slist_append(headers, ("OK-ACCESS-SIGN: " + signature).c_str());
      headers = curl_slist_append(headers, ("OK-ACCESS-TIMESTAMP: " + timestamp).c_str());
      headers = curl_slist_append(headers, ("OK-ACCESS-PASSPHRASE: " + passphrase_).c_str());
      // headers = curl_slist_append(headers, "Content-Type: application/json"); // Set content type
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

      // Perform the request
      res = curl_easy_perform(curl);
      if (res != CURLE_OK)
      {
         std::cerr << "OKXClass: Failed to perform request: " << curl_easy_strerror(res) << std::endl;
      }
      else
      {
         std::cout << "OKXClass: Response received:\n"
                   << "OKXClass: " << response_data << std::endl;
      }

      // Cleanup
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
   }
   else
   {
      std::cerr << "OKXClass: Failed to initialize libcurl" << std::endl;
      return;
   }

   // Cleanup libcurl
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
      // Optional: You can adjust or remove the sleep duration as needed
      // std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate request delay
   }
}
