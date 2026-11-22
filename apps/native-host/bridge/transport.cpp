#include "transport.hpp"
#include <array>
#include <chrono>
namespace mousebox::desktop {
namespace {
constexpr std::uint32_t maximum=8*1024*1024;
bool readAll(HANDLE handle,void* value,DWORD size){auto data=static_cast<char*>(value);while(size){DWORD read=0;if(!ReadFile(handle,data,size,&read,nullptr)||!read)return false;data+=read;size-=read;}return true;}
bool writeAll(HANDLE handle,const void* value,DWORD size){auto data=static_cast<const char*>(value);while(size){DWORD written=0;if(!WriteFile(handle,data,size,&written,nullptr)||!written)return false;data+=written;size-=written;}return true;}
}
Transport::Transport(std::function<void(application::Json)> receive,std::function<void()> close):receive_(std::move(receive)),close_(std::move(close)){
    reader_=std::jthread([this](std::stop_token token){
        try{while(!token.stop_requested()){
            std::array<unsigned char,4> header{};if(!readAll(GetStdHandle(STD_INPUT_HANDLE),header.data(),4))break;
            const auto size=static_cast<std::uint32_t>(header[0])|(static_cast<std::uint32_t>(header[1])<<8)|(static_cast<std::uint32_t>(header[2])<<16)|(static_cast<std::uint32_t>(header[3])<<24);
            if(!size||size>maximum)break;
            std::string body(size,'\0');if(!readAll(GetStdHandle(STD_INPUT_HANDLE),body.data(),size))break;
            receive_(application::Json::parse(body));
        }}catch(const std::exception&){}failed();
    });
    writer_=std::jthread([this](std::stop_token token){
        while(!token.stop_requested()){
            std::string frame;
            {std::unique_lock lock(mutex_);wake_.wait(lock,token,[this]{return !outgoing_.empty()||closed_;});if(outgoing_.empty())break;frame=std::move(outgoing_.front());outgoing_.pop_front();bytes_-=frame.size();inFlight_=true;}
            const bool written=writeAll(GetStdHandle(STD_OUTPUT_HANDLE),frame.data(),static_cast<DWORD>(frame.size()));
            {const std::lock_guard lock(mutex_);inFlight_=false;}wake_.notify_all();
            if(!written){failed();break;}
        }
    });
}
Transport::~Transport(){
    reader_.request_stop();writer_.request_stop();wake_.notify_all();
    const auto finish=[](std::jthread& thread){
        if(!thread.joinable())return;
        // Cancellation can race the small gap before the next synchronous
        // ReadFile/WriteFile. Repeat until the OS thread has actually exited.
        while(WaitForSingleObject(thread.native_handle(),20)!=WAIT_OBJECT_0)CancelSynchronousIo(thread.native_handle());
        thread.join();
    };
    finish(reader_);finish(writer_);
}
void Transport::failed(){if(!closed_.exchange(true)&&close_)close_();wake_.notify_all();}
bool Transport::send(const application::Json& value){
    auto body=value.dump();if(body.empty()||body.size()>maximum){failed();return false;}
    const auto size=static_cast<std::uint32_t>(body.size());std::string frame(4,'\0');for(unsigned i=0;i<4;++i)frame[i]=static_cast<char>((size>>(i*8))&255);frame+=body;
    {const std::lock_guard lock(mutex_);if(closed_)return false;if(outgoing_.size()>=256||bytes_+frame.size()>16*1024*1024){closed_=true;}else{bytes_+=frame.size();outgoing_.push_back(std::move(frame));wake_.notify_one();return true;}}
    if(close_)close_();wake_.notify_all();return false;
}
void Transport::flush(unsigned timeoutMs){
    const auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(timeoutMs);
    while(std::chrono::steady_clock::now()<until&&!closed_){
        {const std::lock_guard lock(mutex_);if(outgoing_.empty()&&!inFlight_)return;}
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
}
