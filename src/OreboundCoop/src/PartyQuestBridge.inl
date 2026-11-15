std::map<unsigned,oCItem*> partyPermissionHandles;
oCItem* partyArmorHandle=nullptr;
std::vector<rc::Tx> partyQuestUses;
unsigned partyResourceFloor=0;
pr::Decisions partyDecisions;
std::string partyDecisionTimeline,partyActiveToken,partyActiveScope;
bool partyQuestAuthority=false,partyQuestPlayback=false;
pr::QuestMessage partyPendingQuest;
unsigned long long partyQuestPendingSince=0,partyResourcesSent=0,partyResourcesReceived=0;
unsigned partyResourceRevision=0;
unsigned PartyResourceRevision(){return partyResourceRevision;}
pr::ResourcePages partyResourcePages;
std::map<unsigned,unsigned> partyPeerResources;
std::map<std::string,pr::QuestMessage> partyQuestResponses;
oCNpc* PartyQuestActor(){return characterGuestDepth?PartnerNpc():player;}
bool PartyQuestBusy(){return partyQuestPendingSince!=0||!partyActiveToken.empty();}
void PartyQuestReset(){if(partyArmorHandle){partyArmorHandle->Release();partyArmorHandle=nullptr;}for(auto& h:partyPermissionHandles)h.second->Release();partyPermissionHandles.clear();partyQuestUses.clear();partyResourceFloor=0;partyPendingQuest={};partyQuestPendingSince=partyResourcesSent=partyResourcesReceived=0;partyActiveToken.clear();partyActiveScope.clear();partyResourcePages={};partyPeerResources.clear();partyQuestResponses.clear();}
void PartyReadDecisions(){
    if(partyDecisionTimeline==recoveryTimeline)return;
    partyDecisions={};partyDecisionTimeline.clear();
    const auto path=recoveryDir+"\\quest-decisions-"+recoveryTimeline+".bin";
    if(GetFileAttributesA(path.c_str())!=INVALID_FILE_ATTRIBUTES)partyDecisions=pr::DecodeDecisions(rc::ReadFile(path));
    for(const auto& row:partyDecisions.Rows())if(!row.second.committed)throw std::runtime_error("Interrupted native quest decision; load a matching shared return point");
    partyDecisionTimeline=recoveryTimeline;
}
void PartyWriteDecisions(){rc::AtomicWrite(recoveryDir+"\\quest-decisions-"+recoveryTimeline+".bin",pr::EncodeDecisions(partyDecisions));}
void PartyQuestSend(const pr::QuestMessage& m){if(!pr::Valid(m)||lootOutbox.size()>=96)throw std::runtime_error("Quest queue invalid/full");lootOutbox.push_back(pr::Encode(m));}
bool PartyQuestDecisionBefore(oCInfo* info){
    const auto which=QuestInfoIndex(info);if(which<0||!CombatConnected()||partyQuestPlayback)return true;
    try{
        if(!HostCombat()){
            if(partyQuestPendingSince){notice=Ui("party.quest_wait");return false;}
            partyPendingQuest={};partyPendingQuest.token=rc::NewGuid();partyPendingQuest.host=recoveryPeerTimeline;partyPendingQuest.owner=recoveryTimeline;partyPendingQuest.info=which;
            PartyQuestSend(partyPendingQuest);partyQuestPendingSince=Now();notice=Ui("party.quest_wait");return false;
        }
        if(!pr::SharedDecisionInfo(which))return true;
        if(!partyQuestAuthority)partyQuestUses.clear();
        PartyReadDecisions();const auto token=partyQuestAuthority?partyPendingQuest.token:rc::NewGuid();const auto scope=pr::DecisionScope(which);
        const auto result=partyDecisions.Acquire(scope,token,partyQuestAuthority?peerReadiness.profile:recoveryStore.Profile(),1);
        if(result!=pr::Claim::Granted){notice=Ui("party.quest_decided");return false;}
        PartyWriteDecisions();partyActiveToken=token;partyActiveScope=scope;return true;
    }catch(const std::exception& e){QuestFail(e.what());return false;}
}
void PartyQuestDecisionAfter(oCInfo* info){
    if(!HostCombat()||partyActiveToken.empty())return;
    try{
        if(!questError.empty())throw std::runtime_error("Native quest result did not reconcile");
        if(!partyDecisions.Commit(partyActiveScope,partyActiveToken))throw std::runtime_error("Native quest decision lost its claim");
        PartyWriteDecisions();pr::QuestMessage m;m.op=pr::QuestOp::Notice;m.token=partyActiveToken;m.host=recoveryTimeline;m.owner=peerReadiness.timeline;m.info=QuestInfoIndex(info);m.accepted=true;
        PartyQuestSend(m);LootLog("party_quest_decision","scope="+partyActiveScope+" token="+partyActiveToken+" actor="+(partyQuestAuthority?"guest":"host"));
        partyActiveToken.clear();partyActiveScope.clear();
    }catch(const std::exception& e){QuestFail(e.what());RecoveryFail("Quest decision requires matching checkpoint recovery");}
}
std::vector<rc::Tx> PartyQuestAwards(unsigned which){
    std::vector<rc::Tx> out=partyQuestUses;for(unsigned event=1;event<=4;++event){if((which==5&&event>=3)||((which==7||which==8)&&event<=2)){const auto* r=QuestEvent(event);if(r&&r->tx.ownerTimeline==peerReadiness.timeline)out.push_back(r->tx);}}return out;
}
void PartyQuestHostRequest(pr::QuestMessage m){
    const auto cached=partyQuestResponses.find(m.token);if(cached!=partyQuestResponses.end()){PartyQuestSend(cached->second);return;}
    auto result=m;result.op=pr::QuestOp::Result;result.accepted=false;
    auto* info=QuestInfo(cavalornInfos[m.info]);oCNpc* npc=nullptr;int matches=0;
    if(info)for(const auto& binding:npcBindings)if(binding->npc&&binding->npc->GetInstance()==info->pd.npc){npc=binding->npc;++matches;}
    if(!QuestPartyReady()||!PartnerNpc()||matches!=1||!npc||npc->IsDead()||WorldLocalTalksTo(npc)||(npc->GetPositionWorld()-PartnerNpc()->GetPositionWorld()).Length()>800){PartyQuestSend(result);return;}
    QuestBindings bindings;CharacterGuestScope guest(true,npc);if(guest.failed||!guest.active){PartyQuestSend(result);return;}
    parser->SetInstance(zSTRING("SELF"),npc);parser->SetInstance(zSTRING("OTHER"),PartnerNpc());
    partyPendingQuest=m;partyQuestUses.clear();partyQuestAuthority=true;
    struct Authority {~Authority(){partyQuestAuthority=false;}} authority;
    if(!::PartyNativeCondition(info)){PartyQuestSend(result);return;}
    if(PartyQuestDecisionBefore(info)){
        const auto before=QuestBeforeInfo(info);::PartyNativeInfo(info);QuestAfterInfo(before);PartyQuestDecisionAfter(info);
        result.accepted=questError.empty()&&partyActiveToken.empty();if(result.accepted)result.rewards=PartyQuestAwards(m.info);
    }
    // Response is queued before the next authoritative quest-state snapshot.
    PartyQuestSend(result);if(partyQuestResponses.size()>=64)partyQuestResponses.erase(partyQuestResponses.begin());partyQuestResponses[m.token]=result;
}
void PartyQuestGuestResult(const pr::QuestMessage& m){
    if(!partyQuestPendingSince||m.token!=partyPendingQuest.token||m.info!=partyPendingQuest.info)return;
    partyQuestPendingSince=0;
    if(!m.accepted){notice=Ui("party.quest_decided");return;}
    auto* info=QuestInfo(cavalornInfos[m.info]);auto& manager=oCInformationManager::GetInformationManager();
    // No native script playback after the player has left this conversation.
    const bool playback=info&&manager.Info==info&&!manager.IsDone&&manager.Player==player;
    std::map<unsigned,unsigned> uniqueBefore;for(const auto& tx:m.rewards)if(pr::UniqueConsumption(tx))uniqueBefore[tx.item]=QuestInventory(tx.item);
    for(const auto& tx:m.rewards)if((!cv::Allowed(tx,cavalornRewards)&&!pr::UniqueConsumption(tx))||!recoveryStore.Put(tx,rc::Role::Owner))throw std::runtime_error("Guest native quest grant cannot be journaled");
    const unsigned xp=static_cast<unsigned>(player->experience_points);const auto gold=QuestInventory(cavalornRewards.goldItem),knife=QuestInventory(cavalornRewards.knifeItem),potions=QuestInventory(cavalornRewards.potionItem);
    if(playback){partyQuestPlayback=true;struct Playback {~Playback(){partyQuestPlayback=false;}} guard;QuestBindings binding;parser->SetInstance(zSTRING("HERO"),player);parser->SetInstance(zSTRING("OTHER"),player);parser->SetInstance(zSTRING("SELF"),manager.Npc);::PartyNativeInfo(info);}
    const unsigned actual[]={0,static_cast<unsigned>(player->experience_points-xp),QuestInventory(cavalornRewards.goldItem)-gold,QuestInventory(cavalornRewards.knifeItem)-knife,QuestInventory(cavalornRewards.potionItem)-potions};
    for(const auto& tx:m.rewards){if(playback){if((pr::UniqueConsumption(tx)?uniqueBefore[tx.item]-QuestInventory(tx.item):actual[tx.instance])!=tx.amount)throw std::runtime_error("Guest native quest reward differs from host; stop and load a matching pair");recoveryEffects.insert(tx.id);}else if(!RecoveryReconcile(*recoveryStore.Find(tx.id)))throw std::runtime_error("Guest quest reward deferred");auto ack=rc::Message{};ack.op=rc::Op::Receipt;ack.tx=tx;ack.revision=partyResourceRevision;QueueRecovery(ack);}
    LootLog("party_quest_guest_commit","token="+m.token+" native_playback="+std::to_string(playback)+" rewards="+std::to_string(m.rewards.size()));notice=Ui("party.quest_confirmed");
}
void PartyQuestReceive(const co::Bytes& bytes){
    try{
        if(bytes[0]==81){pr::Resources m;if(!pr::Decode(bytes,m))throw std::runtime_error("Malformed unique resource page");if(!havePeerReadiness||m.profile!=peerReadiness.profile||m.timeline!=peerReadiness.timeline||m.world!=CurrentWorld())return;
            if(partyResourceFloor&&!co::Newer(m.revision,partyResourceFloor))return;
            if(partyResourcePages.Revision()&&m.revision!=partyResourcePages.Revision()&&!co::Newer(m.revision,partyResourcePages.Revision()))return;
            if(!partyResourcePages.Add(m))throw std::runtime_error("Conflicting unique resource page");if(partyResourcePages.Complete()){partyPeerResources=partyResourcePages.Stock();partyResourcesReceived=Now();}return;}
        pr::QuestMessage m;if(!pr::Decode(bytes,m))throw std::runtime_error("Malformed supported quest request");
        if(!QuestSupported()||!havePeerReadiness||m.host!=(HostCombat()?recoveryTimeline:recoveryPeerTimeline)||m.owner!=(HostCombat()?peerReadiness.timeline:recoveryTimeline))return;
        if(m.op==pr::QuestOp::Request&&HostCombat())PartyQuestHostRequest(m);
        else if(m.op==pr::QuestOp::Result&&!HostCombat())PartyQuestGuestResult(m);
        else if(m.op==pr::QuestOp::Notice&&!HostCombat()){notice=Ui("party.quest_confirmed");LootLog("party_quest_peer_notice","token="+m.token+" info="+std::to_string(m.info));}
        else throw std::runtime_error("Wrong quest decision direction");
    }catch(const std::exception& e){QuestFail(e.what());RecoveryFail("Supported quest/capability reconciliation failed");}
}
bool PartySharedKey(oCMobLockable* lock,oCNpc* who){
    if(!lock||who!=player||!CombatConnected()||!QuestScriptProfileMatch()||!lock->locked||lock->keyInstance.Length()==0||!recoveryReady||!havePeerReadiness||!partyResourcesReceived||timing::Elapsed(Now(),partyResourcesReceived)>4000)return false;
    if((who->GetPositionWorld()-lock->GetPositionWorld()).Length()>500||who->IsInInv(lock->keyInstance,1))return false;
    const auto instance=parser->GetIndex(lock->keyInstance);const auto found=partyPeerResources.find(instance);
    if(found==partyPeerResources.end()||!found->second)return false;
    LootLog("party_shared_key","instance="+std::to_string(instance)+" physical_copy_created=0 owner=peer");return true;
}
void PartyQuestFrame(unsigned long long now){
    if(!CombatConnected()||!recoveryReady||!havePeerReadiness)return;
    if(partyQuestPendingSince&&timing::Elapsed(now,partyQuestPendingSince)>15000){partyQuestPendingSince=0;notice=Ui("party.quest_timeout");}
    if(timing::Elapsed(now,partyResourcesSent)<1500||lootOutbox.size()>48||saving||network.PeerSaving())return;
    try{
        QuestBindings bindings;player->inventory2.UnpackAllItems();LootSource own;own.vob=own.corpse=player;std::map<unsigned,unsigned> stock;
        for(auto* item:LootNativeItems(own))if(pr::UniqueResource(item->flags,item->GetInstanceName().ToChar())){auto& n=stock[item->GetInstance()];if(static_cast<unsigned>(item->amount)>1000000-n)throw std::runtime_error("Unique item stock overflow");n+=item->amount;}
        std::vector<pr::Resource> items;for(const auto& pair:stock)items.push_back({pair.first,pair.second});const auto pages=static_cast<unsigned>((std::max)(std::size_t(1),(items.size()+23)/24));if(pages>128||lootOutbox.size()+pages>96)return;
        auto rev=++partyResourceRevision;if(!rev)rev=++partyResourceRevision;
        for(unsigned page=0;page<pages;++page){pr::Resources m;m.profile=recoveryStore.Profile();m.timeline=recoveryTimeline;m.world=CurrentWorld();m.revision=rev;m.page=page;m.pages=pages;const auto first=(std::min)(items.size(),std::size_t(page*24)),end=(std::min)(items.size(),first+24);m.items.assign(items.begin()+first,items.begin()+end);lootOutbox.push_back(pr::Encode(m));}
        partyResourcesSent=now;
    }catch(const std::exception& e){QuestFail(e.what());}
}

