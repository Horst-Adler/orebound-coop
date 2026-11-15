pr::Saved partySaved,partyPeerSaved;
pr::CommonSave partyCommon;
pr::Control partyControl;
pr::Consent partyConsent;
std::deque<co::Bytes> partyControlOutbox;
bool partyLoadFailed=false;
bool partyResumeIntent=false,partyResumeCommitted=false,partyLoadActive=false,partyRejoin=false;
unsigned long long partyControlStarted=0,partyControlSent=0,partyLoadDue=0,partyRejoinDue=0;
pr::Condition partyPeerLife=pr::Condition::Alive;
pr::Defeat partyLastDefeat=pr::Defeat::Playing;
std::string partyControlText;
bool PartyCommonHost(const pr::CommonSave& s){return s.hostProfile==recoveryStore.Profile();}
std::string PartyControlFile(const char* name){return recoveryDir+"\\"+name;}
void PartyControlSend(pr::Control m){
    m.profile=recoveryStore.Profile();m.timeline=recoveryTimeline;
    if(m.op<pr::ControlOp::Prepare)m.branchHost=m.branchGuest="";
    if(!pr::Valid(m)||partyControlOutbox.size()>=64)throw std::runtime_error("Invalid/full party control queue");
    partyControlOutbox.push_back(pr::Encode(m));
}
void PartyPersistControl(pr::Control m,const char* file){
    m.profile=recoveryStore.Profile();m.timeline=recoveryTimeline;
    if(!pr::Valid(m))throw std::runtime_error("Invalid common checkpoint record");
    combat::Writer sealed;sealed.data=pr::Encode(m);rc::AtomicWrite(PartyControlFile(file),rc::Seal(sealed));
}
bool PartyReadControl(const char* file,pr::Control& m){
    const auto path=PartyControlFile(file);if(GetFileAttributesA(path.c_str())==INVALID_FILE_ATTRIBUTES)return false;
    return pr::Decode(rc::Unseal(rc::ReadFile(path)),m);
}
void PartyInitControl(){
    try{
        pr::Control saved;
        if(PartyReadControl("party-common.bin",saved)&&saved.op==pr::ControlOp::Checkpoint&&
           (saved.common.hostProfile==recoveryStore.Profile()||saved.common.guestProfile==recoveryStore.Profile()))partyCommon=saved.common;
        if(PartyReadControl("party-resume.bin",saved)&&saved.op==pr::ControlOp::Commit&&saved.profile==recoveryStore.Profile()){
            partyControl=saved;partyResumeIntent=partyResumeCommitted=true;partyConsent.Begin(saved.token,true);partyConsent.Accept(saved.token,true);partyConsent.Prepare();partyConsent.Commit();
            LootLog("party_resume_recovered","token="+saved.token+" awaiting_matching_session=1");
        }
    }catch(const std::exception& e){partyControlText=Ui("party.control_error");LootLog("party_control_error",e.what());}
}
void PartyControlReset(){
    partyPeerSaved=pr::Saved{};partyPeerLife=pr::Condition::Alive;partyControlOutbox.clear();partyControlSent=0;
    if(!partyResumeCommitted){partyConsent.Clear();partyControl=pr::Control{};partyResumeIntent=false;partyControlStarted=0;}
}
bool PartyControlSafe(){
    return recoveryStoreReady&&player&&!saving&&!network.PeerSaving()&&!clientPendingTake&&!clientOpenRequest&&!clientDeferredChest&&
        !clientLootShown&&!clientQuickLoot&&worldGroundPending.empty()&&!lootLedger.HasUnapplied()&&!CharacterBusy()&&!PartyQuestBusy()&&!PartyUniquePending();
}
std::string PartySaveFingerprint(int slot){
    const auto dir=RecoverySlot(slot);WIN32_FIND_DATAA item;HANDLE find=FindFirstFileA((dir+"*.SAV").c_str(),&item);
    if(find==INVALID_HANDLE_VALUE)throw std::runtime_error("Native checkpoint files missing");
    std::vector<std::string> names;
    do{if(!(item.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))names.emplace_back(item.cFileName);}while(FindNextFileA(find,&item));FindClose(find);
    if(names.empty()||names.size()>64)throw std::runtime_error("Native checkpoint file count invalid");
    for(const auto* sidecar:{"ZWEIKLANG_RECOVERY.BIN","OREBOUND_CHARACTER.BIN","ZWEIKLANG_CHARACTER.BIN","ZWEIKLANG_ACTORS.BIN"})if(GetFileAttributesA((dir+sidecar).c_str())!=INVALID_FILE_ATTRIBUTES)names.emplace_back(sidecar);
    std::sort(names.begin(),names.end());std::string signature;
    for(const auto& name:names)signature+=name+":"+extensions::FileSha256(dir+name,[]{network.Poll(Now());})+";";
    std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(16)<<net::HashString(signature);return out.str();
}
bool PartyValidateCommon(const pr::CommonSave& s){
    if(!pr::Valid(s)||(s.hostProfile!=recoveryStore.Profile()&&s.guestProfile!=recoveryStore.Profile())||s.world!=CurrentWorld())return false;
    const bool host=PartyCommonHost(s);
    try{return PartySaveFingerprint(pr::LocalSlot(s,host))==pr::LocalFingerprint(s,host);}
    catch(const std::exception&){return false;}
}
void PartySavedAt(int slot){
    try{if(slot<0||slot>20||!recoveryReady)return;partySaved={recoveryStore.Profile(),PartySaveFingerprint(slot),CurrentWorld(),slot};partyControlSent=0;}
    catch(const std::exception& e){partySaved=pr::Saved{};LootLog("party_saved_unavailable",e.what());}
}
void PartyCheckpointCommit(){
    pr::Control m=partyControl;m.op=pr::ControlOp::Checkpoint;m.branchHost=m.branchGuest="";
    PartyPersistControl(m,"party-common.bin");partyCommon=m.common;partyConsent.Clear();partyControlStarted=0;
    partyControlText=notice=Ui("party.checkpoint_ready");LootLog("party_checkpoint","id="+partyCommon.id+" host_slot="+std::to_string(partyCommon.hostSlot)+" guest_slot="+std::to_string(partyCommon.guestSlot));
}
void PartyAdvanceConsent(){
    if(!HostCombat()||!partyConsent.Both())return;
    if(!PartyControlSafe()||!PartyValidateCommon(partyControl.common))throw std::runtime_error("Shared checkpoint preflight failed");
    if(!partyResumeIntent){PartyCheckpointCommit();auto m=partyControl;m.op=pr::ControlOp::Checkpoint;PartyControlSend(m);return;}
    if(partyConsent.prepared)return;
    partyConsent.Prepare();partyControl.branchHost=rc::NewGuid();partyControl.branchGuest=rc::NewGuid();partyControl.op=pr::ControlOp::Prepare;
    PartyPersistControl(partyControl,"party-resume.bin");PartyControlSend(partyControl);
    partyControlText=notice=Ui("party.resume_prepare");
}
void PartyRequestControl(bool resume){
    if(!CombatConnected()||!havePeerReadiness||!PartyControlSafe()||partyResumeCommitted||!partyConsent.token.empty()){notice=Ui("party.control_busy");return;}
    if(!resume&&(player->IsDead()||player->IsUnconscious()||partyPeerLife!=pr::Condition::Alive)){notice=Ui("party.need_alive");return;}
    if(!resume&&(!pr::Valid(partySaved)||!pr::Valid(partyPeerSaved)||partySaved.world!=partyPeerSaved.world)){
        notice=Ui("party.need_saves");return;
    }
    pr::Control m;m.op=resume?pr::ControlOp::Resume:pr::ControlOp::OfferCheckpoint;m.token=rc::NewGuid();
    if(resume){if(!PartyValidateCommon(partyCommon)){notice=Ui("party.checkpoint_invalid");return;}m.common=partyCommon;}
    else{
        const auto& host=HostCombat()?partySaved:partyPeerSaved;const auto& guest=HostCombat()?partyPeerSaved:partySaved;
        m.common={rc::NewGuid(),host.profile,guest.profile,host.fingerprint,guest.fingerprint,host.slot,guest.slot,host.world};
    }
    partyControl=m;partyResumeIntent=resume;partyConsent.Begin(m.token,true);partyControlStarted=Now();
    PartyControlSend(m);partyControlText=notice=Ui("party.wait_consent");
}
void PartyAcceptControl(bool accept){
    if(partyConsent.token.empty()||partyResumeCommitted){notice=Ui("party.no_request");return;}
    auto m=partyControl;m.op=accept?pr::ControlOp::Accept:pr::ControlOp::Decline;
    if(accept&&(!PartyControlSafe()||!PartyValidateCommon(m.common))){notice=Ui("party.checkpoint_invalid");return;}
    PartyControlSend(m);
    if(accept){partyConsent.Accept(m.token,false);PartyAdvanceConsent();}
    else{partyConsent.Clear();partyControlStarted=0;partyControlText=notice=Ui("party.declined");}
}
void PartyCloseMenusForLoad(){
    for(int i=0;i<zCMenu::activeList.GetNumInList();++i)if(auto* m=zCMenu::activeList[i])m->m_exitState=zCMenu::BACK;
}
void PartyQueueLoad(){if(partyLoadFailed)return;if(player&&recoveryTimeline==(PartyCommonHost(partyControl.common)?partyControl.branchHost:partyControl.branchGuest))return;partyLoadDue=Now()+300;partyControlText=notice=Ui("party.loading_common");PartyCloseMenusForLoad();}
void PartyReceiveControl(const co::Bytes& bytes){
    try{
        pr::Control m;if(!pr::Decode(bytes,m))throw std::runtime_error("Invalid party control payload");
        if(!havePeerReadiness||m.profile!=peerReadiness.profile||m.timeline!=peerReadiness.timeline)return;
        if(m.op==pr::ControlOp::Status){
            partyPeerSaved=m.saved;partyPeerLife=m.life;
            if(partyResumeCommitted&&HostCombat()&&partyControl.common.guestProfile==m.profile&&m.timeline!=partyControl.branchGuest){
                auto retry=partyControl;retry.op=pr::ControlOp::Commit;PartyControlSend(retry);retry.op=pr::ControlOp::Go;PartyControlSend(retry);
            }
            return;
        }
        if(m.common.hostProfile!=(HostCombat()?recoveryStore.Profile():peerReadiness.profile)||
           m.common.guestProfile!=(HostCombat()?peerReadiness.profile:recoveryStore.Profile())||!PartyValidateCommon(m.common))return;
        if(m.op==pr::ControlOp::OfferCheckpoint||m.op==pr::ControlOp::Resume){
            if(partyResumeCommitted||!PartyControlSafe()){notice=Ui("party.control_busy");return;}
            if(!partyConsent.token.empty()&&partyConsent.token!=m.token){notice=Ui("party.control_busy");return;}
            partyControl=m;partyResumeIntent=m.op==pr::ControlOp::Resume;partyConsent.Begin(m.token,false);partyConsent.Accept(m.token,true);partyControlStarted=Now();
            partyControlText=notice=partyResumeIntent?Ui("party.resume_offer"):Ui("party.checkpoint_offer");
            LootLog("party_consent_offer","token="+m.token+" resume="+std::to_string(partyResumeIntent));return;
        }
        if(partyConsent.token.empty()&&!partyResumeCommitted)return;
        if(m.token!=partyControl.token||!pr::Same(m.common,partyControl.common))return;
        if(m.op>=pr::ControlOp::Ready&&(m.branchHost!=partyControl.branchHost||m.branchGuest!=partyControl.branchGuest))return;
        if(m.op==pr::ControlOp::Decline&&!partyResumeCommitted){partyConsent.Clear();partyControlStarted=0;partyControlText=notice=Ui("party.declined");return;}
        if(m.op==pr::ControlOp::Accept){partyConsent.Accept(m.token,true);PartyAdvanceConsent();return;}
        if(m.op==pr::ControlOp::Checkpoint&&!HostCombat()&&partyConsent.Both()&&!partyResumeIntent){PartyCheckpointCommit();return;}
        if(m.op==pr::ControlOp::Prepare&&!HostCombat()&&pr::FreshBranches(m,recoveryTimeline,peerReadiness.timeline)&&partyConsent.Both()&&partyResumeIntent&&PartyControlSafe()){
            partyControl=m;partyConsent.Prepare();PartyPersistControl(m,"party-resume.bin");m.op=pr::ControlOp::Ready;PartyControlSend(m);return;
        }
        if(m.op==pr::ControlOp::Ready&&HostCombat()&&partyConsent.prepared&&m.branchHost==partyControl.branchHost&&m.branchGuest==partyControl.branchGuest){
            partyConsent.Commit();partyResumeCommitted=true;partyControl.op=pr::ControlOp::Commit;PartyPersistControl(partyControl,"party-resume.bin");PartyControlSend(partyControl);return;
        }
        if(m.op==pr::ControlOp::Commit&&!HostCombat()&&(partyConsent.prepared||partyResumeCommitted)&&m.branchHost==partyControl.branchHost&&m.branchGuest==partyControl.branchGuest){
            partyControl=m;partyResumeIntent=partyResumeCommitted=true;partyConsent.Commit();PartyPersistControl(m,"party-resume.bin");m.op=pr::ControlOp::Committed;PartyControlSend(m);return;
        }
        if(m.op==pr::ControlOp::Committed&&HostCombat()&&partyResumeCommitted){auto go=partyControl;go.op=pr::ControlOp::Go;PartyControlSend(go);PartyQueueLoad();return;}
        if(m.op==pr::ControlOp::Go&&!HostCombat()&&partyResumeCommitted){PartyQueueLoad();return;}
    }catch(const std::exception& e){partyControlText=notice=Ui("party.control_error");LootLog("party_control_error",e.what());}
}
bool PartyBeforeRead(int slot){
    if(partyLoadActive)return true;
    if(CombatConnected()){PartyRequestControl(true);return false;}
    if(partyResumeCommitted){
        const bool host=PartyCommonHost(partyControl.common);
        if(slot==pr::LocalSlot(partyControl.common,host)){
            try{if(PartySaveFingerprint(slot)==pr::LocalFingerprint(partyControl.common,host)){partyLoadFailed=false;partyLoadActive=true;return true;}}catch(const std::exception&){}
            notice=Ui("party.checkpoint_invalid");return false;
        }
        // A deliberate unrelated manual load supersedes an interrupted resume.
        partyLoadFailed=false;partyResumeCommitted=false;partyControl=pr::Control{};DeleteFileA(PartyControlFile("party-resume.bin").c_str());
    }
    return true;
}
void PartyAfterRead(int slot){
    if(partyLoadActive&&recoveryReady){
        const bool host=PartyCommonHost(partyControl.common);const auto oldLocal=recoveryTimeline,oldGuest=characterArchive.guestLineage;
        if(host){
            for(auto* npc:CharacterNpcSet())if(CharacterSummonNpc(npc)){
                const std::string name=npc->GetObjectName().ToChar();auto changed=pr::RebranchSpawnName(name,"ZKSH_",CharacterLineageTag(oldLocal),CharacterLineageTag(partyControl.branchHost));
                if(!oldGuest.empty())changed=pr::RebranchSpawnName(changed,"ZKSG_",CharacterLineageTag(oldGuest),CharacterLineageTag(partyControl.branchGuest));
                if(changed!=name)npc->SetObjectName(zSTRING(changed.c_str()));
            }
            if(!oldGuest.empty())characterArchive.guestLineage=partyControl.branchGuest;
        }
        recoveryTimeline=host?partyControl.branchHost:partyControl.branchGuest;
        recoveryEffects.clear();recoveryLastReplay=0;characterArchive.timeline=recoveryTimeline;
        partyRejoin=true;partyRejoinDue=Now()+500;
        if(gameMan){gameMan->exitSession=FALSE;gameMan->dontStartGame=FALSE;gameMan->gameIdle=FALSE;}
        LootLog("party_resume_loaded","token="+partyControl.token+" branch="+recoveryTimeline+" slot="+std::to_string(slot));
    }
    if(partyLoadActive&&!recoveryReady){partyLoadFailed=true;partyControlText=notice=Ui("party.control_error");LootLog("party_resume_failed","Native checkpoint did not become ready; automatic retry stopped");}
    PartySavedAt(slot);partyLoadActive=false;
}
void PartyProcessLoad(){
    if(!partyLoadDue||Now()<partyLoadDue||partyLoadActive||!partyControlOutbox.empty()||zCMenu::activeList.GetNumInList()>0)return;
    partyLoadDue=0;
    if(!partyResumeCommitted||!PartyValidateCommon(partyControl.common)){partyLoadFailed=true;partyControlText=notice=Ui("party.checkpoint_invalid");LootLog("party_resume_failed","Checkpoint changed; automatic retry stopped");return;}
    const int slot=pr::LocalSlot(partyControl.common,PartyCommonHost(partyControl.common));
    partyLoadActive=true;::PartyNativeLoad(slot);partyLoadActive=false;
}
void PartyControlFrame(unsigned long long now){
    if(partyRejoin&&now>=partyRejoinDue&&player&&!loading){partyRejoin=false;Start(PartyCommonHost(partyControl.common));}
    if(!CombatConnected()){
        if(partyResumeCommitted&&!partyLoadFailed&&!partyLoadActive&&!partyRejoin&&!partyLoadDue&&player&&recoveryTimeline!=(PartyCommonHost(partyControl.common)?partyControl.branchHost:partyControl.branchGuest))PartyQueueLoad();
        return;
    }
    if(havePeerReadiness&&recoveryReady&&timing::Elapsed(now,partyControlSent)>=2000){
        partyControlSent=now;pr::Control m;m.profile=recoveryStore.Profile();m.timeline=recoveryTimeline;m.saved=partySaved;
        m.life=pr::Life(player&&player->IsDead(),player&&player->IsUnconscious());PartyControlSend(m);
        const auto defeat=pr::DefeatState(m.life,partyPeerLife);
        if(defeat!=partyLastDefeat){partyLastDefeat=defeat;LootLog("party_defeat","state="+std::to_string(static_cast<unsigned>(defeat)));if(defeat!=pr::Defeat::Playing)notice=Ui("party.defeat_notice");}
        if(partyResumeCommitted&&recoveryTimeline==(HostCombat()?partyControl.branchHost:partyControl.branchGuest)&&peerReadiness.timeline==(HostCombat()?partyControl.branchGuest:partyControl.branchHost)){
            partyResumeCommitted=false;partyConsent.Clear();DeleteFileA(PartyControlFile("party-resume.bin").c_str());partyControlText=notice=Ui("party.resume_done");
        }
    }
    for(int n=0;n<4&&!partyControlOutbox.empty()&&network.PendingEvents()<48;++n){
        if(!network.SendEvent(partyControlOutbox.front(),now))break;partyControlOutbox.pop_front();
    }
    if(partyControlStarted&&!partyResumeCommitted&&timing::Elapsed(now,partyControlStarted)>120000){partyConsent.Clear();partyControlStarted=0;partyControlText=notice=Ui("party.expired");}
}

