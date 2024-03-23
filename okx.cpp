#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <cstring>
#include <nlohmann/json.hpp>
#include <ctime>
#include <sstream>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <cstdlib>
#include <unistd.h>

class CalculationClass
{
private:
   int n;
   double *A, *X, *A_temp, *X_temp, *E;
   void printRow(int i);
   bool allocateMemory(double **array, int size);
   void handleMemoryError();

public:
   CalculationClass(int size);
   // ~CalculationClass();
   void printResult();
   void printEquation();
   void mainElement();
   void mainElementTemp();
   int gaussJordan();
   void matrixMultiplication();
   double calculateAccuracy();
   void run(std::atomic<bool> &flagn);
};

class OKXClass
{
private:
   std::string api_key_;
   std::string secret_key_;
   std::string passphrase_;
   std::string url_;
   static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output);
   static std::string generateSignature(const std::string &timestamp, const std::string &method, const std::string &requestPath, const std::string &secretKey);
   static std::string getCurrentUTCTimestamp();

public:
   OKXClass(const std::string &api_key, const std::string &secret_key, const std::string &passphrase, const std::string &url)
       : api_key_(api_key), secret_key_(secret_key), passphrase_(passphrase), url_(url) {}
   void makeRequest() const;
   void run(const OKXClass &client);
};

int main()
{
   std::atomic<bool> flag(false);
   CalculationClass CalculationClass(1000);

   // Retrieve environment variables
   char *api_key_env = std::getenv("OKX_API_KEY");
   char *secret_key_env = std::getenv("OKX_SECRET_KEY");
   char *passphrase_env = std::getenv("OKX_PASSPHRASE");

   // Check if environment variables are set
   if (!api_key_env || !secret_key_env || !passphrase_env)
   {
      std::cerr << "One or more required environment variables are not set." << std::endl;
      return 1;
   }

   // Convert environment variables to std::string
   std::string api_key(api_key_env);
   std::string secret_key(secret_key_env);
   std::string passphrase(passphrase_env);

   std::string url = "https://www.okx.com/api/v5/account/bills"; // Replace with your actual API endpoint

   OKXClass OKX(api_key, secret_key, passphrase, url);

   std::thread compTaskThread([&]()
                              { CalculationClass.run(flag); });
   std::thread OKXClassThread([&]()
                              { OKX.makeRequest();});

   compTaskThread.join();
   OKXClassThread.join();

   return 0;
}

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
         std::cout << "Timestamp: " << timestamp << std::endl;
      }
      else
      {
         std::cerr << "Failed to get timestamp" << std::endl;
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
         std::cerr << "Failed to perform request: " << curl_easy_strerror(res) << std::endl;
      }
      else
      {
         std::cout << "Response received:\n"
                   << response_data << std::endl
                   << std::endl;
      }

      // Cleanup
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
   }
   else
   {
      std::cerr << "Failed to initialize libcurl" << std::endl;
      return;
   }

   // Cleanup libcurl
   curl_global_cleanup();
}
void OKXClass::run(const OKXClass &client)
{
   double epsilon = 1e-15;
   while (true)
   {
      client.makeRequest();
      sleep(epsilon);
   }
}

