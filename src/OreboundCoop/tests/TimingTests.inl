void TestFrameTiming(){
    namespace t=orebound::timing;
    Check(t::Elapsed(225000250,225000296)==0,"journal callback newer than frame is fresh, not UINT64 timeout");
    Check(!t::Expired(225000250,225000296,15000),"live-test journal commit does not expire its new loot lease");
    Check(!t::Expired(225000296,225000296,15000),"same tick does not expire a lease");
    Check(!t::Expired(225015296,225000296,15000),"lease remains valid at exact timeout boundary");
    Check(t::Expired(225015297,225000296,15000),"real overdue lease still expires");
    Check(!t::Expired(100000,0,15000),"unset lease timestamp is not an elapsed lease");
    Check(t::Elapsed(UINT64_MAX,UINT64_MAX-15)==15,"large monotonic uptime has no narrowing");
    Check(t::Elapsed(20,UINT64_MAX-15)==0,"clock rollback does not wrap into a timeout");
    for(unsigned delay:{0u,1u,16u,44u,250u,15001u}){
        std::uint64_t frame=225000250,traffic=frame+delay;
        Check(!t::Expired(frame,traffic,15000),"fresh callback survives every simulated journal stall");
        Check(t::Expired(traffic+15001,traffic,15000),"fresh callback eventually expires without keepalive");
        Check(!t::Expired(traffic+15001,traffic+15001,15000),"keepalive refresh prevents a genuine expiry");
    }
}
void TestBackgroundReadiness(){
    namespace c=orebound::character;
    c::CounterMessage message;message.timeline=std::string(32,'a');
    message.op=c::CounterOp::Snapshot;Check(!c::BlockingOutbound(c::Encode(message)),"periodic unchanged snapshot does not flicker inventory readiness");
    message.op=c::CounterOp::Receipt;Check(!c::BlockingOutbound(c::Encode(message)),"background receipt alone does not block loot");
    message.op=c::CounterOp::Hello;Check(c::BlockingOutbound(c::Encode(message)),"initial character handshake still blocks readiness");
    message.op=c::CounterOp::Delta;message.ack=1;Check(c::BlockingOutbound(c::Encode(message)),"unsent personal change still blocks readiness");
    Check(c::BlockingOutbound({})&&c::BlockingOutbound({70,2}),"malformed outbound state is never treated as background traffic");
    c::Summon summon;summon.spell=22;summon.token=std::string(32,'b');Check(c::BlockingOutbound(c::Encode(summon)),"queued summon is still a blocking state change");
    std::array<float,3> anchor{0,0,0};bool fixed=false;
    Check(orebound::worldsync::CaptureSpawnAnchor(anchor,fixed,std::array<float,3>{100,200,300})&&fixed&&anchor[0]==100,"dormant NPC captures its actual first spawn position");
    Check(!orebound::worldsync::CaptureSpawnAnchor(anchor,fixed,std::array<float,3>{900,800,700})&&anchor[0]==100,"NPC culling cannot move its permanent matching anchor");
}