void PartyMenuRefresh(zCMenu* menu){
    if(!menu||_stricmp(menu->GetName().ToChar(),"MENU_OREBOUND_SESSION")!=0)return;
    const std::string text=!CombatConnected()?Ui("party.need_connection"):!partyControlText.empty()?partyControlText:pr::Valid(partyCommon)?Ui("party.checkpoint_ready"):Ui("party.need_saves");
    const auto& chosen=partyConsent.token.empty()?partyCommon:partyControl.common;
    std::string details=text;
    if(pr::Valid(chosen))details+=std::string("\n")+Ui("party.pair_slots")+std::to_string(chosen.hostSlot)+" / "+std::to_string(chosen.guestSlot)+" | "+chosen.world;
    else if(pr::Valid(partySaved))details+=std::string("\n")+Ui("party.local_slot")+std::to_string(partySaved.slot);
    for(int i=0;i<menu->m_listItems.GetNum();++i){auto* item=menu->m_listItems[i];if(item&&_stricmp(item->GetName().ToChar(),"MENUITEM_ORB_SESSION_STATUS")==0&&item->m_parText[0]!=zSTRING(details.c_str())){
        item->ClrPrintwin();if(item->m_pInnerWindow)item->m_pInnerWindow->ClrPrintwin();item->SetText(zSTRING(details.c_str()),0,FALSE);
    }}
}
bool PartyMenuAction(const zSTRING& command){
    try{
        if(command=="OREBOUND_CHECKPOINT")PartyRequestControl(false);
        else if(command=="OREBOUND_RESUME")PartyRequestControl(true);
        else if(command=="OREBOUND_ACCEPT")PartyAcceptControl(true);
        else if(command=="OREBOUND_DECLINE")PartyAcceptControl(false);
        else return false;
    }catch(const std::exception& e){partyControlText=notice=Ui("party.control_error");LootLog("party_control_error",e.what());}
    return true;
}
void PartyAbandonResume(){
    if(partyLoadActive)return;
    partySaved={};partyLoadFailed=false;partyResumeCommitted=partyResumeIntent=partyRejoin=false;partyLoadDue=0;partyControl=pr::Control{};partyConsent.Clear();
    DeleteFileA(PartyControlFile("party-resume.bin").c_str());
}
