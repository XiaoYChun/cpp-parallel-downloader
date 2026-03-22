/*
    * rangedownloader.cpp
    *
    * Example of using libcurl to range download
    * things in output files.
*/
#include <curl/curl.h>

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

/*
    * two important things
    * first, range GET
    * second, file seekp write
*/

enum class ResponseCode {
    Success,
    FileError,
    CurlOptError,
    RemoteCodeError,
    CurlPerformError,
    CurlInitError,
    RemoteFileSizeError
};

// 对于结构体，内部是否应该保存指针比较好
// 因为实际上在初始化的时候，很容易浪费空间，即 copy 带来额外消耗

// need to maintain an offset
// for seeking and writing at specific positions
typedef struct FileInfo {
    std::fstream *outFile;
    long long offset;
}FileInfo;

typedef struct CurlOpt {
    std::string targetURL;
    std::string savePath;
    FileInfo saveFileInfo;
    int rangeSize;
}CurlOpt;

void OutputInfo(const ResponseCode& responseCode) {
    switch (responseCode) {
        case ResponseCode::Success:
            std::cout << "Operations Success!" << std::endl;
            break;
        case ResponseCode::FileError:
            std::cout << "File Operations Error!" << std::endl;
            break;
        case ResponseCode::CurlOptError:
            std::cout << "CurlOpt Operations Error!" << std::endl;
            break;
        case ResponseCode::RemoteCodeError:
            std::cout << "Remote Response Code Error!" << std::endl;
            break;
        case ResponseCode::CurlPerformError:
            std::cout << "Curl Perform Error!" << std::endl;
            break;
        case ResponseCode::CurlInitError:
            std::cout << "Curl Init Error!" << std::endl;
            break;
        case ResponseCode::RemoteFileSizeError:
            std::cout << "Remote File Size Error!" << std::endl;
            break;
        
        
        default:
            std::cerr << "Output Info Default!" << std::endl;
            break;
    }
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, 
                    void* userp) {  
    size_t totalSize = size * nmemb;
    
    FileInfo *saveFileInfo = static_cast<FileInfo*>(userp);
    if (saveFileInfo && saveFileInfo->outFile && saveFileInfo->outFile->is_open()) {
        std::fstream *outFile = saveFileInfo->outFile;
        outFile->seekp(saveFileInfo->offset, std::ios::beg);
        outFile->write(static_cast<char*>(contents), totalSize);
    } else {
        std::cerr << "[WriteCallback] File Operations Failed!" << std::endl;
        return 0;
    }
    
    return totalSize;
} 

// CURL type is void 
// typedef void CURL
ResponseCode CurlOptSet(CURL* curl, const CurlOpt& curlOpt) {
    curl_easy_setopt(curl, CURLOPT_URL, curlOpt.targetURL.c_str());
        
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlOpt.saveFileInfo);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); 
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

    return ResponseCode::Success;
}

long long GetRemoteFileSize(const std::string& targetURL) {
    CURL *curl = curl_easy_init();
    // CURLINFO_CONTENT_LENGTH_DOWNLOAD needs double
    double fileSize = 0;
    
    if (curl) {
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, targetURL.c_str());

        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        if (curl_easy_perform(curl) == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fileSize);
            if (fileSize == -1) {
                curl_easy_cleanup(curl);
                OutputInfo(ResponseCode::RemoteFileSizeError);
                std::cerr << "[GetRemoteFileSize] File Size == -1!" << std::endl;
                return -1;
            }
        } else {
            curl_easy_cleanup(curl);
            OutputInfo(ResponseCode::CurlPerformError);
            std::cerr << "[GetRemoteFileSize] curl_easy_perform Failed!" << std::endl;
            return -1;
        }

        curl_easy_cleanup(curl);
    } else {
        OutputInfo(ResponseCode::CurlInitError);
        std::cerr << "[GetRemoteFileSize] curl_init_perform Failed!" << std::endl;
    }

    return static_cast<long long>(fileSize);
}

ResponseCode RangeDownload(CurlOpt& curlOpt) {
    if (curlOpt.targetURL.empty() || curlOpt.savePath.empty()) {
        std::cerr << "[RangeDownload] TargetURL Or Save Path Is Empty!" << std::endl;
        return ResponseCode::CurlOptError;
    }

    CURL *curl = curl_easy_init();
    if (curl) {
        CurlOptSet(curl, curlOpt);

        long long fileSize = GetRemoteFileSize(curlOpt.targetURL);
        // for seekp
        if (curlOpt.saveFileInfo.outFile->is_open()) {
            curlOpt.saveFileInfo.outFile->seekp(fileSize - 1, std::ios::beg);
            curlOpt.saveFileInfo.outFile->write("\0", 1);
            curlOpt.saveFileInfo.outFile->seekp(0, std::ios::beg);
        } else {
            curl_easy_cleanup(curl);
            std::cerr << "[RangeDownload] File Open Failed!" << std::endl;
            return ResponseCode::FileError;
        }

        long long startOffset = 0;
        while (startOffset < fileSize) {
            curlOpt.saveFileInfo.offset = startOffset;
            long long endOffset = std::min<long long>(
                static_cast<long long>(startOffset + curlOpt.rangeSize - 1),
                static_cast<long long>(fileSize - 1)
            );
            
            std::string rangeStr = std::to_string(startOffset) + "-" 
                                   + std::to_string(endOffset);
            curl_easy_setopt(curl, CURLOPT_RANGE, rangeStr.c_str());

            CURLcode result = curl_easy_perform(curl);
            if (result == CURLE_OK) {
                long responseCode = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
                if (responseCode != 206) {
                    curl_easy_cleanup(curl);
                    std::cerr << "[RangeDownload] Response Code Error! responseCode: " << responseCode << std::endl;
                    return ResponseCode::RemoteCodeError;
                } 
            } else {
                curl_easy_cleanup(curl);
                std::cerr << "[RangeDownload] curl_easy_perform Failed!" << std::endl;
                return ResponseCode::CurlPerformError;
            }
        
            startOffset = endOffset + 1;
        }
        
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "[RangeDownload] curl_easy_init Failed!" << std::endl;
        return ResponseCode::CurlInitError;
    }

    return ResponseCode::Success;
}

int main() {
    CURLcode globalResult = curl_global_init(CURL_GLOBAL_ALL);
    if (globalResult != CURLE_OK) {
        std::cerr << "Curl_global_init Failed: "
                  << curl_easy_strerror(globalResult) << std::endl;
        return static_cast<int>(globalResult);
    }

    std::string targetURL = "https://www.baidu.com/robots.txt";
    std::string savePath = "D:/MyFiles/UniversityFiles/CareerInformation/cpp-parallel-downloader/output/rangedownloader.txt";
    std::fstream saveFile(savePath, std::ios::binary | std::ios::trunc | std::ios::out | std::ios::in);
    if (!saveFile || !saveFile.is_open()) {
        return -1;
    }
    FileInfo saveFileInfo{&saveFile, 0};
    int rangeSize = 1024;
    CurlOpt curlOpt{targetURL, savePath, saveFileInfo, rangeSize};
    
    OutputInfo(RangeDownload(curlOpt));

    curl_global_cleanup();

    return 0;
}