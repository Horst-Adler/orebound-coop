#pragma once
#include "coop/PartyRules.h"
namespace orebound::party {
// Only explicitly verified native quest adapters may execute these messages.
enum class QuestOp:unsigned {Request,Result,Notice};
struct QuestMessage {QuestOp op=QuestOp::Request;std::string token,host,owner;unsigned info=0;bool accepted=false;std::vector<recovery::Tx> rewards;};
inline bool Valid(const QuestMessage& m){if(static_cast<unsigned>(m.op)>2||!recovery::Guid(m.token)||!recovery::Guid(m.host)||!recovery::Guid(m.owner)||m.info>=11||m.rewards.size()>4)return false;std::set<std::string> ids,events;for(const auto& t:m.rewards)if(!ids.insert(t.id).second||!events.insert(t.source.name+":"+std::to_string(t.instance)).second||!recovery::Valid(t)||(t.source.name!="CAVALORN_KILLBRAGO"&&!UniqueConsumption(t))||t.hostTimeline!=m.host||t.ownerTimeline!=m.owner||t.world!="NEWWORLD.ZEN"||t.decision!=recovery::Decision::Commit)return false;if(m.op==QuestOp::Notice)return m.accepted&&m.rewards.empty();if(m.op==QuestOp::Request)return !m.accepted&&m.rewards.empty();return m.accepted||m.rewards.empty();}
inline combat::Bytes Encode(const QuestMessage& m){combat::Writer w;w.data.push_back(82);w.u(static_cast<unsigned>(m.op));w.text(m.token);w.text(m.host);w.text(m.owner);w.u(m.info);w.u(m.accepted);w.u(static_cast<unsigned>(m.rewards.size()));for(const auto& t:m.rewards)recovery::Write(w,t);return w.data;}
inline bool Decode(const combat::Bytes& b,QuestMessage& m){if(b.empty()||b[0]!=82||b.size()>net::MaximumDatagramBytes-net::ProtocolHeaderBytes)return false;combat::Reader r{b};m={};m.op=static_cast<QuestOp>(r.u());m.token=r.text();m.host=r.text();m.owner=r.text();m.info=r.u();const auto yes=r.u(),n=r.u();if(yes>1||n>4)return false;m.accepted=yes!=0;for(unsigned i=0;i<n;++i)m.rewards.push_back(recovery::Read(r));return r.done()&&Valid(m);}
inline bool SharedDecisionInfo(unsigned info){return info==4||info==5||info==6||info==7||info==8;}
inline std::string DecisionScope(unsigned info){return info==7||info==8?"cavalorn.turnin":"cavalorn.info."+std::to_string(info);}
inline combat::Bytes EncodeDecisions(const Decisions& decisions){combat::Writer w;w.data.push_back(1);w.u(static_cast<unsigned>(decisions.Rows().size()));for(const auto& row:decisions.Rows()){w.text(row.first);w.text(row.second.token);w.text(row.second.owner);w.u(row.second.revision);w.u(row.second.committed);}return recovery::Seal(w);}
inline Decisions DecodeDecisions(const combat::Bytes& bytes){const auto b=recovery::Unseal(bytes);if(b.empty()||b[0]!=1)throw std::runtime_error("Unsupported quest decisions");combat::Reader r{b};const auto n=r.u();if(n>8192)throw std::runtime_error("Too many quest decisions");Decisions out;for(unsigned i=0;i<n;++i){const auto scope=r.text(),token=r.text(),owner=r.text();const auto rev=r.u(),commit=r.u();if(commit>1||out.Acquire(scope,token,owner,rev)!=Claim::Granted)throw std::runtime_error("Invalid/duplicate quest decision");if(commit)out.Commit(scope,token);}if(!r.done())throw std::runtime_error("Invalid quest decision tail");return out;}
inline bool UniqueResource(unsigned flags,const std::string& name){return (flags&4096)!=0||name.rfind("ITKE_",0)==0;}
struct Resource {unsigned instance=0,amount=0;};
struct Resources {std::string profile,timeline,world;unsigned revision=0,page=0,pages=1;std::vector<Resource> items;};
inline bool Valid(const Resources& m){if(!recovery::Guid(m.profile)||!recovery::Guid(m.timeline)||m.world.empty()||m.world.size()>63||!m.revision||!m.pages||m.pages>128||m.page>=m.pages||m.items.size()>24)return false;std::set<unsigned> seen;for(const auto& i:m.items)if(!i.instance||i.instance>=1000000||!i.amount||i.amount>1000000||!seen.insert(i.instance).second)return false;return true;}
inline combat::Bytes Encode(const Resources& m){combat::Writer w;w.data.push_back(81);w.text(m.profile);w.text(m.timeline);w.text(m.world);w.u(m.revision);w.u(m.page);w.u(m.pages);w.u(static_cast<unsigned>(m.items.size()));for(const auto& i:m.items){w.u(i.instance);w.u(i.amount);}return w.data;}
inline bool Decode(const combat::Bytes& b,Resources& m){if(b.empty()||b[0]!=81)return false;combat::Reader r{b};m={};m.profile=r.text();m.timeline=r.text();m.world=r.text();m.revision=r.u();m.page=r.u();m.pages=r.u();const auto n=r.u();if(n>24)return false;for(unsigned i=0;i<n;++i)m.items.push_back({r.u(),r.u()});return r.done()&&Valid(m);}
class ResourcePages {
    std::string profile_,timeline_,world_;unsigned revision_=0,pages_=0;std::map<unsigned,std::vector<Resource>> chunks_;
public:
    bool Add(const Resources& m){if(!Valid(m))return false;if(m.profile!=profile_||m.timeline!=timeline_||m.world!=world_||combat::Newer(m.revision,revision_)){profile_=m.profile;timeline_=m.timeline;world_=m.world;revision_=m.revision;pages_=m.pages;chunks_.clear();}if(m.revision!=revision_||m.pages!=pages_)return false;const auto old=chunks_.find(m.page);if(old!=chunks_.end()){if(old->second.size()!=m.items.size())return false;for(std::size_t i=0;i<m.items.size();++i)if(old->second[i].instance!=m.items[i].instance||old->second[i].amount!=m.items[i].amount)return false;}chunks_[m.page]=m.items;return true;}
    unsigned Revision()const{return revision_;}
    bool Complete()const{return pages_&&chunks_.size()==pages_;}
    std::map<unsigned,unsigned> Stock()const{std::map<unsigned,unsigned> out;if(!Complete())return out;for(const auto& c:chunks_)for(const auto& i:c.second){if(!out.emplace(i.instance,i.amount).second)throw std::runtime_error("Duplicate unique item across pages");}return out;}
};
}
