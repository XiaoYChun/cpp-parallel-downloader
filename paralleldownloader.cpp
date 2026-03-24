/*
    * paralleldownloader.cpp
    *
    * parallel downloader!
*/
#include <curl/curl.h>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <thread>


class CurlEasyHandle {
public:
    CurlEasyHandle() {
        curl = curl_easy_init();
    }
    ~CurlEasyHandle() {
        if (curl) curl_easy_cleanup(curl);
    }

     // delete copy
    CurlEasyHandle(const CurlEasyHandle&) = delete;
    CurlEasyHandle& operator=(const CurlEasyHandle&) = delete;

    // move
    CurlEasyHandle(CurlEasyHandle&& other) noexcept {
        this->curl = other.curl;
        other.curl = nullptr;
    }
    CurlEasyHandle& operator=(CurlEasyHandle&& other) noexcept {
        // self move check
        if (this != &other) {
            if (this->curl) {
                curl_easy_cleanup(curl);
            }
            this->curl = other.curl;
            other.curl = nullptr;
        }

        return *this;
    }

    CURL* GetCurl() {
        return curl;
    }

private:
    CURL *curl;
};

typedef struct FileInfo {
    std::fstream *outFile;
    long long offset;
    long long fileSize;
} FileInfo;

typedef struct CurlOpt {
    std::string targetURL;
    std::string savePath;
    FileInfo saveFileInfo;
    int rangeSize;
    int totalCount;
} CurlOpt;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, 
                    void* userp) {  
    size_t totalSize = size * nmemb;
    
    FileInfo *saveFileInfo = static_cast<FileInfo*>(userp);
    if (saveFileInfo && saveFileInfo->outFile && saveFileInfo->outFile->is_open()) {
        std::fstream *outFile = saveFileInfo->outFile;
        outFile->seekp(saveFileInfo->offset, std::ios::beg);
        outFile->write(static_cast<char*>(contents), totalSize);
        if (!outFile) {
            std::cerr << "[WriteCallback] File Writed Failed!" << std::endl;
            return 0;
        }
    } else {
        std::cerr << "[WriteCallback] File Operations Failed!" << std::endl;
        return 0;
    }
    
    std::cout << "[WriteCallback] Success to write into file from " << saveFileInfo->offset
             << " to " << saveFileInfo->offset + totalSize - 1 << "!" << std::endl;
    return totalSize;
} 

long long GetRemoteFileSize(const std::string& targetURL) {
    CurlEasyHandle curlHandle;
    // CURLINFO_CONTENT_LENGTH_DOWNLOAD needs double
    double fileSize = 0;
    
    if (curlHandle.GetCurl()) {
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_URL, targetURL.c_str());

        curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_FOLLOWLOCATION, 1L);

        if (curl_easy_perform(curlHandle.GetCurl()) == CURLE_OK) {
            curl_easy_getinfo(curlHandle.GetCurl(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fileSize);
            if (fileSize == -1) {
                std::cerr << "[GetRemoteFileSize] File Size == -1!" << std::endl;
                return -1;
            }
        } else {
            std::cerr << "[GetRemoteFileSize] curl_easy_perform Failed!" << std::endl;
            return -1;
        }
    } else {
        std::cerr << "[GetRemoteFileSize] curl_init_perform Failed!" << std::endl;
        return -1;
    }

    return static_cast<long long>(fileSize);
}

