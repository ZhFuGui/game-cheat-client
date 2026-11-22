#include "mouse_reports.hpp"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace mousebox::application {
namespace {
Json quantile(const std::vector<double>& sorted,double fraction){
    if(sorted.empty())return nullptr;
    return sorted[static_cast<std::size_t>(std::ceil(fraction*static_cast<double>(sorted.size())))-1];
}
Json histogram(const std::vector<double>& values,bool integral){
    Json result=Json::array();if(values.empty())return result;
    const auto [lo,hi]=std::minmax_element(values.begin(),values.end());
    const double low=integral?std::floor(*lo)-0.5:*lo;
    const double span=integral?std::floor(*hi)-std::floor(*lo)+1:*hi-*lo;
    const auto bins=integral?static_cast<std::size_t>(std::min(64.0,span)):(span>0?std::size_t{64}:std::size_t{1});
    const double width=span>0?span/static_cast<double>(bins):1;
    std::vector<std::size_t> counts(bins);
    for(const auto value:values){const auto bin=std::min(bins-1,static_cast<std::size_t>(std::max(0.0,(value-low)/width)));++counts[bin];}
    for(std::size_t i=0;i<bins;++i)result.push_back({{"low",low+static_cast<double>(i)*width},{"high",low+static_cast<double>(i+1)*width},{"count",counts[i]}});
    return result;
}
struct TimeBin {
    std::size_t count=0;
    double xMin=0,xMax=0,yMin=0,yMax=0,mMin=0,mMax=0,xSum=0,ySum=0,mSum=0;
    void add(double x,double y,double magnitude){
        if(!count){xMin=xMax=x;yMin=yMax=y;mMin=mMax=magnitude;}
        else{xMin=std::min(xMin,x);xMax=std::max(xMax,x);yMin=std::min(yMin,y);yMax=std::max(yMax,y);mMin=std::min(mMin,magnitude);mMax=std::max(mMax,magnitude);}
        ++count;xSum+=x;ySum+=y;mSum+=magnitude;
    }
};
}
Json mouseReportsProgressJson(const MouseReportsProgress& p){
    return {{"recording",p.recording},{"elapsedMs",static_cast<double>(p.durationUs)/1000},{"reports",p.reports},{"otherDeviceReports",p.otherDeviceReports},{"absoluteReports",p.absoluteReports},{"device",p.device},{"reason",p.reason}};
}
Json analyzeMouseReports(const MouseReportRecording& recording){
    if(recording.reports.size()>MaximumMouseReports)throw std::invalid_argument("鼠标报文数量超限");
    const auto& reports=recording.reports;
    const auto duration=std::max(recording.progress.durationUs,reports.empty()?std::uint64_t{0}:reports.back().us);
    const double durationMs=static_cast<double>(duration)/1000;
    const auto binCount=static_cast<std::size_t>(std::clamp(std::ceil(durationMs/5),1.0,512.0));
    std::vector<TimeBin> bins(binCount);
    std::vector<double> xs,ys,magnitudes,intervals;xs.reserve(reports.size());ys.reserve(reports.size());magnitudes.reserve(reports.size());intervals.reserve(reports.size());
    double sumX=0,sumY=0,path=0,maxX=0,maxY=0;std::size_t moving=0,zeroIntervals=0;
    std::uint64_t previous=0;
    for(std::size_t i=0;i<reports.size();++i){
        const auto& p=reports[i];if(i&&p.us<previous)throw std::invalid_argument("鼠标报文时间倒退");
        const double x=p.dx,y=p.dy,magnitude=std::hypot(x,y);
        xs.push_back(x);ys.push_back(y);magnitudes.push_back(magnitude);sumX+=x;sumY+=y;path+=magnitude;maxX=std::max(maxX,std::abs(x));maxY=std::max(maxY,std::abs(y));
        if(p.dx||p.dy)++moving;
        if(i){const auto interval=static_cast<double>(p.us-previous)/1000;intervals.push_back(interval);if(p.us==previous)++zeroIntervals;}
        previous=p.us;
        const auto bin=duration?std::min(binCount-1,static_cast<std::size_t>(static_cast<long double>(p.us)*binCount/duration)):0;
        bins[bin].add(x,y,magnitude);
    }
    const auto count=reports.size();
    auto magnitudeSorted=magnitudes;std::sort(magnitudeSorted.begin(),magnitudeSorted.end());auto intervalSorted=intervals;std::sort(intervalSorted.begin(),intervalSorted.end());
    Json summary={{"reports",count},{"movingReports",moving},{"zeroReports",count-moving},{"durationMs",durationMs},{"observedHz",duration?Json(static_cast<double>(count)*1000000/static_cast<double>(duration)):Json{}},
        {"sumX",sumX},{"sumY",sumY},{"pathCounts",path},{"meanMagnitude",count?Json(path/static_cast<double>(count)):Json{}},{"p50Magnitude",quantile(magnitudeSorted,0.5)},{"p95Magnitude",quantile(magnitudeSorted,0.95)},{"p99Magnitude",quantile(magnitudeSorted,0.99)},
        {"maxMagnitude",count?Json(magnitudeSorted.back()):Json{}},{"maxAbsX",maxX},{"maxAbsY",maxY},{"medianIntervalMs",quantile(intervalSorted,0.5)},{"p95IntervalMs",quantile(intervalSorted,0.95)},{"maxIntervalMs",intervals.empty()?Json{}:Json(intervalSorted.back())},{"zeroIntervals",zeroIntervals}};
    Json timeline=Json::array();
    for(std::size_t i=0;i<bins.size();++i){const auto& b=bins[i];const auto n=static_cast<double>(b.count);
        timeline.push_back({{"startMs",durationMs*static_cast<double>(i)/static_cast<double>(binCount)},{"endMs",durationMs*static_cast<double>(i+1)/static_cast<double>(binCount)},{"reports",b.count},
            {"xMin",b.count?Json(b.xMin):Json{}},{"xMax",b.count?Json(b.xMax):Json{}},{"xMean",b.count?Json(b.xSum/n):Json{}},
            {"yMin",b.count?Json(b.yMin):Json{}},{"yMax",b.count?Json(b.yMax):Json{}},{"yMean",b.count?Json(b.ySum/n):Json{}},
            {"magnitudeMin",b.count?Json(b.mMin):Json{}},{"magnitudeMax",b.count?Json(b.mMax):Json{}},{"magnitudeMean",b.count?Json(b.mSum/n):Json{}}});
    }
    return {{"schemaVersion",1},{"source","windows-raw-input"},{"device",recording.progress.device},{"stopReason",recording.progress.reason},{"otherDeviceReports",recording.progress.otherDeviceReports},{"absoluteReports",recording.progress.absoluteReports},
        {"summary",std::move(summary)},{"histograms",{{"x",histogram(xs,true)},{"y",histogram(ys,true)},{"magnitude",histogram(magnitudes,false)},{"interval",histogram(intervals,false)}}},{"timeline",std::move(timeline)}};
}
bool saveMouseReportsCsv(const std::filesystem::path& path,const MouseReportRecording& recording){
    std::error_code ec;if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path(),ec);if(ec)return false;
    const auto temp=std::filesystem::path(path.wstring()+L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    std::ofstream stream(temp,std::ios::binary|std::ios::trunc);if(!stream)return false;
    stream<<"t_us,dx,dy,flags,button_flags\r\n";
    for(const auto& p:recording.reports)stream<<p.us<<','<<p.dx<<','<<p.dy<<','<<p.flags<<','<<p.buttons<<"\r\n";
    stream.flush();const bool written=stream.good();stream.close();
    const bool saved=written&&MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
    if(!saved)DeleteFileW(temp.c_str());return saved;
}
}
