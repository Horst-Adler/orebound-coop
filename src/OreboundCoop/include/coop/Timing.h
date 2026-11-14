#pragma once
#include <cstdint>
namespace orebound::timing {
// Callbacks and journal I/O may publish a timestamp newer than a frame's
// cached clock. Such a sample is fresh, never an unsigned timeout wraparound.
inline std::uint64_t Elapsed(std::uint64_t now,std::uint64_t then) {
    return now>=then?now-then:0;
}
inline bool Expired(std::uint64_t now,std::uint64_t then,std::uint64_t limit) {
    return then!=0 && Elapsed(now,then)>limit;
}
}
