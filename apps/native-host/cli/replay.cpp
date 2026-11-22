#include "adapters/sources/pubg_reader/evidence_loader.hpp"
#include "games/pubg/pipeline/target_selector.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
namespace {
void quoted(std::ostream& out,std::string_view text) {
    out<<'"';
    for(const unsigned char ch:text) {
        if(ch=='"'||ch=='\\') out<<'\\'<<static_cast<char>(ch);
        else if(ch<32) out<<"\\u00"<<std::hex<<std::setw(2)<<std::setfill('0')<<static_cast<unsigned>(ch)<<std::dec;
        else out<<static_cast<char>(ch);
    }
    out<<'"';
}
void header(std::ostream& out,const mousebox::control::Frame& f) {
    const auto& h=f.header;
    out<<"{\"reader\":";quoted(out,h.reader);
    out<<",\"session\":"<<h.session<<",\"epoch\":"<<h.epoch<<",\"revision\":"<<h.revision
        <<",\"capture\":"<<h.capture<<",\"source_start_ns\":"<<h.sourceStartNs
        <<",\"source_finish_ns\":"<<h.sourceFinishNs<<",\"root_unverified\":"<<(h.worldRootUnverified?"true":"false")
        <<",\"cache_used\":"<<(h.worldCacheUsed?"true":"false")<<'}';
}
}
int replayEntry(int argc,char** argv) {
    try {
        std::filesystem::path root,output;std::string living="Living-2",view="Local-1";
        for(int i=1;i<argc;++i) {
            const std::string key=argv[i];
            if(key=="--help") {std::cout<<"mousebox-native replay --evidence wire-and-state [--living Living-2] [--view Local-1] [--output report.json]\nOffline saved-record reconstruction only; no network or device output.\n";return 0;}
            if(i+1>=argc) throw std::invalid_argument("Option value missing");
            const std::string value=argv[++i];
            if(key=="--evidence") root=value;
            else if(key=="--living") living=value;
            else if(key=="--view") view=value;
            else if(key=="--output") output=value;
            else throw std::invalid_argument("Unknown replay option");
        }
        if(root.empty()) throw std::invalid_argument("--evidence is required");
        const auto l=mousebox::reader::loadEvidence(root/living),v=mousebox::reader::loadEvidence(root/view);
        using namespace mousebox::control;
        const auto pair=checkPair(l.frame,v.frame);TargetSelector selector;
        const auto target=selector.select(l.frame,v.frame,pair);
        const auto motion=checkMotion(v.frame,pair,{},std::nullopt,false);
        std::ostringstream report;report<<std::setprecision(17)
            <<"{\n\"version\":\"0.3.0-offline\",\"record_set\":\"partial_saved_records\",\"serial_opened\":false,\"network_opened\":false,\n"
            <<"\"pair_reason\":\""<<reasonName(pair.reason)<<"\",\"source_skew_ms\":"<<pair.sourceSkewMs<<",\n"
            <<"\"motion_reason\":\""<<reasonName(motion)<<"\",\"age_upper_ms\":null,\"calibration_measured\":false,\n"
            <<"\"living\":";header(report,l.frame);report<<",\n\"view\":";header(report,v.frame);
        const auto heads=std::count_if(l.frame.characters.begin(),l.frame.characters.end(),[](const Character& c){return c.head&&c.headOrigin;});
        report<<",\n\"characters\":"<<l.frame.characters.size()<<",\"characters_with_head\":"<<heads<<",\"self_entity_id\":";
        if(l.frame.self) report<<l.frame.self->id;else report<<"null";
        report<<",\"selected\":";
        if(target) report<<"{\"entity_id\":"<<target->identity.id<<",\"horizontal_degrees\":"<<target->error.horizontal
            <<",\"vertical_degrees\":"<<target->error.vertical<<",\"cone_degrees\":"<<target->error.cone<<'}';
        else report<<"null";
        report<<"\n}\n";std::cout<<report.str();
        if(!output.empty()) {
            if(!output.parent_path().empty()) std::filesystem::create_directories(output.parent_path());
            std::ofstream file(output,std::ios::binary|std::ios::trunc);file<<report.str();
            if(!file) throw std::runtime_error("Cannot write replay report");
        }
        return 0; // A legitimate blocked pair/no candidate is a reported result, not corrupt evidence.
    } catch(const std::exception& error) {std::cerr<<"Replay rejected evidence: "<<error.what()<<'\n';return 1;}
}
