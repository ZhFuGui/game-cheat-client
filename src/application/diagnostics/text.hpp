#pragma once
#include "application/json.hpp"
#include <sstream>
#include <iomanip>
#include <string>
namespace mousebox::application {
// Small UTF-8 formatting value used by the shared diagnostic descriptions.
class Text {
    std::string value_;
public:
    Text()=default;
    Text(const char* value):value_(value){}
    Text(std::string value):value_(std::move(value)){}
    const std::string& str() const noexcept{return value_;}
    bool operator==(const char* value)const{return value_==value;}
    Text& operator+=(const Text& other){value_+=other.value_;return *this;}
    friend Text operator+(Text a,const Text& b){return a+=b;}
    static Text number(double value,char format='f',int precision=2){std::ostringstream out;if(format=='f')out<<std::fixed;out<<std::setprecision(precision)<<value;return out.str();}
    static Text fromStdString(const std::string& value){return value;}
    static Text fromUtf8(const char* value,std::size_t size){return std::string(value,size);}
    template<class T> Text arg(const T& value) const {std::ostringstream out;if constexpr(std::is_same_v<T,Text>)out<<value.str();else out<<value;auto result=value_;unsigned lowest=100;for(unsigned i=1;i<100;++i){const auto token="%"+std::to_string(i);if(result.find(token)!=std::string::npos){lowest=i;break;}}if(lowest==100)return result;const auto token="%"+std::to_string(lowest);std::size_t pos=0;const auto replacement=out.str();while((pos=result.find(token,pos))!=std::string::npos){const auto end=pos+token.size();if(end<result.size()&&result[end]>='0'&&result[end]<='9'){pos=end;continue;}result.replace(pos,token.size(),replacement);pos+=replacement.size();}return result;}
    template<class T,class... Rest>Text arg(const T& value,const Rest&... rest) const{return arg(value).arg(rest...);}
    friend void to_json(Json& j,const Text& text){j=text.value_;}
};
}
