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
 * @class HttpClient
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
     */
    void addGetRequest(const std::string& url, CTLHttpRequest* &httprequest);

    /**
     * @brief Adds a POST request to the client.
     * 
     * @param url The URL for the POST request.
     * @param postData The data to be posted.
     * @param httprequest A pointer to the CTLHttpRequest object containing request details.
     * @param httpRetCallBack A pointer to the CHttpRetCallBack object for handling response callbacks.
     */
    void addPostRequest(const std::string& url, const std::string& postData, CTLHttpRequest* httprequest, CHttpRetCallBack* httpRetCallBack);

    

private:
    void performRequests();
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

    struct CallbackData
    {
        CTLHttpRequest* httprequest;
        CHttpRetCallBack* httpRetCallBack;
    };

    CURLM* multiHandle;
    typedef std::map<CURL*, CallbackData*> CurlMap;
    CurlMap curlRequestHandles;
    CurlMap curlResponseHandles;
    std::mutex curlHandlesMutex; // 用于保护 curlHandles
    std::thread requestThread;
    bool isRunning;
};
#endif // ASYN_HTTP_CLIENT_H