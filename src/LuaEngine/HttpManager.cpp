#include <thread>
extern "C"
{
#include "lua.h"
#include "lauxlib.h"
};

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "libs/httplib.h"
#include "HttpManager.h"
#include "LuaEngine.h"

HttpWorkItem::HttpWorkItem(int funcRef, const YlaStateRef& owner, uint64 gen, const std::string& httpVerb, const std::string& url, const std::string& body, const std::string& contentType, const httplib::Headers& headers)
    : funcRef(funcRef),
    owner(owner),
    gen(gen),
    httpVerb(httpVerb),
    url(url),
    body(body),
    contentType(contentType),
    headers(headers)
{ }

HttpResponse::HttpResponse(int funcRef, const YlaStateRef& owner, uint64 gen, int statusCode, const std::string& body, const httplib::Headers& headers)
    : funcRef(funcRef),
    owner(owner),
    gen(gen),
    statusCode(statusCode),
    body(body),
    headers(headers)
{ }

HttpManager::HttpManager()
    : workQueue(16),
    responseQueue(16),
    startedWorkerThread(false),
    cancelationToken(false),
    condVar(),
    condVarMutex(),
    parseUrlRegex("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?")
{
    StartHttpWorker();
}

HttpManager::~HttpManager()
{
    StopHttpWorker();
}

void HttpManager::PushRequest(HttpWorkItem* item)
{
    {
        std::unique_lock<std::mutex> lock(condVarMutex);
        std::lock_guard<std::mutex> qlock(queueMutex);
        EnqueueRequest(item);
        condVar.notify_one();
    }
}

void HttpManager::DropPending()
{
    ClearQueues();
}

void HttpManager::StartHttpWorker()
{
    ClearQueues();

    if (!startedWorkerThread)
    {
        cancelationToken.store(false);
        workerThread = std::thread(&HttpManager::HttpWorkerThread, this);
        startedWorkerThread = true;
    }
}

void HttpManager::ClearQueues()
{
    std::lock_guard<std::mutex> qlock(queueMutex);
    while (workQueue.front())
    {
        HttpWorkItem* item = *workQueue.front();
        if (item != nullptr)
        {
            delete item;
        }
        workQueue.pop();
    }

    while (responseQueue.front())
    {
        HttpResponse* item = *responseQueue.front();
        if (item != nullptr)
        {
            delete item;
        }
        responseQueue.pop();
    }

    // Same no-unref rationale as the queues above: on the reload path
    // lua_close reclaims the whole registry right after.
    while (!overflowRequests.empty())
    {
        delete overflowRequests.front();
        overflowRequests.pop_front();
    }
    while (!overflowResponses.empty())
    {
        delete overflowResponses.front();
        overflowResponses.pop_front();
    }
}

void HttpManager::RefillQueues()
{
    while (!overflowRequests.empty() && workQueue.try_push(overflowRequests.front()))
        overflowRequests.pop_front();
    while (!overflowResponses.empty() && responseQueue.try_push(overflowResponses.front()))
        overflowResponses.pop_front();
}

void HttpManager::EnqueueRequest(HttpWorkItem* item)
{
    if (workQueue.try_push(item))
        return;
    if (overflowRequests.size() >= MaxOverflow)
    {
        YLA_LOG_ERROR("[YLA]: HTTP request overflow full, dropping oldest (funcRef leak until reload).");
        delete overflowRequests.front();
        overflowRequests.pop_front();
    }
    overflowRequests.push_back(item);
}

void HttpManager::EnqueueResponse(HttpResponse* res)
{
    if (responseQueue.try_push(res))
        return;
    if (overflowResponses.size() >= MaxOverflow)
    {
        YLA_LOG_ERROR("[YLA]: HTTP response overflow full, dropping oldest (funcRef leak until reload).");
        delete overflowResponses.front();
        overflowResponses.pop_front();
    }
    overflowResponses.push_back(res);
}

void HttpManager::FailRequest(HttpWorkItem* req)
{
    EnqueueResponse(new HttpResponse(req->funcRef, req->owner, req->gen, 0, "", httplib::Headers()));
}

void HttpManager::StopHttpWorker()
{
    if (!startedWorkerThread)
    {
        return;
    }

    cancelationToken.store(true);
    condVar.notify_one();
    workerThread.join();
    ClearQueues();
    startedWorkerThread = false;
}