CalculationClass::CalculationClass(int size)
{
   n = size;

   if (!allocateMemory(&A, n * n + n * n))
   {
      handleMemoryError();
   }
   if (!allocateMemory(&A_temp, n * n + n * n))
   {
      handleMemoryError();
   }
   if (!allocateMemory(&E, n * n))
   {
      handleMemoryError();
   }

   X = A + n * n;
   X_temp = A_temp + n * n;
   int i;

   for (i = 0; i < n * n; i++)
   {
      A[i] = double(rand() % 10000) / double(1000);
   }

   for (i = 0; i < n * n; i++)
   {
      if (i % (n + 1) == 0)
         X[i] = 1.0;
      else
         X[i] = 0.0;
   }

   for (i = 0; i < n * n; i++)
   {
      A_temp[i] = A[i];
      X_temp[i] = X[i];
   }
}
void CalculationClass::printRow(int i)
{
   double epsilon = 1e-10;

   if (n >= 10)
   {
      std::cout << std::fixed;
      for (int j = 0; j < 5; j++)
         std::cout << std::setw(8) << std::setprecision(3) << (fabs(A[i * n + j]) < epsilon ? 0 : A[i * n + j]);
      std::cout << std::setw(8) << "..." << std::setw(8) << std::setprecision(3) << (fabs(A[i * n + n - 1]) < epsilon ? 0 : A[i * n + n - 1]) << " | ";
      for (int j = 0; j < 5; j++)
         std::cout << std::setw(8) << std::setprecision(3) << (fabs(X[i * n + j]) < epsilon ? 0 : X[i * n + j]);
      std::cout << std::setw(8) << "..." << std::setw(8) << std::setprecision(3) << (fabs(X[i * n + n - 1]) < epsilon ? 0 : X[i * n + n - 1]) << "\n";
   }
   else
   {
      for (int j = 0; j < n; j++)
         std::cout << std::setw(8) << std::setprecision(3) << (fabs(A[i * n + j]) < epsilon ? 0 : A[i * n + j]);
      std::cout << " | ";
      for (int j = 0; j < n; j++)
         std::cout << std::setw(8) << std::setprecision(3) << (fabs(X[i * n + j]) < epsilon ? 0 : X[i * n + j]);
      std::cout << '\n';
   }
}
void CalculationClass::printResult()
{
   double epsilon = 1e-10;
   if (n >= 10)
   {
      std::cout << std::fixed;
      for (int i = 0; i < 5; i++)
      {
         for (int j = 0; j < 5; j++)
            std::cout << std::setw(8) << std::setprecision(3) << (fabs(E[i * n + j]) < epsilon ? 0 : E[i * n + j]);
         std::cout << std::setw(8) << "..." << std::setw(8) << std::setprecision(3) << (fabs(E[i * n + n - 1]) < epsilon ? 0 : E[i * n + n - 1]) << '\n';
      }
   }
   else
   {
      for (int i = 0; i < n; i++)
      {
         for (int j = 0; j < n; j++)
            std::cout << std::setw(8) << std::setprecision(2) << (fabs(E[i * n + j]) < epsilon ? 0 : E[i * n + j]) << ' ';
         std::cout << '\n';
      }
   }
}
void CalculationClass::printEquation()
{
   if (n >= 10)
   {
      for (int i = 0; i < 5; i++)
         printRow(i);
      std::cout << '\n';
      for (int i = 0; i < 7; i++)
         std::cout << std::setw(8) << "...";
      std::cout << " | ";
      for (int i = 0; i < 7; i++)
         std::cout << std::setw(8) << "...";
      std::cout << '\n'
                << '\n';
      printRow(n - 1);
      std::cout << '\n';
   }
   else
      for (int i = 0; i < n; i++)
         printRow(i);
   std::cout << '\n';
}
void CalculationClass::mainElement()
{
   int i, j, k, max_row;
   double temp, max_element;

   for (i = 0; i < n; i++)
   {
      max_element = A[i * n + i];
      max_row = i;
      for (j = i + 1; j < n; j++)
      {
         if (fabs(max_element) < fabs(A[j * n + i]))
         {
            max_element = fabs(A[j * n + i]);
            max_row = j;
         }
      }

      if (fabs(max_element) < 1.e-20)
      {
         std::cout << "The matrix cannot be inverted!\n";
         return;
      }

      for (j = i; j < n; j++)
      {
         temp = A[i * n + j];
         A[i * n + j] = A[max_row * n + j];
         A[max_row * n + j] = temp;
      }
   }
}
void CalculationClass::mainElementTemp()
{
   int i, j, k, max_row;
   double temp, max_element;

   for (i = 0; i < n; i++)
   {
      max_element = A_temp[i * n + i];
      max_row = i;
      for (j = i + 1; j < n; j++)
      {
         if (fabs(max_element) < fabs(A_temp[j * n + i]))
         {
            max_element = fabs(A_temp[j * n + i]);
            max_row = j;
         }
      }

      if (fabs(max_element) < 1.e-20)
      {
         std::cout << "The matrix cannot be inverted!\n";
         return;
      }

      for (j = i; j < n; j++)
      {
         temp = A_temp[i * n + j];
         A_temp[i * n + j] = A_temp[max_row * n + j];
         A_temp[max_row * n + j] = temp;
      }
   }
}
int CalculationClass::gaussJordan()
{
   int i, j, k, max_row;
   double temp, max_element;

   mainElement();

   for (i = 0; i < n; i++)
   {
      temp = 1. / A[i * n + i];

      for (j = i; j < n; j++)
         A[i * n + j] *= temp;

      for (j = 0; j < n; j++)
         X[i * n + j] *= temp;

      for (j = i + 1; j < n; j++)
      {
         temp = A[j * n + i];
         for (k = i; k < n; k++)
            A[j * n + k] -= temp * A[i * n + k];

         for (k = 0; k < n; k++)
            X[j * n + k] -= temp * X[i * n + k];
      }
   }
   for (i = n - 1; i >= 0; i--)
   {
      for (j = i - 1; j >= 0; j--)
      {
         temp = A[j * n + i];
         for (k = 0; k < n; k++)
         {
            A[j * n + k] -= temp * A[i * n + k];
            X[j * n + k] -= temp * X[i * n + k];
         }
      }
   }
   return 0;
}
void CalculationClass::matrixMultiplication()
{
   int i, j, k;
   for (i = 0; i < n * n; i++)
      E[i] = 0.0;
   for (i = 0; i < n; i++)
      for (j = 0; j < n; j++)
         for (k = 0; k < n; k++)
            E[i * n + j] += A_temp[i * n + k] * X[k * n + j];
}
double CalculationClass::calculateAccuracy()
{
   double norma = 0.;
   int i, j, k;
   for (i = 0; i < n; i++)
   {
      for (j = 0; j < n; j++)
      {
         norma += E[i * n + j] * E[i * n + j];
      }
   }
   norma = sqrt(norma);
   return fabs(sqrt(n) - norma);
}
bool CalculationClass::allocateMemory(double **array, int size)
{
   *array = (double *)malloc(size * sizeof(double));
   if (!(*array))
   {
      std::cerr << "Error allocating memory for matrix!\n";
      return false;
   }
   return true;
}
void CalculationClass::handleMemoryError()
{
   std::cerr << "Error allocating memory. Program terminating.\n";
   exit(1); // Exit with an error code (optional)
}
void CalculationClass::run(std::atomic<bool> &flag)
{
   double variable_for_time;
   try
   {
      srand(time(0));

      std::cout << "CalculationClass:\nMatrix " << n << " by " << n << " filled successfully.\n\n";
      printEquation();

      variable_for_time = clock();
      gaussJordan();
      gaussJordan();
      mainElementTemp();
      matrixMultiplication();
      variable_for_time = clock() - variable_for_time;

      std::cout << "CalculationClass:\nMatrix A and Matrix X after the gaussJordan function: \n\n";
      printEquation();

      std::cout << "CalculationClass:\nResult of multiplying A (original) by X (after gaussJordan): \n\n";
      printResult();

      std::cout << "\nCalculationClass: Calculation time in seconds: " << std::setw(6) << std::setprecision(5) << variable_for_time / CLOCKS_PER_SEC << "\n";
      std::cout << std::scientific << std::setprecision(6) << "CalculationClass: L2 Norm ||AX - E|| = " << calculateAccuracy() << '\n';
      std::cout << "CalculationClass: Finished!\n";
   }
   catch (const std::bad_alloc &e)
   {
      std::cerr << "Memory allocation error: " << e.what() << std::endl;
   }
}
