#pragma once
#include <algorithm>
namespace orebound::inventory_input {
// The native chest/corpse HandleEvent action path removes an item directly;
// it does not invoke TransferItem. Keep its input/quantity policy testable.
inline bool Action(bool primary,bool secondary,bool leftMouse,bool closing){
    return !closing&&(primary||secondary||leftMouse);
}
inline unsigned Amount(unsigned selected,int transferCount,bool firstPress,bool all,bool multiple){
    if(!selected)return 0;
    if(!multiple||(firstPress&&!all))return 1;
    if(all)return selected;
    const unsigned count=transferCount>=10000?10000:transferCount>=1000?1000:transferCount>=100?100:transferCount>=10?10:1;
    return (std::min)(selected,count);
}
}