void HttpManager::HttpWorkerThread()
{
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(condVarMutex);
            condVar.wait(lock, [&] {
                std::lock_guard<std::mutex> qlock(queueMutex);
                return workQueue.front() != nullptr || !overflowRequests.empty() || cancelationToken.load();
            });
        }

        if (cancelationToken.load())
        {
            break;
        }

        HttpWorkItem* req = nullptr;
        {
            std::lock_guard<std::mutex> qlock(queueMutex);
            RefillQueues();
            if (!workQueue.front())
                continue;
            req = *workQueue.front();
            workQueue.pop();
        }
        if (!req)
        {
            continue;
        }

        try
        {
            std::string host;
            std::string path;

            if (!ParseUrl(req->url, host, path)) {
                YLA_LOG_ERROR("[YLA]: Could not parse URL {}", req->url);
                {
                    std::lock_guard<std::mutex> qlock(queueMutex);
                    FailRequest(req);
                }
                delete req;
                continue;
            }

            httplib::Client cli(host);
            cli.set_connection_timeout(0, 3000000); // 3 seconds
            cli.set_read_timeout(5, 0); // 5 seconds
            cli.set_write_timeout(5, 0); // 5 seconds

            httplib::Result res = DoRequest(cli, req, path);
            httplib::Error err = res.error();
            if (err != httplib::Error::Success)
            {
                YLA_LOG_ERROR("[YLA]: HTTP request error: {}", httplib::to_string(err));
                {
                    std::lock_guard<std::mutex> qlock(queueMutex);
                    FailRequest(req);
                }
                delete req;
                continue;
            }

            if (res->status == 301)
            {
                std::string location = res->get_header_value("Location");
                std::string host;
                std::string path;

                if (!ParseUrl(location, host, path))
                {
                    YLA_LOG_ERROR("[YLA]: Could not parse URL after redirect: {}", location);
                    {
                        std::lock_guard<std::mutex> qlock(queueMutex);
                        FailRequest(req);
                    }
                    delete req;
                    continue;
                }
                httplib::Client cli2(host);
                cli2.set_connection_timeout(0, 3000000); // 3 seconds
                cli2.set_read_timeout(5, 0); // 5 seconds
                cli2.set_write_timeout(5, 0); // 5 seconds
                res = DoRequest(cli2, req, path);
            }

            {
                std::lock_guard<std::mutex> qlock(queueMutex);
                EnqueueResponse(new HttpResponse(req->funcRef, req->owner, req->gen, res->status, res->body, res->headers));
            }
        }
        catch (const std::exception& ex)
        {
            YLA_LOG_ERROR("[YLA]: HTTP request error: {}", ex.what());
            if (req)
            {
                std::lock_guard<std::mutex> qlock(queueMutex);
                FailRequest(req);
            }
        }

        delete req;
    }
}

httplib::Result HttpManager::DoRequest(httplib::Client& client, HttpWorkItem* req, const std::string& urlPath)
{
    const char* path = urlPath.c_str();
    if (req->httpVerb == "GET")
    {
        return client.Get(path, req->headers);
    }
    if (req->httpVerb == "HEAD")
    {
        return client.Head(path, req->headers);
    }
    if (req->httpVerb == "POST")
    {
        return client.Post(path, req->headers, req->body, req->contentType.c_str());
    }
    if (req->httpVerb == "PUT")
    {
        return client.Put(path, req->headers, req->body, req->contentType.c_str());
    }
    if (req->httpVerb == "PATCH")
    {
        return client.Patch(path, req->headers, req->body, req->contentType.c_str());
    }
    if (req->httpVerb == "DELETE")
    {
        return client.Delete(path, req->headers);
    }
    if (req->httpVerb == "OPTIONS")
    {
        return client.Options(path, req->headers);
    }

    YLA_LOG_ERROR("[YLA]: HTTP request error: invalid HTTP verb {}", req->httpVerb);
    return client.Get(path, req->headers);
}

bool HttpManager::ParseUrl(const std::string& url, std::string& host, std::string& path)
{
    std::smatch matches;

    if (!std::regex_search(url, matches, parseUrlRegex))
    {
        return false;
    }

    std::string scheme = matches[2];
    std::string authority = matches[4];
    std::string query = matches[7];
    host = scheme + "://" + authority;
    path = matches[5];
    if (path.empty())
    {
        path = "/";
    }
    path += (query.empty() ? "" : "?") + query;

    return true;
}

void HttpManager::HandleHttpResponses(YLA* owner, bool isGlobal)
{
    while (true)
    {
        HttpResponse* res = nullptr;
        {
            std::lock_guard<std::mutex> qlock(queueMutex);
            RefillQueues();
            if (responseQueue.empty())
                break;
            res = *responseQueue.front();
            responseQueue.pop();
        }

        if (res == nullptr)
        {
            continue;
        }

        LOCK_YLA;
        // The callback runs on the state that issued the request, under
        // global -> state order (this drain holds the same nesting).
        // Stale generations (reload recycled the registry while the
        // worker was in flight) are dropped, never unref'd on the new one.
        auto state = YLA::LockStateRef(res->owner);
        if (!state || state.get() != owner || state->luaGen.load(std::memory_order_acquire) != res->gen || !state->HasLuaState())
        {
            delete res;
            continue;
        }

        YLA::Guard stateGuard(state->GetStateLock());
        if (!state->HasLuaState())
        {
            delete res;
            continue;
        }
        (void)isGlobal;
        lua_State* L = state->L;

        // Get function
        lua_rawgeti(L, LUA_REGISTRYINDEX, res->funcRef);

        // Push parameters
        YLA::Push(L, res->statusCode);
        YLA::Push(L, res->body);
        lua_newtable(L);
        for (const auto& item : res->headers) {
            YLA::Push(L, item.first);
            YLA::Push(L, item.second);
            lua_settable(L, -3);
        }

        // Call function
        state->ExecuteCall(3, 0);

        luaL_unref(L, LUA_REGISTRYINDEX, res->funcRef);

        delete res;
    }
}
