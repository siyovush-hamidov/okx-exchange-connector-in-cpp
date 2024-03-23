#ifndef OKXCLASS_H
#define OKXCLASS_H

#include <string>
#include <atomic>
#include <mutex>

class OKXClass
{
private:
   std::string api_key_, secret_key_, passphrase_, url_;
   static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output);
   static std::string generateSignature(const std::string &timestamp, const std::string &method, const std::string &requestPath, const std::string &secretKey);
   static std::string getCurrentUTCTimestamp();

public:
   OKXClass(const std::string &api_key, const std::string &secret_key, const std::string &passphrase, const std::string &url)
       : api_key_(api_key), secret_key_(secret_key), passphrase_(passphrase), url_(url) {}
   void makeRequest() const;
   void run(const OKXClass &client, std::atomic<int> &okxRequestsCount, std::mutex &mutex, std::atomic<bool> &flag);
};

#endif // OKXCLASS_H