bool PartyUniqueEffect(const rc::Record& record){
    if(record.role==rc::Role::Host)return true; // authoritative reservation; owner performs the single inventory debit
    if(record.role!=rc::Role::Owner||!player)return false;
    LootSource own;own.vob=own.corpse=player;
    const bool ok=RecoveryRemove(own,record.tx.item,record.tx.amount);
    if(ok)LootLog("party_unique_consumed","tx="+record.tx.id+" instance="+std::to_string(record.tx.item)+" amount="+std::to_string(record.tx.amount));return ok;
}
void PartyUniqueReceipt(const rc::Message& m){partyResourceFloor=m.revision;partyResourcesReceived=0;partyPeerResources.clear();partyResourcePages={};}
bool PartyUniquePending(){
    if(!CombatConnected()||!HostCombat())return false;
    for(const auto& pair:recoveryStore.Records()){const auto& r=pair.second;if(r.role==rc::Role::Host&&pr::UniqueConsumption(r.tx)&&r.tx.hostTimeline==recoveryTimeline&&havePeerReadiness&&r.tx.ownerTimeline==peerReadiness.timeline&&!partyRewardReceipts.count(r.tx.id))return true;}return false;
}
oCItem* PartyPermission(oCNpc* actor,int instance,int amount){
    if(!HostCombat()||!CombatConnected()||actor!=PartyQuestActor()||partyActiveToken.empty()||!QuestSupported()||amount<1||!partyResourcesReceived||timing::Elapsed(Now(),partyResourcesReceived)>4000||PartyUniquePending())return nullptr;
    const auto stock=partyPeerResources.find(instance);if(stock==partyPeerResources.end()||stock->second<static_cast<unsigned>(amount))return nullptr;
    auto found=partyPermissionHandles.find(instance);
    if(found==partyPermissionHandles.end()){auto* item=CreateLootItem({1,static_cast<unsigned>(instance),stock->second});if(!item)return nullptr;if(!pr::UniqueResource(item->flags,item->GetInstanceName().ToChar())){item->Release();return nullptr;}found=partyPermissionHandles.emplace(instance,item).first;}
    found->second->amount=stock->second;return found->second;
}
oCItem* PartyConsumePermission(oCNpc* actor,int instance,int amount){
    auto* handle=PartyPermission(actor,instance,amount);if(!handle)return nullptr;
    rc::Tx tx;tx.id=rc::NewGuid();tx.hostTimeline=recoveryTimeline;tx.ownerTimeline=peerReadiness.timeline;tx.world=CurrentWorld();tx.action=rc::Action::UniqueConsume;tx.decision=rc::Decision::Commit;tx.source.kind=lo::Source::Quest;
    std::ostringstream key;key<<"ORB_USE_"<<std::hex<<std::setfill('0')<<std::setw(16)<<net::HashString(partyActiveToken+"|"+std::to_string(instance));tx.source.name=key.str();tx.instance=tx.item=instance;tx.amount=amount;
    auto* detached=CreateLootItem({1,tx.item,tx.amount});if(!detached)throw std::runtime_error("Shared quest consumption handle unavailable");
    if(!recoveryStore.Put(tx,rc::Role::Host)){detached->Release();throw std::runtime_error("Cannot persist unique quest consumption");}
    recoveryEffects.insert(tx.id);partyPeerResources[instance]-=amount;partyQuestUses.push_back(tx);
    LootLog("party_unique_reserved","tx="+tx.id+" scope="+partyActiveScope+" physical_owner=guest");return detached;
}

bool PartyQuestArmorOverride(oCNpc* actor,oCItem*& item){
    if(!partyQuestAuthority||actor!=PartnerNpc()||!havePeerCombat)return false;
    item=nullptr;const auto instance=peerCombat.visual.armor;if(instance<0)return true;
    if(partyArmorHandle&&partyArmorHandle->GetInstance()!=instance){partyArmorHandle->Release();partyArmorHandle=nullptr;}
    if(!partyArmorHandle)partyArmorHandle=CreateLootItem({1,static_cast<unsigned>(instance),1});
    if(!partyArmorHandle){RecoveryFail("Remote quest armor predicate unavailable");return true;}
    item=partyArmorHandle;return true;
}
