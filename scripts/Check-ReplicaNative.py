"""Check the pinned engine contracts used by replica fixes; never launch Gothic."""
from pathlib import Path
import runpy,re,struct,json
r=Path(__file__).resolve().parents[1]
code=runpy.run_path(str(r/'scripts/Check-InteractionNative.py'))['code']
sdk=r/'src/GothicCoop/ZenGin/Gothic_II_Addon/API'
assert code(0x7808d0,5)==bytes.fromhex('3d1035ab00'), 'native oCVob-list class filter changed'
talk_gate=bytes.fromhex('668b4024663d08000f8469010000663d09000f845f010000')
assert code(0x6bd0a5,len(talk_gate))==talk_gate, 'native queued-item talk gate changed'
assert code(0x74acc0,12)==bytes.fromhex('83ec08568bf18b8668090000'), 'interaction item cleanup ABI changed'
assert code(0x576ca0,8)==bytes.fromhex('518b410885c08b01'), 'terminal animation progress ABI changed'
model=(sdk/'zModel.h').read_text();npc=(sdk/'oNpc.h').read_text()
assert re.search(r'void SetNodeVisual\( zCModelNodeInst\*, zCVisual\*, int \).*0x005792A0',model)
assert re.search(r'void SetProgressPercent\( float \).*0x00576CA0',model)
assert re.search(r'void SetInteractItem\( oCItem\* \).*0x0074ACC0',npc)
src=r/'src/OreboundCoop/src'
mechanism=(src/'MechanismBridge.inl').read_text(encoding='utf-8')
assert 'globalVobTree' in mechanism and '->voblist' not in mechanism
assert 'found->second.size()==1' in mechanism, 'ambiguous mover names must not pick a random gate'
world=(src/'WorldBridge.inl').read_text(encoding='utf-8')
intent=world.split('void WorldTalkInput(int key){',1)[1].split('void WorldTalkFrame',1)[0]
assert 'IsBinded(GAME_ACTION' in intent and 'GetFocusNpc()' in intent
assert '!it->second->mirror' in intent and 'HostCombat()' in intent
assert intent.index('WorldBeginTalk(npc,player)')<intent.index('em->Clear()')
character=(src/'CharacterBridge.inl').read_text(encoding='utf-8')
weapons=character.split('void CharacterApplyWeapons(){',1)[1].split('void CharacterApplyPeerVisual()',1)[0]
for banned in ['PutInInv','EquipWeapon(','PutInSlot(','AddVob(','inventory2.Insert']:
 assert banned not in weapons,banned
assert 'SetNodeVisual(node,visual,TRUE)' in weapons
combat=(src/'CombatBridge.inl').read_text(encoding='utf-8')
assert 'model->timeScale=0' in combat and 'entry->originalTimeScale' in combat
assert 'SetProgressPercent' in combat and 'co::TerminalKind' in combat
recovery=(r/'src/OreboundCoop/include/coop/Recovery.h').read_text(encoding='utf-8')
worker=recovery.split('write_=std::async',1)[1].split('return true;',1)[0]
assert 'AtomicWrite(path,bytes)' in worker and 'this' not in worker
assert 'FlushFileBuffers(h)' in recovery and 'MOVEFILE_WRITE_THROUGH' in recovery
print('PASS: complete native tree vs oCVob filter, queued-item talk gate, cleanup/progress/mesh ABI, explicit guest-only handoff, visual-only weapons and durable byte-only worker. Native play acceptance pending.')
