#pragma once
#include "coop/CombatProtocol.h"
namespace orebound::mechanism {
enum class Kind:unsigned {Switch=1,Mover=2};
enum class Op:unsigned {State=52,Activate=53};
struct State {
    Op op=Op::State;Kind kind=Kind::Switch;
    unsigned revision=0,request=0,state=0,frames=0;
    float frame=0,direction=0;std::array<float,3> anchor{};
    std::string name,target;
};
inline bool Valid(const State& s){
    if((s.op!=Op::State&&s.op!=Op::Activate)||(s.kind!=Kind::Switch&&s.kind!=Kind::Mover))return false;
    if(s.name.size()>63||s.target.size()>63||s.name.find('\0')!=std::string::npos||s.target.find('\0')!=std::string::npos)return false;
    for(const auto* text:{&s.name,&s.target})for(unsigned char c:*text)if(c<32||c>126)return false;
    for(float v:s.anchor)if(!std::isfinite(v)||std::abs(v)>1000000)return false;
    if(!std::isfinite(s.direction)||std::abs(s.direction)>1||!std::isfinite(s.frame)||s.frames>4096||s.frame<0)return false;
    if(s.kind==Kind::Switch){if(s.target.empty()||s.state>1||s.frames||s.frame!=0||s.direction!=0)return false;}
    else if(s.op!=Op::State||s.name.empty()||s.state>3||s.frames<2||s.frame>static_cast<float>(s.frames-1))return false;
    return s.op==Op::Activate?(s.request!=0&&s.revision==0):(s.revision!=0&&s.request==0);
}
inline combat::Bytes Encode(const State& s){
    combat::Writer w;w.data.push_back(static_cast<unsigned char>(s.op));w.u(static_cast<unsigned>(s.kind));
    w.u(s.revision);w.u(s.request);w.u(s.state);w.u(s.frames);w.f(s.frame);w.f(s.direction);
    for(float v:s.anchor)w.f(v);w.text(s.name);w.text(s.target);return w.data;
}
inline bool Decode(const combat::Bytes& b,State& s){
    if(b.empty()||(b[0]!=52&&b[0]!=53))return false;
    combat::Reader r{b};s.op=static_cast<Op>(b[0]);s.kind=static_cast<Kind>(r.u());s.revision=r.u();s.request=r.u();
    s.state=r.u();s.frames=r.u();s.frame=r.f();s.direction=r.f();for(float& v:s.anchor)v=r.f();s.name=r.text();s.target=r.text();
    return r.done()&&Valid(s);
}
inline bool Within(const std::array<float,3>& a,const std::array<float,3>& b,float range){
    float d=0;for(int i=0;i<3;++i){const auto v=a[i]-b[i];d+=v*v;}
    return std::isfinite(d)&&range>=0&&d<=range*range;
}
inline bool CanActivate(bool ready,bool occupied,bool fresh,const State& request,const std::array<float,3>& peer){
    return ready&&!occupied&&fresh&&request.op==Op::Activate&&Valid(request)&&Within(request.anchor,peer,500);
}
inline bool ApplyNewer(unsigned revision,unsigned incoming){return incoming&&(!revision||combat::Newer(incoming,revision));}
enum class OpenGate {Wait,Open,Cancel};
inline OpenGate AfterUnlock(int decision,bool timedOut,bool interacting,bool locked){
    if(timedOut||!interacting||decision>1)return OpenGate::Cancel;
    if(decision==0)return OpenGate::Wait;
    return locked?OpenGate::Cancel:OpenGate::Open;
}
}
