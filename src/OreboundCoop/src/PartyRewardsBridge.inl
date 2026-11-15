namespace pr=party;
std::set<std::string> partyRewardReceipts;
std::map<std::string,unsigned long long> partyRewardSent;
std::string partyError;
bool partyRewardReplay=false;
void PartyReset(){PartyQuestReset();PartyControlReset();partyRewardReceipts.clear();partyRewardSent.clear();partyError.clear();}
pr::VictoryBefore PartyVictoryBefore(oCNpc* npc){
    pr::VictoryBefore s;
    s.eligible=CombatConnected()&&HostCombat()&&npc&&npc!=player&&npc!=PartnerNpc()&&recoveryReady&&
        havePeerReadiness&&!peerReadiness.timeline.empty()&&havePeerCombat&&npc->homeWorld==ogame->GetWorld()&&QuestScriptProfileMatch()&&QuestInt("AIV_VICTORYXPGIVEN",-1)==16&&QuestInt("XP_PER_VICTORY",0)==10;
    if(s.eligible){s.credited=npc->aiscriptvars[16]!=0;s.hostXP=player->experience_points;s.hostHP=player->attribute[0];s.hostUnconscious=player->IsUnconscious()!=FALSE;s.hostDead=player->IsDead()&&!s.hostUnconscious;s.hostMaxHP=player->attribute[1];s.hostLevel=player->level;s.hostLP=player->learn_points;s.hostNextXP=player->experience_points_next_level;s.proxyXP=PartnerNpc()?PartnerNpc()->experience_points:0;}
    return s;
}
bool PartyRecoveryEffect(const rc::Record& record,bool desired,bool& handled){
    handled=pr::PartyReward(record.tx);if(!handled)return false;
    if(!desired)return true;
    if(pr::UniqueConsumption(record.tx))return PartyUniqueEffect(record);
    if(!player||!parser||partyRewardReplay||!pr::CanApplyXP(pr::Life(player->IsDead()!=FALSE,player->IsUnconscious()!=FALSE)))return false;
    if(player->experience_points>static_cast<unsigned long>(INT_MAX-record.tx.amount))return false;
    const auto before=player->experience_points;const auto hp=player->attribute[0];const bool unconscious=player->IsUnconscious()!=FALSE;
    QuestBindings bindings;parser->SetInstance(zSTRING("HERO"),player);
    struct Replay {Replay(){partyRewardReplay=true;}~Replay(){partyRewardReplay=false;}} replay;
    ExtensionCall(extensions::Command::QuestXp,record.tx.amount);
    if(unconscious)player->attribute[0]=hp; // native level-up healing must not wake an unconscious recipient
    const bool applied=player->experience_points>=before&&player->experience_points-before==record.tx.amount;
    if(applied)LootLog("party_combat_xp","tx="+record.tx.id+" full_xp="+std::to_string(record.tx.amount)+" recipient=local");
    return applied;
}
std::string PartyVictoryKey(oCNpc* npc,NpcBinding& binding){
    const std::string key=CurrentWorld()+"|"+npc->GetObjectName().ToChar()+"|"+std::to_string(npc->GetInstance())+"|"+
        std::to_string(binding.anchor[0])+","+std::to_string(binding.anchor[1])+","+std::to_string(binding.anchor[2])+"|"+std::to_string(binding.generation);
    std::ostringstream out;out<<"ORB_XP_"<<std::hex<<std::setfill('0')<<std::setw(16)<<net::HashString(key);return out.str();
}
void PartyVictoryAfter(oCNpc* npc,const pr::VictoryBefore& before){
    if(before.eligible&&before.hostDead&&player){player->attribute[0]=before.hostHP;player->attribute[1]=before.hostMaxHP;player->level=before.hostLevel;player->learn_points=before.hostLP;player->experience_points=before.hostXP;player->experience_points_next_level=before.hostNextXP;}
    if(before.eligible&&before.hostUnconscious&&player)player->attribute[0]=before.hostHP;
    if(!before.eligible||before.credited||!npc||npc->aiscriptvars[16]==0||!CombatConnected()||!HostCombat())return;
    auto* binding=RegisterCombatNpc(npc);if(!binding)return;
    const auto amount=pr::VictoryXP(static_cast<unsigned>(npc->level),static_cast<unsigned>(QuestInt("XP_PER_VICTORY",0)));
    if(!amount)return;
    const auto key=PartyVictoryKey(npc,*binding);
    for(const auto& pair:recoveryStore.Records())if(pair.second.role==rc::Role::Host&&pair.second.tx.source.name==key&&pair.second.tx.hostTimeline==recoveryTimeline)return;
    try{
        rc::Tx tx;tx.id=rc::NewGuid();tx.hostTimeline=recoveryTimeline;tx.ownerTimeline=peerReadiness.timeline;tx.world=CurrentWorld();
        tx.action=rc::Action::CombatXp;tx.decision=rc::Decision::Commit;tx.source.kind=lo::Source::Quest;tx.source.name=key;
        tx.source.position={binding->anchor[0],binding->anchor[1],binding->anchor[2]};tx.instance=npc->GetInstance();tx.amount=amount;
        if(!recoveryStore.Put(tx,rc::Role::Host))throw std::runtime_error("Cannot persist shared victory XP");
        const auto after=player->experience_points;
        const bool nativeHost=after>=before.hostXP&&after-before.hostXP>=amount;
        if(nativeHost)recoveryEffects.insert(tx.id);
        else if(!RecoveryReconcile(*recoveryStore.Find(tx.id))){if(!before.hostDead)throw std::runtime_error("Host victory XP deferred");LootLog("party_reward_deferred","tx="+tx.id+" recipient=dead_host no_revival=1");}
        LootLog("party_victory","tx="+tx.id+" npc="+std::string(npc->GetObjectName().ToChar())+" event="+key+" xp_each="+std::to_string(amount)+" native_host="+std::to_string(nativeHost)+" native_proxy_delta="+std::to_string(PartnerNpc()&&PartnerNpc()->experience_points>=before.proxyXP?PartnerNpc()->experience_points-before.proxyXP:0));
    }catch(const std::exception& e){partyError=e.what();RecoveryFail(partyError);}
}
void PartyReceiveReward(const rc::Message& m){
    if(!pr::PartyReward(m.tx))throw std::runtime_error("Invalid party reward");
    if(HostCombat()){
        const auto* record=recoveryStore.Find(m.tx.id);
        if(m.op!=rc::Op::Receipt||!record||record->role!=rc::Role::Host||!rc::Same(record->tx,m.tx)||!havePeerReadiness||m.tx.ownerTimeline!=peerReadiness.timeline)throw std::runtime_error("Invalid combat XP receipt");
        if(pr::UniqueConsumption(m.tx))PartyUniqueReceipt(m);partyRewardReceipts.insert(m.tx.id);return;
    }
    if(m.op!=rc::Op::Result||m.tx.hostTimeline!=recoveryPeerTimeline||m.tx.ownerTimeline!=recoveryTimeline||m.tx.world!=CurrentWorld()||!havePeerReadiness||m.tx.hostTimeline!=peerReadiness.timeline)throw std::runtime_error("Party XP belongs to another session/world");
    const auto* old=recoveryStore.Find(m.tx.id);if(old&&(old->role!=rc::Role::Owner||!rc::Same(old->tx,m.tx)))throw std::runtime_error("Conflicting party reward");
    if(!recoveryStore.Put(m.tx,rc::Role::Owner))throw std::runtime_error("Cannot record party XP");
    if(!RecoveryReconcile(*recoveryStore.Find(m.tx.id))){LootLog("party_reward_deferred","tx="+m.tx.id+" awaiting_live_actor=1");return;}
    auto ack=m;ack.op=rc::Op::Receipt;ack.revision=PartyResourceRevision();QueueRecovery(ack);
}
void PartyRewardsFrame(unsigned long long now){
    if(!CombatConnected()||!HostCombat()||!havePeerReadiness||!recoveryReady)return;
    unsigned count=0;
    for(const auto& pair:recoveryStore.Records()){
        const auto& r=pair.second;const auto& t=r.tx;
        if(r.role!=rc::Role::Host||!pr::PartyReward(t)||t.hostTimeline!=recoveryTimeline||t.ownerTimeline!=peerReadiness.timeline||t.world!=CurrentWorld()||partyRewardReceipts.count(t.id)||timing::Elapsed(now,partyRewardSent[t.id])<2000)continue;
        rc::Message m;m.op=rc::Op::Result;m.tx=t;QueueRecovery(m);partyRewardSent[t.id]=now;if(++count==4)break;
    }
}
