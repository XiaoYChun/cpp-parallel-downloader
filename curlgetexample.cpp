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

        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); 
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

        // Perform the request
        result = curl_easy_perform(curl);
        long responseCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
        if (result == CURLE_OK) {
            // Print the response data
            std::cout << "Response from https://github.com" << std::endl;
            std::cout << "Response Size: " << responseString.size() << " bytes\n" 
                      << "Response Code: " << responseCode << std::endl;
        } else {
            std::cerr << "curl_easy_perform(curl) failed: " 
                    << curl_easy_strerror(result) << std::endl;
            return (int)result;
        }

        // Clean up the curl handle
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "curl_easy_init() failed" << std::endl;
        return -1;
    }


    // Clean up libcurl globally
    curl_global_cleanup();

    return 0;
}