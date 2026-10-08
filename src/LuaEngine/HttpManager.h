#ifndef YLA_HTTP_MANAGER_H
#define YLA_HTTP_MANAGER_H

#include <regex>

#include "YLAEventMgr.h"
#include "libs/httplib.h"
#include "libs/rigtorp/SPSCQueue.h"

struct HttpWorkItem
{
public:
    HttpWorkItem(int funcRef, const YlaStateRef& owner, uint64 gen, const std::string& httpVerb, const std::string& url, const std::string& body, const std::string &contentType, const httplib::Headers& headers);

    int funcRef;
    // Owning-state identity + registry generation: the callback must run
    // on the state whose registry owns funcRef, never blindly on GALE.
    YlaStateRef owner;
    uint64 gen = 0;
    std::string httpVerb;
    std::string url;
    std::string body;
    std::string contentType;
    httplib::Headers headers;
};

struct HttpResponse
{
public:
    HttpResponse(int funcRef, const YlaStateRef& owner, uint64 gen, int statusCode, const std::string& body, const httplib::Headers& headers);

    int funcRef;
    YlaStateRef owner;
    uint64 gen = 0;
    int statusCode;
    std::string body;
    httplib::Headers headers;
};


class HttpManager
{
public:
    HttpManager();
    ~HttpManager();

    void StartHttpWorker();
    void StopHttpWorker();
    void PushRequest(HttpWorkItem* item);
    // Runs queued responses for the given owner state (held alive by the
    // caller). Stale-generation responses (CloseLua/reload raced the
    // worker) are dropped, never run on the new registry.
    void HandleHttpResponses(class YLA* owner, bool isGlobal);
    // Drops queued work/responses (reload path, before CloseLua).
    void DropPending();

private:
    void ClearQueues();
    void HttpWorkerThread();
    bool ParseUrl(const std::string& url, std::string& host, std::string& path);
    httplib::Result DoRequest(httplib::Client& client, HttpWorkItem* req, const std::string& path);

    rigtorp::SPSCQueue<HttpWorkItem*> workQueue;
    rigtorp::SPSCQueue<HttpResponse*> responseQueue;
    std::thread workerThread;
    bool startedWorkerThread;
    std::atomic_bool cancelationToken;
    std::condition_variable condVar;
    std::mutex condVarMutex;
    // The SPSC queues assume a single producer; Lua runs on many threads
    // (one per map worker in multistate, N workers on GALE in compat), so
    // every push/pop is serialized here.
    std::mutex queueMutex;
    std::regex parseUrlRegex;
};

#endif // #ifndef YLA_HTTP_MANAGER_H