bool ThreadDownload(CurlOpt& curlOpt, std::atomic<int>& taskCount, std::atomic<bool>& errorFlag) {
    if (curlOpt.targetURL.empty() || curlOpt.savePath.empty()) {
        std::cerr << "[RangeDownload] TargetURL Or Save Path Is Empty!" << std::endl;
        errorFlag = true;
        return false;
    }

    // dynamic get task, can flag the startOffset
    int currentTask;
    // each thread have own fstream
    std::fstream threadFile(curlOpt.savePath, std::ios::binary | std::ios::out | std::ios::in);
    if (!threadFile || !threadFile.is_open()) {
        std::cerr << "[RangeDownload] Thread File Stream Error!" << std::endl;
        errorFlag = true;
        return false;
    }
    FileInfo threadFileInfo{&threadFile, 0, curlOpt.saveFileInfo.fileSize};
    while ((currentTask = taskCount.fetch_add(1)) < curlOpt.totalCount) {
        CurlEasyHandle curlHandle;
        if (curlHandle.GetCurl()) {
            long long startOffset = currentTask * curlOpt.rangeSize;
            long long endOffset = std::min<long long>(
                static_cast<long long>(startOffset + curlOpt.rangeSize - 1),
                static_cast<long long>(curlOpt.saveFileInfo.fileSize - 1)
            );
            threadFileInfo.offset = startOffset;      // startOffset change by the currentTask
            
            std::string rangeStr = std::to_string(startOffset) + "-" 
                                    + std::to_string(endOffset);
            curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_RANGE, rangeStr.c_str());

            curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_WRITEDATA, &threadFileInfo);
            curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_WRITEFUNCTION, WriteCallback);

            curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_URL, curlOpt.targetURL.c_str());            
            curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_FOLLOWLOCATION, 1L); 
            curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_MAXREDIRS, 5L);

            char errorBuffer[CURL_ERROR_SIZE];
            curl_easy_setopt(curlHandle.GetCurl(), CURLOPT_ERRORBUFFER, errorBuffer);

            CURLcode result = curl_easy_perform(curlHandle.GetCurl());
            if (result == CURLE_OK) {
                long responseCode = 0;
                curl_easy_getinfo(curlHandle.GetCurl(), CURLINFO_RESPONSE_CODE, &responseCode);
                if (responseCode != 206) {
                    std::cerr << "[RangeDownload] Response Code Error! responseCode: " << responseCode << std::endl;
                    errorFlag = true;
                    return false;
                } else {
                    std::cout << "[RangeDownload] Success to download from " 
                              << startOffset << " to " << endOffset << "!" << std::endl;
                }
            } else {
                std::cerr << "[RangeDownload] curl_easy_perform Failed! Now Try To Download From " 
                          << startOffset << " to " << endOffset << "!" << std::endl
                          << "Libcurl Error Details: " << errorBuffer << std::endl;
                errorFlag = true;
                return false;
            }
        } else {
            std::cerr << "[RangeDownload] curl_easy_init Failed!" << std::endl;
            errorFlag = true;
            return false;
        }
    } 

    return true;
}

int main() {
    CURLcode globalResult = curl_global_init(CURL_GLOBAL_ALL);
    if (globalResult != CURLE_OK) {
        std::cerr << "Curl_global_init Failed: "
                  << curl_easy_strerror(globalResult) << std::endl;
        return static_cast<int>(globalResult);
    }

    std::string targetURL = "https://www.baidu.com/robots.txt";
    std::string savePath = "D:/MyFiles/UniversityFiles/CareerInformation/cpp-parallel-downloader/output/paralleldownloader.txt";
    std::fstream saveFile(savePath, std::ios::binary | std::ios::trunc | std::ios::out | std::ios::in);
    if (!saveFile || !saveFile.is_open()) {
        curl_global_cleanup();
        return -1;
    }

    long long fileSize = GetRemoteFileSize(targetURL);
    if (fileSize <= 0) {
        std::cerr << "[Main] Remote File Size Error!" << std::endl;
        curl_global_cleanup();
        return -1;
    }
    if (saveFile.is_open()) {
        saveFile.seekp(fileSize - 1, std::ios::beg);
        saveFile.write("\0", 1);
        saveFile.seekp(0, std::ios::beg);
    } else {
        std::cerr << "[Main] File Open Failed!" << std::endl;
        curl_global_cleanup();
        return -1;
    }
    FileInfo saveFileInfo{&saveFile, 0, fileSize};

    int rangeSize = 256;
    int totalCount = (fileSize + rangeSize - 1) / rangeSize;
    CurlOpt curlOpt{targetURL, savePath, saveFileInfo, rangeSize, totalCount};
    
    std::vector<std::thread> threads;
    std::atomic<int> taskCount{0};      // the symbol of total task count
    int threadsNum = 8;
    std::atomic<bool> errorFlag{false};
    for (int i = 0; i < threadsNum; i++) {
        threads.emplace_back(ThreadDownload, std::ref(curlOpt), std::ref(taskCount), std::ref(errorFlag));
    }
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    if (errorFlag) {
        std::cerr << "[Main] Some Thread Error!" << std::endl;
    }

    curl_global_cleanup();

    return 0;
}