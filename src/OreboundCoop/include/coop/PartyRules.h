#pragma once
#include "coop/Recovery.h"
#include <set>
namespace orebound::party {
enum class Faction:unsigned {Any=0,Militia=1,Mercenary=2,Monastery=3};
inline Faction Path(unsigned guild){switch(guild){case 1:case 2:return Faction::Militia;case 6:case 7:return Faction::Mercenary;case 4:case 5:return Faction::Monastery;default:return Faction::Any;}}
inline bool Eligible(Faction required,unsigned guild){return required==Faction::Any||Path(guild)==required;}
inline bool ShareFactionQuest(Faction required,unsigned actorGuild,unsigned peerGuild){return Eligible(required,actorGuild)&&Eligible(required,peerGuild);}
inline bool CombatCredit(bool connected,bool sameWorld){return connected&&sameWorld;}
inline unsigned VictoryXP(unsigned level,unsigned perVictory){return level&&perVictory&&level<=1000000/perVictory?level*perVictory:0;}
struct VictoryBefore {bool eligible=false,credited=false;unsigned hostXP=0,proxyXP=0;int hostHP=0,hostMaxHP=0,hostLevel=0;unsigned hostLP=0,hostNextXP=0;bool hostUnconscious=false,hostDead=false;};
enum class Claim {Granted,Duplicate,Busy,Conflict};
struct Decision {std::string token,owner;unsigned revision=0;bool committed=false;};
class Decisions {
    std::map<std::string,Decision> rows_;
public:
    Claim Acquire(const std::string& scope,const std::string& token,const std::string& owner,unsigned revision){
        if(scope.empty()||scope.size()>63||!recovery::Guid(token)||!recovery::Guid(owner)||!revision)return Claim::Conflict;
        const auto old=rows_.find(scope);
        if(old!=rows_.end()){
            if(old->second.token==token&&old->second.owner==owner)return Claim::Duplicate;
            return old->second.committed?Claim::Conflict:Claim::Busy;
        }
        if(rows_.size()>=8192)return Claim::Busy;
        rows_[scope]={token,owner,revision,false};return Claim::Granted;
    }
    bool Commit(const std::string& scope,const std::string& token){auto it=rows_.find(scope);if(it==rows_.end()||it->second.token!=token)return false;it->second.committed=true;return true;}
    bool Abort(const std::string& scope,const std::string& token){auto it=rows_.find(scope);if(it==rows_.end()||it->second.token!=token||it->second.committed)return false;rows_.erase(it);return true;}
    const std::map<std::string,Decision>& Rows()const{return rows_;}
};
enum class Condition:unsigned {Alive,Unconscious,Dead};
inline Condition Life(bool dead,bool unconscious){return unconscious?Condition::Unconscious:dead?Condition::Dead:Condition::Alive;}
inline bool CanApplyXP(Condition condition){return condition!=Condition::Dead;}
enum class Defeat:unsigned {Playing,OneDead,BothDead};
inline Defeat DefeatState(Condition local,Condition peer){const unsigned dead=(local==Condition::Dead)+(peer==Condition::Dead);return dead==2?Defeat::BothDead:dead?Defeat::OneDead:Defeat::Playing;}
struct CommonSave {std::string id,hostProfile,guestProfile,hostFingerprint,guestFingerprint;int hostSlot=-1,guestSlot=-1;std::string world;};
inline bool Valid(const CommonSave& s){return recovery::Guid(s.id)&&recovery::Guid(s.hostProfile)&&recovery::Guid(s.guestProfile)&&s.hostSlot>=0&&s.hostSlot<=20&&s.guestSlot>=0&&s.guestSlot<=20&&!s.world.empty()&&s.world.size()<=63&&!s.hostFingerprint.empty()&&s.hostFingerprint.size()<=63&&!s.guestFingerprint.empty()&&s.guestFingerprint.size()<=63;}
inline bool CanResume(const CommonSave& save,bool transferPending,bool fingerprintsMatch,bool bothAccepted){return Valid(save)&&!transferPending&&fingerprintsMatch&&bothAccepted;}
inline bool UniqueConsumption(const recovery::Tx& t){return t.action==recovery::Action::UniqueConsume&&recovery::Valid(t);}
inline std::string RebranchSpawnName(const std::string& name,const std::string& owner,const std::string& oldTag,const std::string& nextTag){
    if((owner!="ZKSH_"&&owner!="ZKSG_")||oldTag.empty()||nextTag.empty()||name.rfind(owner+oldTag,0)!=0)return name;
    const auto changed=owner+nextTag+name.substr(owner.size()+oldTag.size());return changed.size()<63?changed:name;
}
inline bool PartyReward(const recovery::Tx& t){
    if(UniqueConsumption(t))return true;
    if(t.source.kind!=loot::Source::Quest||t.source.name.rfind("ORB_XP_",0)!=0||t.source.name.size()!=23)return false;
    for(std::size_t i=7;i<t.source.name.size();++i)if(!((t.source.name[i]>='0'&&t.source.name[i]<='9')||(t.source.name[i]>='a'&&t.source.name[i]<='f')))return false;
    return static_cast<unsigned>(t.action)==10&&t.instance>0&&t.instance<1000000&&t.item==0&&t.amount>0&&t.amount<=1000000&&t.currency==0&&t.price==0;
}
}
