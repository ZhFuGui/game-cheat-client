#pragma once
#include "application/json.hpp"
#include <Windows.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
namespace mousebox::desktop {
class Transport {
    std::mutex mutex_;
    std::condition_variable_any wake_;
    std::deque<std::string> outgoing_;
    std::size_t bytes_=0;
    bool inFlight_=false;
    std::jthread reader_,writer_;
    std::atomic<bool> closed_=false;
    std::function<void(application::Json)> receive_;
    std::function<void()> close_;
    void failed();
public:
    Transport(std::function<void(application::Json)>,std::function<void()>);
    ~Transport();
    bool send(const application::Json&);
    void flush(unsigned timeoutMs=1000);
    bool closed()const{return closed_;}
};
}
