void TestPartyRules(){
    namespace pr=orebound::party;namespace rc=orebound::recovery;
    for(unsigned a=0;a<17;++a)for(unsigned b=0;b<17;++b){
        const auto pa=pr::Path(a),pb=pr::Path(b);Check(pr::ShareFactionQuest(pr::Faction::Any,a,b),"ordinary quest eligible regardless of player factions");
        for(const auto required:{pr::Faction::Militia,pr::Faction::Mercenary,pr::Faction::Monastery})Check(pr::ShareFactionQuest(required,a,b)==(pa==required&&pb==required),"faction reward evaluated against both current guild paths");
    }
    Check(pr::Path(1)==pr::Path(2)&&pr::Path(6)==pr::Path(7)&&pr::Path(4)==pr::Path(5),"promotion within the same faction preserves shared quest eligibility");
    Check(pr::ShareFactionQuest(pr::Faction::Militia,2,2)&&!pr::ShareFactionQuest(pr::Faction::Militia,2,7),"faction change before grant changes eligibility, acceptance does not freeze it");
    Check(pr::CombatCredit(true,true)&&!pr::CombatCredit(false,true)&&!pr::CombatCredit(true,false),"combat XP requires same-world connection and no distance predicate");
    Check(pr::VictoryXP(2,10)==20&&pr::VictoryXP(1000000,2)==0&&pr::VictoryXP(1,0)==0&&pr::VictoryXP(100000,10)==1000000,"full native victory XP with checked arithmetic");
    Check(pr::CanApplyXP(pr::Condition::Alive)&&pr::CanApplyXP(pr::Condition::Unconscious)&&!pr::CanApplyXP(pr::Condition::Dead),"unconscious XP is immediate while corpse XP cannot revive a player");
    Check(pr::Life(true,true)==pr::Condition::Unconscious&&pr::DefeatState(pr::Condition::Unconscious,pr::Condition::Alive)==pr::Defeat::Playing,"unconscious recipient is not treated as a corpse");
    Check(pr::DefeatState(pr::Condition::Dead,pr::Condition::Alive)==pr::Defeat::OneDead&&pr::DefeatState(pr::Condition::Dead,pr::Condition::Dead)==pr::Defeat::BothDead,"individual and party defeat distinguished");
    Check(pr::SharedDecisionInfo(4)&&pr::SharedDecisionInfo(7)&&pr::SharedDecisionInfo(8)&&!pr::SharedDecisionInfo(0)&&!pr::SharedDecisionInfo(9),"shared progression choices do not lock permanent informational dialogue");
    Check(pr::RebranchSpawnName("ZKSG_old_token","ZKSG_","old_","new_")=="ZKSG_new_token","restored owned summon receives the new agreed recovery lineage without respawning");
    Check(pr::RebranchSpawnName("XARDAS","ZKSG_","old_","new_")=="XARDAS"&&pr::RebranchSpawnName("ZKSH_old_token","ZKSG_","old_","new_")=="ZKSH_old_token","ordinary NPCs and the other player's ownership prefix are not retagged");
    Check(pr::UniqueResource(4096,"ITMI_LETTER")&&pr::UniqueResource(0,"ITKE_XARDAS")&&!pr::UniqueResource(0,"ITMI_GOLD"),"quest flag and keys are permissions, ordinary gold remains an individual full reward");
}
void TestPartyDecisions(){
    namespace pr=orebound::party;namespace rc=orebound::recovery;
    const auto a=rc::NewGuid(),b=rc::NewGuid(),owner=rc::NewGuid();pr::Decisions d;
    const auto scope=pr::DecisionScope(7);Check(scope==pr::DecisionScope(8),"both mutually exclusive Cavalorn completion variants have one decision scope");
    Check(d.Acquire(scope,a,owner,1)==pr::Claim::Granted&&d.Acquire(scope,b,owner,1)==pr::Claim::Busy,"simultaneous second quest choice cannot overtake first prepared choice");
    Check(d.Acquire(scope,a,owner,1)==pr::Claim::Duplicate&&!d.Commit(scope,b),"repeated request retains its identity and cannot commit another token");
    Check(d.Commit(scope,a)&&d.Acquire(scope,b,owner,2)==pr::Claim::Conflict&&!d.Abort(scope,a),"first confirmed shared decision cannot be reversed");
    const auto bytes=pr::EncodeDecisions(d);auto restart=pr::DecodeDecisions(bytes);Check(restart.Rows().at(scope).committed&&restart.Rows().at(scope).token==a,"first committed decision survives restart");
    for(std::size_t n=0;n<bytes.size();++n){bool rejected=false;try{pr::DecodeDecisions(Bytes(bytes.begin(),bytes.begin()+n));}catch(const std::exception&){rejected=true;}Check(rejected,"truncated durable decisions rejected");}
    auto damaged=bytes;damaged[7]^=1;bool rejected=false;try{pr::DecodeDecisions(damaged);}catch(const std::exception&){rejected=true;}Check(rejected,"decision CRC catches altered ownership");
    pr::Decisions pending;Check(pending.Acquire("x",a,owner,1)==pr::Claim::Granted&&pending.Abort("x",a)&&pending.Acquire("x",b,owner,2)==pr::Claim::Granted,"only uncommitted claims may be explicitly aborted");
}
void TestPartyControl(){
    namespace pr=orebound::party;namespace rc=orebound::recovery;
    pr::Control c;c.profile=rc::NewGuid();c.timeline=rc::NewGuid();c.saved={c.profile,"0123456789abcdef","NEWWORLD.ZEN",3};
    const auto guest=rc::NewGuid();c.common={rc::NewGuid(),c.profile,guest,"0123456789abcdef","fedcba9876543210",3,8,"NEWWORLD.ZEN"};c.token=rc::NewGuid();
    for(unsigned op=0;op<=10;++op){c.op=static_cast<pr::ControlOp>(op);if(op>=6){c.branchHost=rc::NewGuid();c.branchGuest=rc::NewGuid();}else c.branchHost=c.branchGuest="";pr::Control decoded;const auto bytes=pr::Encode(c);Check(pr::Decode(bytes,decoded)&&decoded.op==c.op&&pr::Same(c.common,decoded.common),"checkpoint, consent and staged load messages roundtrip");Check(bytes.size()<=MaximumDatagramBytes-ProtocolHeaderBytes,"party checkpoint message fits one reliable datagram");for(std::size_t n=0;n<bytes.size();++n)Check(!pr::Decode(Bytes(bytes.begin(),bytes.begin()+n),decoded),"partial consent packet cannot authorize a load");auto tail=bytes;tail.push_back(0);Check(!pr::Decode(tail,decoded),"consent message tail rejected");}
    c.op=pr::ControlOp::Prepare;c.branchGuest=c.branchHost;pr::Control decoded;Check(!pr::Decode(pr::Encode(c),decoded),"load branches must differ");c.branchGuest=rc::NewGuid();c.branchHost=c.timeline;Check(pr::Decode(pr::Encode(c),decoded)&&!pr::FreshBranches(c,c.timeline,guest),"a prepare cannot reuse an old branch; committed reconnect messages may originate from the new branch");
    const auto original=c.common;c.common.guestSlot++;Check(!pr::Same(original,c.common),"a different guest slot is a different consent target");
    pr::Consent host,peer;const auto token=rc::NewGuid();host.Begin(token,true);peer.Begin(token,false);Check(!host.Prepare()&&!peer.Commit(),"one player's click cannot load either game");Check(peer.Accept(token,true)&&peer.Accept(token,false)&&peer.Prepare(),"recipient records explicit agreement and initiator consent");Check(!host.Accept(rc::NewGuid(),true)&&host.Accept(token,true)&&host.Prepare()&&host.Commit(),"stale request cannot satisfy current consent");Check(!host.Accept(token,true),"committed consent cannot be changed");
    Check(pr::CanResume(original,false,true,true)&&!pr::CanResume(original,true,true,true)&&!pr::CanResume(original,false,false,true)&&!pr::CanResume(original,false,true,false),"pending transfer, overwritten native save or missing consent each prevent common load");
    Check(pr::LocalSlot(original,true)==3&&pr::LocalSlot(original,false)==8&&pr::LocalProfile(original,false)==guest,"host and client may select different local slots for one common pair");
}
void TestPartyResources(){
    namespace pr=orebound::party;namespace rc=orebound::recovery;
    pr::Resources m;m.profile=rc::NewGuid();m.timeline=rc::NewGuid();m.world="NEWWORLD.ZEN";m.revision=1;m.pages=2;m.items={{100,1},{200,1}};
    auto bytes=pr::Encode(m);pr::Resources out;Check(pr::Decode(bytes,out)&&out.items.size()==2,"shared mission-item permission page roundtrip");for(std::size_t n=0;n<bytes.size();++n)Check(!pr::Decode(Bytes(bytes.begin(),bytes.begin()+n),out),"truncated permission snapshot rejected");
    pr::ResourcePages pages;Check(pages.Add(m)&&!pages.Complete()&&pages.Stock().empty(),"incomplete mission inventory cannot grant permissions");m.page=1;m.items={{300,1}};Check(pages.Add(m)&&pages.Complete()&&pages.Stock().size()==3,"full page set grants permissions without creating inventory objects");Check(pages.Add(m),"identical repeated permission page is harmless");m.items[0].amount=2;Check(!pages.Add(m),"conflicting repeated page rejected");m.revision=2;m.pages=1;m.page=0;m.items.clear();Check(pages.Add(m)&&pages.Complete()&&pages.Stock().empty(),"new empty inventory revokes old key permissions");m.revision=1;Check(!pages.Add(m),"delayed old inventory cannot restore consumed key permissions");
    m.revision=3;m.items={{100,1},{100,1}};Check(!pr::Decode(pr::Encode(m),out),"duplicate resource IDs within page rejected");
}
void TestPartyAwards(){
    namespace pr=orebound::party;namespace rc=orebound::recovery;
    auto t=recovery_test::Transaction();t.action=rc::Action::CombatXp;t.source.kind=orebound::loot::Source::Quest;t.source.name="ORB_XP_0123456789abcdef";t.instance=1234;t.item=0;t.amount=20;t.decision=rc::Decision::Commit;
    Check(rc::Valid(t)&&pr::PartyReward(t),"combat XP has an independently validated journal identity");t.source.name.back()='X';Check(!rc::Valid(t)&&!pr::PartyReward(t),"invented combat reward identity refused");t.source.name="ORB_XP_0123456789abcdef";
    std::set<std::string> effects;unsigned xp=0;for(int retry=0;retry<10;++retry)Check(rc::Reconcile({t,rc::Role::Owner,1},effects,[&](const rc::Record& r,bool wanted){if(wanted)xp+=r.tx.amount;return true;}),"combat XP resend reconciles");Check(xp==20,"full combat XP applied once after lost receipt");
    auto use=t;use.id=rc::NewGuid();use.action=rc::Action::UniqueConsume;use.source.name="ORB_USE_0123456789abcdef";use.item=use.instance=444;use.amount=1;
    Check(rc::Valid(use)&&pr::UniqueConsumption(use)&&pr::PartyReward(use),"unique consumption has one durable owner debit");unsigned stock=1;effects.clear();for(int retry=0;retry<12;++retry)Check(rc::Reconcile({use,rc::Role::Owner,1},effects,[&](const rc::Record& r,bool wanted){if(!wanted||stock<r.tx.amount)return false;stock-=r.tx.amount;return true;}),"unique debit receipt retry");Check(stock==0&&effects.size()==1,"unique quest item is consumed exactly once across repeated transfers");
    const auto dir=recovery_test::Temp(),path=dir+"\\transactions.bin";rc::Store journal;journal.Open(path);Check(journal.Put(t,rc::Role::Host)&&journal.Put(use,rc::Role::Host),"combat award and unique consumption durable before sending");rc::Store restarted;restarted.Open(path);Check(restarted.Records().size()==2&&pr::UniqueConsumption(restarted.Find(use.id)->tx),"new semantics reopen with existing journal format");
    rc::Checkpoint cp;cp.profile=journal.Profile();cp.timeline=t.ownerTimeline;cp.effects=effects;const auto restored=rc::DecodeCheckpoint(rc::Encode(cp));Check(restored.effects.count(use.id)==1,"saved owner receipt prevents double consumption after reload");
    const auto newBranch=rc::NewGuid();Check(t.hostTimeline!=newBranch&&use.hostTimeline!=newBranch,"agreed rollback starts a fresh timeline instead of replaying post-checkpoint decisions");DeleteFileA(path.c_str());RemoveDirectoryA(dir.c_str());
}
void TestPartyReliableFlow(){
    namespace pr=orebound::party;namespace rc=orebound::recovery;Network host,guest;ConnectedPair(host,guest);
    pr::QuestMessage request;request.token=rc::NewGuid();request.host=rc::NewGuid();request.owner=rc::NewGuid();request.info=7;auto wire=pr::Encode(request);pr::QuestMessage read;
    Check(pr::Decode(wire,read)&&read.info==7,"guest native quest request codec");for(std::size_t n=0;n<wire.size();++n)Check(!pr::Decode(Bytes(wire.begin(),wire.begin()+n),read),"partial native quest request rejected");
    Check(guest.SendEvent(wire,1010),"guest quest claim sent on real reliable UDP");Pump(host,guest,1010);Bytes received;Check(host.ConsumeEvent(received)&&pr::Decode(received,read)&&read.token==request.token,"host receives the original quest decision token");
    auto result=request;result.op=pr::QuestOp::Result;result.accepted=true;
    for(unsigned i=1;i<=4;++i){auto tx=recovery_test::Transaction(i);tx.source.kind=orebound::loot::Source::Quest;tx.source.name="CAVALORN_KILLBRAGO";tx.hostTimeline=request.host;tx.ownerTimeline=request.owner;tx.decision=rc::Decision::Commit;tx.instance=i;tx.action=i==1?rc::Action::QuestXp:rc::Action::QuestItem;tx.item=i==1?0:i;tx.amount=i==1?100:i==2?50:i==3?1:2;result.rewards.push_back(tx);}
    const auto grant=pr::Encode(result);Check(grant.size()<=MaximumDatagramBytes-ProtocolHeaderBytes&&pr::Decode(grant,read)&&read.rewards.size()==4,"full bounded native quest grant fits one reliable packet");
    for(std::size_t n=0;n<grant.size();++n)Check(!pr::Decode(Bytes(grant.begin(),grant.begin()+n),read),"truncated native reward batch cannot partially credit a quest");
    auto invalid=result;invalid.rewards[1].id=invalid.rewards[0].id;Check(!pr::Decode(pr::Encode(invalid),read),"repeated reward ID in one quest batch rejected");invalid=result;invalid.accepted=false;Check(!pr::Decode(pr::Encode(invalid),read),"rejected native quest cannot carry a reward batch");
    Check(host.SendEvent(pr::Encode(result),1020),"host confirms a shared decision");Pump(host,guest,1020);Check(guest.ConsumeEvent(received)&&pr::Decode(received,read)&&read.accepted,"guest sees authoritative confirmation");
    pr::Resources caps;caps.profile=rc::NewGuid();caps.timeline=request.owner;caps.world="NEWWORLD.ZEN";caps.revision=1;caps.items={{444,1}};Check(guest.SendEvent(pr::Encode(caps),1030),"shared key capability sent over UDP");Pump(host,guest,1030);pr::Resources decoded;Check(host.ConsumeEvent(received)&&pr::Decode(received,decoded)&&decoded.items[0].amount==1,"host gets permission, no second physical item grant");
}
