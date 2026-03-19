/*
    * curlgetexample.cpp
    *
    * Example of using libcurl to perform 
    * an HTTP GET request and print the response.
*/

#include <iostream>
#include <curl/curl.h>

// Callback function to write the response data into a string
// contents: pointer to the data received
// size: size of each data element
// nmemb: number of data elements
// userp: pointer to the user data(in this case, a string to store the response)
size_t WriteCallback(void* contents, size_t size, size_t nmemb, 
                    std::string* userp) {
                    
                    size_t totalSize = size * nmemb;
                    userp->append((char*)contents, totalSize);
                    
                    return totalSize;
} 

int main() {
    // Initialize libcurl globally
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    if (result != CURLE_OK) {
        std::cerr << "curl_global_init() failed: " 
                  << curl_easy_strerror(result) << std::endl;
        return (int)result;
    }

    // Create a curl handle to perform the request
    // Curl is just like a table of information 
    // about the request, and we can set various options on it
    CURL *curl = curl_easy_init();
    if (curl) {
        // Set the URL for the GET request
        curl_easy_setopt(curl, CURLOPT_URL, "https://github.com");

        // Set the callback function to write the response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        // String to store the response data
        std::string responseString;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        // Perform the request
        result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            std::cerr << "curl_global_init() failed: " 
                    << curl_easy_strerror(result) << std::endl;
            return (int)result;
        } else {
            // Print the response data
            std::cout << "Response from https://github.com:" << std::endl;
            // std::cout << responseString << std::endl;
        }

        // Clean up the curl handle
        curl_easy_cleanup(curl);
    }


    // Clean up libcurl globally
    curl_global_cleanup();

    return 0;
}