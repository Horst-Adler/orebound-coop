"""Verify the private archive, native-compatible texture and unchanged body."""
from pathlib import Path
import hashlib,importlib.util,json,sys
r=Path(__file__).resolve().parents[1];sys.path.insert(0,str(r/'tools/python-libs'))
import zenkit
c=json.loads((r/'config/avatar-clothing.json').read_text(encoding='utf-8'))
p=r/'build/alpha/avatar/OreboundCoop_Avatar.vdf'
assert c['Schema']==1 and c['File']=='Data/OreboundCoop_Avatar.vdf'
assert hashlib.sha256(p.read_bytes()).hexdigest().upper()==c['SHA256']
v=zenkit.Vfs();v.mount_disk(p)
name='_WORK/DATA/ANIMS/_COMPILED/ORB_BODY_BABE0.MDM';model=v.resolve(name).open().data
tex=v.resolve('_WORK/DATA/TEXTURES/_COMPILED/ORB_BODY_CLOTH_V0_C0-C.TEX').open().data
assert model==(r/'build/alpha/avatar/native-original.MDM').read_bytes().replace(b'HUM_BODY_BABE0',b'ORB_BODY_BABE0').replace(b'HUM_BODY_NAKED',b'ORB_BODY_CLOTH')
assert tex==(r/'build/alpha/avatar/ORB_BODY_CLOTH_V0_C0-C.TEX').read_bytes()
t=zenkit.Texture.load(r/'build/alpha/avatar/ORB_BODY_CLOTH_V0_C0-C.TEX')
assert (t.width,t.height,t.mipmap_count,t.format)==(512,512,1,zenkit.TextureFormat.DXT1)
assert set(t.mipmap_rgba(0)[3::4])=={255}
assert c['SHA256'] in (r/'src/OreboundCoop/include/coop/AvatarClothing.h').read_text(encoding='utf-8')
manifest=json.loads((r/'build/alpha/avatar/manifest.json').read_text(encoding='utf-8'))
assert len(manifest['Entries'])==2 and manifest['GeometryUVSkeletonUnchanged']
assert not any('HUM_HEAD' in x or 'HUM_BODY_NAKED' in x for x in manifest['Entries'])
print('PASS: clothing catalogue/hash, exact native body rename only, BC1 single-mip opaque decode and exactly two isolated archive resources; no original NPC texture overridden.')
