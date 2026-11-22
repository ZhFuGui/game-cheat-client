#include "dispatch.hpp"
#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
int controllerEntry(int,wchar_t**);
int readerEntry(int,char**);
int replayEntry(int,char**);
namespace {
std::string utf8(const wchar_t* value) {
    const int count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value,-1,nullptr,0,nullptr,nullptr);
    if(count<=0) throw std::runtime_error("Invalid command-line Unicode");
    std::string result(static_cast<std::size_t>(count),'\0');
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value,-1,result.data(),count,nullptr,nullptr))
        throw std::runtime_error("Cannot encode command-line Unicode");
    result.pop_back();return result;
}
}
namespace native_cli {
std::optional<int> dispatch(int argc,wchar_t** argv) {
    if(argc>=2&&(std::wstring_view(argv[1])==L"--bridge"||std::wstring_view(argv[1])==L"bridge")) return std::nullopt;
    if(argc<2||std::wstring_view(argv[1])==L"--help"||std::wstring_view(argv[1])==L"-h") {
        std::cout<<"Mousebox native runtime\n  --bridge  framed desktop IPC\n  controller [options]  serial diagnostic controller\n  reader [options]  read-only Reader observer\n  replay [options]  offline evidence reconstruction\n";return 0;
    }
    const std::wstring command=argv[1];
    if(command==L"controller")return controllerEntry(argc-1,argv+1);
    if(command!=L"reader"&&command!=L"reader-client"&&command!=L"replay") {
        std::cerr<<"Unknown native subcommand. Use --help.\n";return 2;
    }
    std::vector<std::string> values;values.reserve(static_cast<std::size_t>(argc-1));
    for(int i=1;i<argc;++i)values.push_back(utf8(argv[i]));
    std::vector<char*> pointers;pointers.reserve(values.size()+1);
    for(auto& value:values)pointers.push_back(value.data());pointers.push_back(nullptr);
    return command==L"replay"?replayEntry(argc-1,pointers.data()):readerEntry(argc-1,pointers.data());
}
}
