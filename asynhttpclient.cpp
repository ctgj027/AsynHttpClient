#include <iostream>
#include "asynhttpclient.h"
#include "rgserver.h"
#include <chrono>

AsynHttpClient::AsynHttpClient() : isRunning(true){
    curl_global_init(CURL_GLOBAL_ALL);
    multiHandle = curl_multi_init();
    if (!multiHandle){
        throw std::runtime_error("Failed to initialize CURL multi handle");
    }
}

AsynHttpClient::~AsynHttpClient() {
    stopRequestThread();
}

void AsynHttpClient::startRequestThread() {
    requestThread = std::thread(&AsynHttpClient::performRequests, this);
}

AsynHttpClient::stopRequestThread() {
    isRunning = false;
    if (requestThread.joinable()) {
        requestThread.join();
    }

    for (auto& pair : curlRequestHandles) {
        curl_easy_cleanup(pair.first);
        delete pair.second;
    }

    for (auto& pair : curlResponseHandles) {
        curl_easy_cleanup(pair.first);
        delete pair.second;
    }
    curl_multi_cleanup(multiHandle);
    curl_global_cleanup();
}

int AsynHttpClient::addGetRequest(const std::string& url, CTLHttpRequest* &httprequest) {
    std::lock_guard<std::mutex> lock(curl_handles_mutex); // 确保线程安全
    CURL* curl = curl_easy_init();
    if (!curl) {
        return CURL_ADD_FAIL;
    } else {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, httprequest);
        curl_handles.insert({curl, httprequest});
        curl_multi_add_handle(multiHandle, curl);
    }
    return CURL_ADD_SUCCESS;
}

int AsynHttpClient::addPostRequest(const std::string& url, const std::string& postData, CTLHttpRequest* httprequest, CHttpRetCallBack* httpRetCallBack) {
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        return CURL_ADD_FAIL;
    } else {
        CallbackData* callbackdata = new CallbackData();
        callbackdata->httprequest = httprequest;
        callbackdata->httpRetCallBack = httpRetCallBack;
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, TRUE);
        curl_easy_setopt(curl, (CURLoption)INET_OPTION_CONNECTTIMEOUT, DEFAULT_TIMEOUT/1000);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, DEFAULT_TIMEOUT/1000); //linux区分执行超时和连接超时，这里增加执行超时，超时时间通过连接超时设置
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.length());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, callbackdata);
        std::lock_guard<std::mutex> lock(curlHandlesMutex); // 确保线程安全
        curlRequestHandles.insert({curl, callbackdata});
    }
    return CURL_ADD_SUCCESS;
}

void AsynHttpClient::performRequests() {
    while (isRunning) {
        CURLMAP tmp;
        {
            std::lock_guard<std::mutex> lock(curlHandlesMutex); // 确保线程安全
            tmp = curlRequestHandles;
            curlRequestHandles.clear();
        }

        for (auto it = tmp.begin(); it != tmp.end(); it++){
            if (it->first) {
                curl_multi_add_handle(multiHandle, it->first);
                curlResponseHandles.insert({it->first, it->second});
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        //baratol:TL_Sleep(30);
        int runningHandles;
		while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(multiHandle, &running_handles)) {
			//cout << runningHandles << endl;
		}

        int numfds;
        if (running_handles > 0) {
			if (CURLM_OK != curl_multi_wait(multiHandle, NULL, 0, 1000, &numfds)) {
				continue;
			}
		}
        
        // 检查完成的请求
        CURLMsg *msg;
        int msgsLeft;
        while ((msg = curl_multi_info_read(multiHandle, &msgsLeft))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL* easyHandle = msg->easy_handle;
                char* url;
                curl_easy_getinfo(easyHandle, CURLINFO_EFFECTIVE_URL, &url);
                CallbackData* callbackdata = curlResponseHandles[easyHandle];
                callbackdata->httpRetCallBack->OnReceiveData(callbackdata->httprequest, NULL, 0);// 请求数据全部接收
                curl_multi_remove_handle(multiHandle, easyHandle);
                callbackdata->httpRetCallBack->OnFinish(callbackdata->httprequest);
                delete callbackdata;
                curlResponseHandles.erase(easyHandle);  // 从映射中移除句柄
                curl_easy_cleanup(easyHandle);
            }
        }
    }
}

size_t AsynHttpClient::writeCallback(void* contents, size_t size, size_t nmemb, void* stream) {
    CallbackData* callbackdata = (CallbackData*)stream;
    callbackdata->httpRetCallBack->OnReceiveData(callbackdata->httprequest, (const char*)contents, size*nmemb);
    return size * nmemb;
}
