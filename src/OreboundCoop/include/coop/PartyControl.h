#pragma once
#include "coop/PartyRules.h"
namespace orebound::party {
enum class ControlOp:unsigned {Status,OfferCheckpoint,Accept,Decline,Checkpoint,Resume,Prepare,Ready,Commit,Committed,Go};
struct Saved {std::string profile,fingerprint,world;int slot=-1;};
inline bool Valid(const Saved& s){return recovery::Guid(s.profile)&&s.slot>=0&&s.slot<=20&&!s.fingerprint.empty()&&s.fingerprint.size()<=63&&!s.world.empty()&&s.world.size()<=63;}
struct Control {
    ControlOp op=ControlOp::Status;std::string profile,timeline,token,branchHost,branchGuest;
    Saved saved;CommonSave common;Condition life=Condition::Alive;
};
inline void WriteSaved(combat::Writer& w,const Saved& s){w.text(s.profile);w.i(s.slot);w.text(s.fingerprint);w.text(s.world);}
inline Saved ReadSaved(combat::Reader& r){Saved s;s.profile=r.text();s.slot=r.i();s.fingerprint=r.text();s.world=r.text();return s;}
inline void WriteCommon(combat::Writer& w,const CommonSave& s){w.text(s.id);w.text(s.hostProfile);w.text(s.guestProfile);w.i(s.hostSlot);w.i(s.guestSlot);w.text(s.hostFingerprint);w.text(s.guestFingerprint);w.text(s.world);}
inline CommonSave ReadCommon(combat::Reader& r){CommonSave s;s.id=r.text();s.hostProfile=r.text();s.guestProfile=r.text();s.hostSlot=r.i();s.guestSlot=r.i();s.hostFingerprint=r.text();s.guestFingerprint=r.text();s.world=r.text();return s;}
inline bool Same(const CommonSave& a,const CommonSave& b){return a.id==b.id&&a.hostProfile==b.hostProfile&&a.guestProfile==b.guestProfile&&a.hostSlot==b.hostSlot&&a.guestSlot==b.guestSlot&&a.hostFingerprint==b.hostFingerprint&&a.guestFingerprint==b.guestFingerprint&&a.world==b.world;}
inline bool Valid(const Control& m){
    if(static_cast<unsigned>(m.op)>10||!recovery::Guid(m.profile)||!recovery::Guid(m.timeline)||static_cast<unsigned>(m.life)>2)return false;
    if(m.op==ControlOp::Status)return m.saved.slot==-1||(Valid(m.saved)&&m.saved.profile==m.profile);
    if(!recovery::Guid(m.token)||!Valid(m.common))return false;
    const bool branch=m.op>=ControlOp::Prepare;
    return branch?recovery::Guid(m.branchHost)&&recovery::Guid(m.branchGuest)&&m.branchHost!=m.branchGuest:(m.branchHost.empty()&&m.branchGuest.empty());
}
inline bool FreshBranches(const Control& m,const std::string& own,const std::string& peer){return recovery::Guid(m.branchHost)&&recovery::Guid(m.branchGuest)&&m.branchHost!=m.branchGuest&&m.branchHost!=own&&m.branchGuest!=own&&m.branchHost!=peer&&m.branchGuest!=peer;}
inline combat::Bytes Encode(const Control& m){combat::Writer w;w.data.push_back(80);w.u(static_cast<unsigned>(m.op));w.text(m.profile);w.text(m.timeline);w.text(m.token);w.text(m.branchHost);w.text(m.branchGuest);WriteSaved(w,m.saved);WriteCommon(w,m.common);w.u(static_cast<unsigned>(m.life));return w.data;}
inline bool Decode(const combat::Bytes& b,Control& m){if(b.empty()||b[0]!=80)return false;combat::Reader r{b};m.op=static_cast<ControlOp>(r.u());m.profile=r.text();m.timeline=r.text();m.token=r.text();m.branchHost=r.text();m.branchGuest=r.text();m.saved=ReadSaved(r);m.common=ReadCommon(r);m.life=static_cast<Condition>(r.u());return r.done()&&Valid(m);}
struct Consent {
    std::string token;bool local=false,peer=false,prepared=false,committed=false;
    void Begin(const std::string& id,bool initiator){token=id;local=initiator;peer=false;prepared=committed=false;}
    bool Accept(const std::string& id,bool remote){if(id!=token||committed)return false;(remote?peer:local)=true;return true;}
    bool Both()const{return recovery::Guid(token)&&local&&peer;}
    bool Prepare(){if(!Both())return false;prepared=true;return true;}
    bool Commit(){if(!prepared||!Both())return false;committed=true;return true;}
    void Clear(){*this=Consent{};}
};
inline std::string LocalFingerprint(const CommonSave& s,bool host){return host?s.hostFingerprint:s.guestFingerprint;}
inline int LocalSlot(const CommonSave& s,bool host){return host?s.hostSlot:s.guestSlot;}
inline std::string LocalProfile(const CommonSave& s,bool host){return host?s.hostProfile:s.guestProfile;}
}
