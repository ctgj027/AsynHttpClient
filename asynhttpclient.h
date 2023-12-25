#ifndef ASYN_HTTP_CLIENT_H
#define ASYN_HTTP_CLIENT_H
#include <string>
#include <curl/curl.h>
#include <map>
#include <thread>
#include <mutex>

class CHttpRetCallBack;
class CTLHttpRequest;

/**
 * @class AsynHttpClient
 * @brief HTTP Client for handling asynchronous HTTP requests.
 *
 * This class provides functionality for sending asynchronous HTTP GET and POST requests
 * using libcurl. It handles request creation, sending and response handling in a separate thread.
 */

class AsynHttpClient {
public:
    /**
     * @brief Constructor for AsynHttpClient.
     *
     * Initializes the libcurl environment and sets up necessary resources.
     */
    AsynHttpClient();

    /**
     * @brief Destructor for AsynHttpClient.
     *
     * Cleans up resources and ensures proper shutdown of the HTTP client.
     */
    ~AsynHttpClient();

    /**
     * @brief Starts the thread to handle HTTP requests.
     */
    void startRequestThread();

    /**
     * @brief Stops the thread to handle HTTP requests.
     */
    void stopRequestThread();

    /**
     * @brief Adds a GET request to the client.
     * 
     * @param url The URL for the GET request.
     * @param httprequest A pointer to the CTLHttpRequest object containing request details.
     * @return int The return value indicates the result of adding the request.
     */
    int addGetRequest(const std::string& url, CTLHttpRequest* &httprequest);

    /**
     * @brief Adds a POST request to the client.
     * 
     * @param url The URL for the POST request.
     * @param postData The data to be posted.
     * @param httprequest A pointer to the CTLHttpRequest object containing request details.
     * @param httpRetCallBack A pointer to the CHttpRetCallBack object for handling response callbacks.
     * @return int The return value indicates the result of adding the request.
     */
    int addPostRequest(const std::string& url, const std::string& postData, CTLHttpRequest* httprequest, CHttpRetCallBack* httpRetCallBack);

    

private:
    void performRequests();
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

    struct CallbackData
    {
        CTLHttpRequest* httprequest;
        CHttpRetCallBack* httpRetCallBack;
    };

    CURLM* multiHandle;                                // 用于管理多个并发的 CURL 请求
    typedef std::map<CURL*, CallbackData*> CurlMap;    // 用于将 CURL 句柄关联到回调数据
    CurlMap curlRequestHandles;                        // 存储待处理的 CURL 请求及其关联的回调数据
    CurlMap curlResponseHandles;                       // 存储已发送的 CURL 请求及其关联的回调数据，等待处理响应
    std::mutex curlHandlesMutex;                       // 用于保护对 curlRequestHandles 和 curlResponseHandles 的访问，确保线程安全
    std::thread requestThread;                         // 用于处理 HTTP 请求的独立线程
    bool isRunning;                                    // 用于指示请求处理线程是否应继续运行
};
#endif // ASYN_HTTP_CLIENT_H
