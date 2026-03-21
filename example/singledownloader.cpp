/*
    * singledownloader.cpp
    *
    * Example of using libcurl to download
    * things in output files.
*/

#include <curl/curl.h>

#include <iostream>
#include <string>
#include <fstream>

/*
    * for a downloader
    * first need the GET url or target url
    * then need the result url to save what I download
*/

size_t WriteCallback(void* contents, size_t size, size_t nmemb, 
                    void* userp) {
                    
                    size_t totalSize = size * nmemb;
                    
                    std::ofstream *outFile = static_cast<std::ofstream*>(userp);
                    if (outFile && outFile->is_open()) {
                        outFile->write(static_cast<char*>(contents), totalSize);

                        if (outFile->bad()) {
                            return 0;
                        }
                    } else {
                        std::cerr << "WriteCallback: OutFile Doesn't Exist Or Can't Open!" << std::endl;
                    }
                    
                    return totalSize;
} 

void Download(CURLcode result, std::string targetPath, std::string savePath) {
    if (targetPath == "" || savePath == "") {
        std::cerr << "File Path Error: " << std::endl
                  << "targetPath: " << targetPath
                  << "savePath: " << savePath << std::endl;
        return;
    }

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, targetPath.c_str());
        
        std::ofstream saveFile(savePath, std::ios::binary | std::ios::trunc);
        if (!saveFile) {
            std::cerr << "SaveFile Creating Failed!" << std::endl;
        }
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &saveFile);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); 
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

        result = curl_easy_perform(curl);
        if (result == CURLE_OK) {
            // Get the HTTP response code
            long responseCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
            if (responseCode == 200) {
                // Print the response data
                std::cout << "Response from " << targetPath << std::endl
                          << "Response data, please click: " << savePath << std::endl;
            } else {
                std::cerr << "Response error code: " << responseCode << std::endl;
            }
        } else {
            std::cerr << "curl_easy_perform(curl) failed: " 
                      << curl_easy_strerror(result) << std::endl;
        }

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Download: curl_easy_init() failed" << std::endl;
    }
}

int main() {
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    if (result != CURLE_OK) {
        std::cerr << "Curl_global_init Failed: "
                  << curl_easy_strerror(result) << std::endl;
        return static_cast<int>(result);
    }

    std::string targetPath = "https://github.com";
    std::string savePath = "D:/MyFiles/UniversityFiles/CareerInformation/cpp-parallel-downloader/output/singledownloader.txt";
    Download(result, targetPath, savePath);

    curl_global_cleanup();

    return 0;
}