#include <iostream>
#include <curl/curl.h>
#include <string>
#include <nlohmann/json.hpp> // Include JSON library, e.g., nlohmann/json

using json = nlohmann::json;

// Callback function to handle response data
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t total_size = size * nmemb;
    output->append((char *)contents, total_size);
    return total_size;
}

std::string getTimestamp()
{
    CURL *curl;
    CURLcode res;
    std::string urlTimestamp = "https://www.okx.com/api/v5/public/time";
    std::string response_data;
    std::string timestamp;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl)
    {
        // Set the URL for the request
        curl_easy_setopt(curl, CURLOPT_URL, urlTimestamp.c_str());

        // Set the callback function to handle response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        // Perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "Failed to perform request: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            // Parse JSON response
            try
            {
                json data = json::parse(response_data);
                // Extract timestamp from the response
                timestamp = data["data"][0]["ts"];
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "Failed to initialize libcurl" << std::endl;
    }

    // Cleanup libcurl
    curl_global_cleanup();

    return timestamp;
}

int main()
{
    std::string timestamp = getTimestamp();
    if (!timestamp.empty())
    {
        std::cout << "Timestamp: " << std::stoll(timestamp) / 1000 << std::endl;
    }
    else
    {
        std::cerr << "Failed to get timestamp" << std::endl;
    }

    return 0;
}
