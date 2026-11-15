void TestAsyncRecoveryDurability(){
    namespace rc=orebound::recovery;
    const auto dir=recovery_test::Temp(),path=dir+"\\transactions.bin";
    rc::Store store;store.Open(path);auto tx=recovery_test::Transaction(41);
    HANDLE entered=CreateEventA(nullptr,TRUE,FALSE,nullptr),release=CreateEventA(nullptr,TRUE,FALSE,nullptr);
    const auto mainThread=GetCurrentThreadId();std::atomic<DWORD> writerThread{0};
    rc::SetTraceSink([&](const std::string& phase,const std::string&){
        if(phase=="write_begin"){writerThread=GetCurrentThreadId();SetEvent(entered);WaitForSingleObject(release,3000);}
    });
    const bool started=store.BeginPut(tx,rc::Role::Owner);
    const bool reached=WaitForSingleObject(entered,2000)==WAIT_OBJECT_0;
    const bool hidden=store.Find(tx.id)==nullptr;
    const bool notReady=!store.Poll();
    auto other=recovery_test::Transaction(42);const bool backpressure=!store.BeginPut(other,rc::Role::Owner);
    SetEvent(release);store.Wait();rc::SetTraceSink({});CloseHandle(entered);CloseHandle(release);
    Check(started&&reached,"disk worker starts without blocking the calling thread");
    Check(writerThread!=mainThread,"durable disk writes execute outside the engine thread");
    Check(hidden&&notReady&&backpressure,"unflushed record is invisible and another write cannot overtake it");
    Check(store.Find(tx.id)&&store.Find(tx.id)->tx.decision==rc::Decision::Prepared,"completed write publishes prepared record on caller thread");
    rc::Store restart;restart.Open(path);Check(restart.Find(tx.id)!=nullptr,"async prepare survives reopening actual journal");
    tx.decision=rc::Decision::Commit;Check(store.BeginPut(tx,rc::Role::Owner),"async commit accepted");store.Wait();
    restart.Open(path);Check(restart.Find(tx.id)->tx.decision==rc::Decision::Commit,"async commit is durable before downstream effects");
    tx.decision=rc::Decision::Abort;Check(!store.BeginPut(tx,rc::Role::Owner),"async write cannot reverse committed transaction");
    Check(CreateDirectoryA((path+".pending").c_str(),nullptr)!=FALSE,"create actual disk failure fixture");
    Check(store.BeginPut(other,rc::Role::Owner),"failure fixture reaches asynchronous writer");
    bool failed=false;try{store.Wait();}catch(const std::exception&){failed=true;}
    Check(failed&&!store.Find(other.id),"write failure never publishes uncommitted record");
    RemoveDirectoryA((path+".pending").c_str());
    rc::Store preserved;preserved.Open(path);Check(preserved.Find(tx.id)&&!preserved.Find(other.id),"failed async write leaves committed journal readable");
    DeleteFileA(path.c_str());RemoveDirectoryA(dir.c_str());
}
void TestReplicaEquipmentAndTerminalCodec(){
    namespace co=orebound::combat;
    co::Peer p;p.stats.attributes[0]=10;p.stats.attributes[1]=100;p.visual.melee=123;p.visual.ranged=456;
    co::Peer decoded;Check(co::Decode(co::Encode(p),decoded)&&decoded.visual.melee==123&&decoded.visual.ranged==456,"actual weapon instances transmitted independently of weapon mode");
    p.visual.melee=-2;Check(!co::Decode(co::Encode(p),decoded),"invalid negative equipment rejected");p.visual.melee=1000000;Check(!co::Decode(co::Encode(p),decoded),"equipment instance bound enforced");
    co::Npc npc;npc.stats=p.stats;npc.id=1;npc.instance=123;npc.animation="T_DEAD";npc.animationProgress=0.6f;
    co::Npc read;Check(co::Decode(co::Encode(npc),read)&&read.animationProgress==0.6f,"death animation progress preserved rather than replayed from zero");
    npc.animationProgress=-0.1f;Check(!co::Decode(co::Encode(npc),read),"negative animation progress rejected");
    npc.animationProgress=1.1f;Check(!co::Decode(co::Encode(npc),read),"animation progress beyond endpoint rejected");
    npc.animationProgress=std::numeric_limits<float>::quiet_NaN();Check(!co::Decode(co::Encode(npc),read),"nonfinite animation progress rejected");
    Check(co::TerminalKind(0,0)==2&&co::TerminalKind(10,23|32768)==2,"dead health or flagged dead body has terminal state");
    Check(co::TerminalKind(1,22|65536)==1&&co::TerminalKind(10,0)==0,"unconscious and alive replicas remain distinct");
    Check(co::TerminalKind(40,128)==0,"body modifiers alone do not mark a living actor dead");
    namespace ws=orebound::worldsync;
    auto probe=ws::MatchNpcProbe({},false);Check(probe.index<0&&probe.provenAbsent,"only empty candidate set proves NPC absence");
    probe=ws::MatchNpcProbe({9000000},false);Check(probe.index<0&&!probe.provenAbsent,"far existing NPC must not become deletion tombstone");
    probe=ws::MatchNpcProbe({0,0},true);Check(probe.index<0&&!probe.provenAbsent,"identical waypoint cluster remains unresolved without deletion");
    probe=ws::MatchNpcProbe({9000000},true);Check(probe.index==0&&!probe.provenAbsent,"unique native insertion waypoint binds moved NPC");
    probe=ws::MatchNpcProbe({100,40000},false);Check(probe.index==0,"unambiguous nearby anchor resolves without waypoint");
    probe=ws::MatchNpcProbe({std::numeric_limits<float>::quiet_NaN()},false);Check(probe.index<0&&!probe.provenAbsent,"invalid probe distance never proves absence");
}
