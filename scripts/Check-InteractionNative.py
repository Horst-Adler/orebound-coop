"""Verify native key/dialog/gate hook contracts without executing the game."""
from pathlib import Path
import argparse,struct
root=Path(__file__).resolve().parents[1]
a=argparse.ArgumentParser();a.add_argument("--game-path",type=Path,default=root/"games/Gothic 2 Gold");args=a.parse_args()
data=(args.game_path/"System/Gothic2.exe").read_bytes()
pe=struct.unpack_from("<I",data,0x3c)[0];count=struct.unpack_from("<H",data,pe+6)[0]
optional=struct.unpack_from("<H",data,pe+20)[0];base=struct.unpack_from("<I",data,pe+52)[0];table=pe+24+optional

def code(address,size):
    for n in range(count):
        vs,va,rs,raw=struct.unpack_from("<IIII",data,table+n*40+8)
        if va<=address-base<va+max(vs,rs):return data[raw+address-base-va:raw+address-base-va+size]
    raise AssertionError(hex(address))

interact=code(0x723cf0,0x5f0)
assert bytes.fromhex("e8 36 54 02 00") in interact, "native key inventory check changed"
assert bytes.fromhex("83 e7 fe 6a 38 89 be 34 02 00 00") in interact, "direct key-unlock write changed"
assert bytes.fromhex("c2 18 00") in interact,"lock interaction must pop NPC plus five integer arguments"
assert bytes.fromhex("c2 08 00") in code(0x724a70,0x750),"standalone Unlock ABI changed"
assert bytes.fromhex("c2 04 00") in code(0x721c20,0x2e0),"StopInteraction ABI changed"
assert code(0x75a680,4)==bytes.fromhex("53 55 56 57"),"ProcessInfos entry changed"
assert code(0x42ad50,8)==bytes.fromhex("64 a1 00 00 00 00 6a ff"),"game manager input entry changed"
for address in [0x71e7d0,0x71eac0,0x613080,0x613170]:
    assert bytes.fromhex("c2 08 00") in code(address,0x300),hex(address)
assert code(0x611d90,8)==bytes.fromhex("64 a1 00 00 00 00 6a ff"),"native mover advance entry changed"
source=(root/"src/OreboundCoop/src/Plugin.cpp").read_text(encoding="utf-8-sig")
for address in [0x723cf0,0x75a680,0x42ad50,0x71e7d0,0x71eac0,0x613080,0x613170,0x611d90,0x612f80,0x6c0d80]:
    assert f"0x{address:08X}" in source,hex(address)
load=source.split("void BeforeLoad() {",1)[1].split("void BeforeNewGame",1)[0]
assert load.index("Leave(")<load.index("CharacterClearForms();"),"release character forms after bridge cleanup and before native load"
character=(root/"src/OreboundCoop/src/CharacterBridge.inl").read_text(encoding="utf-8-sig")
assert "npc->AddRef();characterForms.push_back(npc)" in character and "for(auto* npc:characterForms)npc->Release();" in character
print("PASS: direct native key branch, ten interaction/dialog/mover hook entries, stack cleanup and before-load reference lifecycle; no game executed.")
