// Basic script-free target switches and named keyframe movers only. Scripted
// workstations, inventory locks, teleporters and quest triggers are separate.
namespace mech=mechanism;
struct MechanismBinding {
    zCVob* vob=nullptr;mech::State desired;unsigned revision=0;
    unsigned long long lastSent=0;bool mirror=false;
};
std::vector<std::unique_ptr<MechanismBinding>> mechanismBindings;
std::map<zCVob*,MechanismBinding*> mechanismByPointer;
unsigned mechanismRevision=0,mechanismRequest=0,mechanismLastRequest=0;
unsigned long long mechanismScan=0,mechanismFrameAt=0;
bool mechanismReplaying=false;
std::set<std::string> mechanismTargetNames;
std::map<std::string,std::vector<zCMover*>> mechanismMoverTargets;
std::map<std::string,unsigned> mechanismTargetCounts;
std::string mechanismRegistrySummary;
void MechanismClearTargets(){for(auto& targetPair:mechanismMoverTargets)for(auto* mover:targetPair.second)mover->Release();mechanismMoverTargets.clear();mechanismTargetCounts.clear();}
std::vector<zCVob*> MechanismWorldObjects(){
    std::vector<zCVob*> objects;
    if(!ogame||!ogame->GetGameWorld())return objects;
    auto* root=&ogame->GetGameWorld()->globalVobTree;
    for(auto* node=root->GetFirstChild();node;){
        if(auto* vob=node->GetData())objects.push_back(vob);
        if(node->GetFirstChild()){node=node->GetFirstChild();continue;}
        while(node!=root&&!node->GetNextChild())node=node->GetParent();
        node=node==root?nullptr:node->GetNextChild();
    }
    return objects;
}
bool MechanismMoverShape(zCMover* mover){
    return mover&&mover->GetObjectName().Length()>0&&mover->GetObjectName().Length()<=63&&
        mover->moverAniType==zCMover::MA_KEYFRAME&&
        (mover->moverBehavior==zCMover::MB_2STATE_TOGGLE||mover->moverBehavior==zCMover::MB_2STATE_TRIGGER_CONTROL)&&
        mover->keyframeList.GetNumInList()>=2&&mover->keyframeList.GetNumInList()<=4096;
}
bool MechanismSwitch(oCMobInter* mob){
    if(!mob||mob->triggerTarget.Length()==0||mob->triggerTarget.Length()>63||mob->onStateFuncName.Length()!=0||
       mob->conditionFunc.Length()!=0||mob->useWithItem.Length()!=0||mob->state_num!=1||zDYNAMIC_CAST<oCMobLockable>(mob))return false;
    // Only direct native gate targets. Do not execute unrelated quest, teleport
    // or workstation trigger chains remotely under a guessed actor context.
    const auto found=mechanismMoverTargets.find(mob->triggerTarget.ToChar());
    const auto count=mechanismTargetCounts.find(mob->triggerTarget.ToChar());
    return found!=mechanismMoverTargets.end()&&found->second.size()==1&&count!=mechanismTargetCounts.end()&&count->second==1&&MechanismMoverShape(found->second.front());
}
bool MechanismMover(zCMover* mover){return MechanismMoverShape(mover)&&mechanismTargetNames.count(mover->GetObjectName().ToChar())!=0;}
mech::State MechanismCapture(zCVob* vob){
    mech::State s;s.name=vob->GetObjectName().ToChar();auto p=vob->GetPositionWorld();
    if(auto* mover=zDYNAMIC_CAST<zCMover>(vob)){
        s.kind=mech::Kind::Mover;s.state=static_cast<unsigned>(mover->moverState);
        s.frames=mover->keyframeList.GetNumInList();s.frame=mover->actKeyframeF;s.direction=mover->advanceDir;p=mover->keyframeList[0].pos;
    }else if(auto* mob=zDYNAMIC_CAST<oCMobInter>(vob)){
        s.kind=mech::Kind::Switch;s.target=mob->triggerTarget.ToChar();s.state=static_cast<unsigned>((std::max)(0,(std::min)(1,mob->state)));
    }
    s.anchor={p[0],p[1],p[2]};return s;
}
void MechanismReset(){
    MechanismClearTargets();mechanismRegistrySummary.clear();
    for(auto& item:mechanismBindings)item->vob->Release();
    mechanismTargetNames.clear();mechanismBindings.clear();mechanismByPointer.clear();mechanismRevision=mechanismRequest=mechanismLastRequest=0;
    mechanismScan=mechanismFrameAt=0;mechanismReplaying=false;
}
void MechanismScan(){
    if(!ogame||!ogame->GetGameWorld())return;
    // oCWorld::voblist excludes zCMover: enumerate the complete zCVob tree.
    const auto objects=MechanismWorldObjects();MechanismClearTargets();mechanismTargetNames.clear();
    unsigned movers=0,switches=0;
    for(auto* vob:objects)if(auto* mover=zDYNAMIC_CAST<zCMover>(vob)){
        if(!MechanismMoverShape(mover))continue;
        if(++movers>4096){combatFailure="Mechanism target limit reached";return;}
        mover->AddRef();mechanismMoverTargets[mover->GetObjectName().ToChar()].push_back(mover);
    }
    // A same-name script trigger or unsupported mover makes the target chain
    // ambiguous even if there is only one otherwise supported mover.
    for(auto* vob:objects)if(mechanismMoverTargets.count(vob->GetObjectName().ToChar()))++mechanismTargetCounts[vob->GetObjectName().ToChar()];
    for(auto* vob:objects){
        auto* mob=zDYNAMIC_CAST<oCMobInter>(vob);if(MechanismSwitch(mob)){mechanismTargetNames.insert(mob->triggerTarget.ToChar());++switches;}
    }
    for(auto* vob:objects){
        if(!vob||mechanismByPointer.count(vob))continue;
        auto* mover=zDYNAMIC_CAST<zCMover>(vob);auto* mob=zDYNAMIC_CAST<oCMobInter>(vob);
        if(!MechanismMover(mover)&&!MechanismSwitch(mob))continue;
        if(mechanismBindings.size()>=4096){combatFailure="Mechanism registry limit reached";return;}
        auto entry=std::make_unique<MechanismBinding>();entry->vob=vob;vob->AddRef();entry->desired=MechanismCapture(vob);
        mechanismByPointer[vob]=entry.get();mechanismBindings.push_back(std::move(entry));
        LootLog("mechanism_registered","name="+std::string(vob->GetObjectName().ToChar())+" target="+(mob?std::string(mob->triggerTarget.ToChar()):std::string())+" kind="+(mover?"mover":"switch"));
    }
    const auto summary="objects="+std::to_string(objects.size())+" movers="+std::to_string(movers)+" switches="+std::to_string(switches)+" bindings="+std::to_string(mechanismBindings.size());
    if(summary!=mechanismRegistrySummary){mechanismRegistrySummary=summary;LootLog("mechanism_registry",summary);}
}
MechanismBinding* MechanismFind(const mech::State& s){
    MechanismBinding* match=nullptr;
    for(auto& e:mechanismBindings){
        const auto identity=MechanismCapture(e->vob);
        if(identity.kind!=s.kind||identity.name!=s.name||identity.target!=s.target||!mech::Within(identity.anchor,s.anchor,5))continue;
        if(match){LootLog("mechanism_ambiguous","name="+s.name+" target="+s.target);return nullptr;}match=e.get();
    }
    return match;
}
void MechanismSend(MechanismBinding& entry){
    auto s=MechanismCapture(entry.vob);s.revision=++mechanismRevision;
    if(!mech::Valid(s)){LootLog("mechanism_invalid_state","name="+s.name);return;}
    QueueWorld(mech::Encode(s));entry.desired=s;entry.lastSent=Now();
}
bool MechanismMirror(zCMover* mover){
    if(mechanismReplaying||!CombatConnected()||HostCombat()||!MechanismMover(mover))return false;
    return true; // Host owns movement/timers even before the first baseline.
}
bool MechanismTrigger(oCMobInter* mob,zCVob* sender,zCVob* instigator,bool on){
    if(mechanismReplaying||!CombatConnected()||!MechanismSwitch(mob)||sender!=mob)return false;
    if(HostCombat())return false;
    if(instigator!=player)return true;
    if(!recoveryReady||saving||network.PeerSaving())return true;
    auto s=MechanismCapture(mob);s.op=mech::Op::Activate;s.state=on?1:0;s.request=++mechanismRequest;
    if(!mech::Valid(s)){combatFailure="Invalid switch activation";return true;}
    QueueWorld(mech::Encode(s));
    LootLog("mechanism_request","request="+std::to_string(s.request)+" target="+s.target+" state="+std::to_string(s.state));
    return true; // Native local animation remains; trigger chain runs on host.
}
void MechanismApply(MechanismBinding& entry,const mech::State& s,bool initial){
    if(s.kind==mech::Kind::Mover){
        auto* mover=zDYNAMIC_CAST<zCMover>(entry.vob);
        if(!MechanismMover(mover)||static_cast<unsigned>(mover->keyframeList.GetNumInList())!=s.frames)return;
        if(initial){mechanismReplaying=true;mover->SetToKeyframe_KF(s.frame);mechanismReplaying=false;}
        mover->advanceDir=s.direction;mover->moverState=static_cast<zCMover::zTMoverState>(s.state);
    }else{
        auto* mob=zDYNAMIC_CAST<oCMobInter>(entry.vob);
        if(!mob||mob->IsOccupied()||(player&&player->interactMob==mob))return;
        if(mob->state!=static_cast<int>(s.state)){
            mob->state=mob->state_target=s.state;
            if(auto* model=zDYNAMIC_CAST<zCModel>(mob->GetVisual())){
                const std::string name="S_S"+std::to_string(s.state);const int id=model->GetAniIDFromAniName(zSTRING(name.c_str()));
                if(id>=0)model->StartAni(id,FALSE);
            }
        }
    }
}
void MechanismReceive(const std::vector<std::uint8_t>& bytes){
    mech::State s;if(!mech::Decode(bytes,s)){LootLog("mechanism_reject","reason=codec");return;}
    if(mechanismBindings.empty())MechanismScan();auto* entry=MechanismFind(s);if(!entry){LootLog("mechanism_reject","reason=unmapped target="+s.target+" name="+s.name);return;}
    if(HostCombat()){
        if(s.op!=mech::Op::Activate)return;
        if(mechanismLastRequest&&!co::Newer(s.request,mechanismLastRequest)){MechanismSend(*entry);return;}
        auto* mob=zDYNAMIC_CAST<oCMobInter>(entry->vob);
        const bool fresh=haveTarget&&timing::Elapsed(Now(),lastSnapshotAt)<=1000;
        const std::array<float,3> peer={target.x,target.y,target.z};
        if(!MechanismSwitch(mob)||!mech::CanActivate(havePeerCombat&&PartnerNpc()&&recoveryReady&&rulesReady&&!saving&&!network.PeerSaving(),mob->IsOccupied()!=FALSE,fresh,s,peer)){
            LootLog("mechanism_denied","request="+std::to_string(s.request)+" target="+s.target);MechanismSend(*entry);return;
        }
        mechanismLastRequest=s.request;
        if(mob->state!=static_cast<int>(s.state)){
            mechanismReplaying=true;mob->state=mob->state_target=s.state;
            if(s.state)mob->OnTrigger(mob,PartnerNpc());else mob->OnUntrigger(mob,PartnerNpc());
            mechanismReplaying=false;
        }
        MechanismSend(*entry);
        LootLog("mechanism_applied","request="+std::to_string(s.request)+" target="+s.target+" state="+std::to_string(s.state));return;
    }
    if(s.op!=mech::Op::State||!mech::ApplyNewer(entry->revision,s.revision))return;
    const bool initial=!entry->mirror;entry->desired=s;entry->revision=s.revision;entry->mirror=true;
    MechanismApply(*entry,s,initial);
    if(initial||s.kind==mech::Kind::Switch)LootLog("mechanism_state","target="+s.target+" name="+s.name+" state="+std::to_string(s.state)+" baseline="+std::to_string(initial));
}
void MechanismFrame(unsigned long long now){
    if(!CombatConnected())return;
    const float dt=(std::min)(0.25f,static_cast<float>(timing::Elapsed(now,mechanismFrameAt))/1000.0f);mechanismFrameAt=now;
    if(timing::Elapsed(now,mechanismScan)>=1000){mechanismScan=now;MechanismScan();}
    unsigned sent=0;
    for(auto& entry:mechanismBindings){
        if(!entry->vob->homeWorld||entry->vob->homeWorld!=ogame->GetWorld())continue;
        if(HostCombat()){
            if(sent>=16)continue;
            const auto s=MechanismCapture(entry->vob);
            const bool changed=s.state!=entry->desired.state||std::abs(s.frame-entry->desired.frame)>0.005f;
            const bool peerNearby=haveTarget&&mech::Within(s.anchor,{target.x,target.y,target.z},10000);
            if(!entry->lastSent||(changed&&timing::Elapsed(now,entry->lastSent)>=100)||(peerNearby&&timing::Elapsed(now,entry->lastSent)>=2000)){
                MechanismSend(*entry);++sent;
            }
        }else if(entry->mirror){
            MechanismApply(*entry,entry->desired,false);
            if(entry->desired.kind==mech::Kind::Mover){
                auto* mover=zDYNAMIC_CAST<zCMover>(entry->vob);const float wanted=entry->desired.frame;
                const float next=std::abs(wanted-mover->actKeyframeF)<0.01f?wanted:mover->actKeyframeF+(wanted-mover->actKeyframeF)*(1-std::exp(-dt*20));
                if(std::abs(next-mover->actKeyframeF)>0.0001f){mechanismReplaying=true;mover->SetToKeyframe_KF(next);mechanismReplaying=false;}
            }
        }
    }
}
