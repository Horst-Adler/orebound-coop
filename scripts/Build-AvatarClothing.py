"""Package existing approved art as opaque BC1 and a private native G2 body.

Format conversion only. No generated geometry, stat changes or game execution.
The mesh is derived locally from the user's Gothic II installation.
"""
from pathlib import Path
import argparse,datetime,hashlib,io,json,struct,sys
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'tools/python-libs'))
import zenkit
from PIL import Image,ImageChops,ImageStat
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--game-path',type=Path,default=root/'games/Gothic 2 Gold')
p.add_argument('--image',type=Path,default=root/'assets/velaya-g2/source/Velaya-Leder-Stoff.png')
p.add_argument('--output',type=Path,default=root/'build/alpha/avatar')
args=p.parse_args();out=args.output;out.mkdir(parents=True,exist_ok=True)
archive=(args.game_path/'Data/Anims.vdf').read_bytes()
count=struct.unpack_from('<I',archive,272)[0];catalog=struct.unpack_from('<I',archive,288)[0]
assert catalog+count*80<=len(archive),'Native animation archive catalogue truncated'
matches=[]
for at in range(catalog,catalog+count*80,80):
 name=archive[at:at+64].decode('ascii').strip('\0 ')
 offset,length,flags,_=struct.unpack_from('<4I',archive,at+64)
 if not flags&0x80000000 and name.upper()=='HUM_BODY_BABE0.MDM':
  assert offset+length<=len(archive)
  matches.append(archive[offset:offset+length])
assert len(matches)==1,'Expected a unique native Gothic II female body'
original=matches[0]
mesh=original
for before,after in [(b'HUM_BODY_BABE0',b'ORB_BODY_BABE0'),(b'HUM_BODY_NAKED',b'ORB_BODY_CLOTH')]:
 assert len(before)==len(after) and before in mesh
 mesh=mesh.replace(before,after)
assert len(mesh)==len(original)
base=out/'native-original.MDM';base.write_bytes(original)
model=out/'ORB_BODY_BABE0.MDM';model.write_bytes(mesh)
before=zenkit.ModelMesh.load(base);after=zenkit.ModelMesh.load(model)
assert before.checksum==after.checksum and len(before.meshes)==len(after.meshes)
for x,y in zip(before.meshes,after.meshes):
 assert [bytes(v) for v in x.mesh.positions]==[bytes(v) for v in y.mesh.positions]
 assert [bytes(v) for v in x.mesh.normals]==[bytes(v) for v in y.mesh.normals]
 for left,right in zip(x.mesh.submeshes,y.mesh.submeshes):
  assert [v.wedges for v in left.triangles]==[v.wedges for v in right.triangles]
  assert [(bytes(v.texture),v.index) for v in left.wedges]==[(bytes(v.texture),v.index) for v in right.wedges]
 assert all(v.name.startswith('ORB_BODY_BABE0') and v.texture=='ORB_BODY_CLOTH_V0_C0.TGA' for v in y.mesh.material)
im=Image.open(args.image).convert('RGB').resize((512,512),Image.Resampling.LANCZOS)
stream=io.BytesIO();im.save(stream,format='DDS',pixel_format='DXT1');dds=stream.getvalue()
assert dds[:4]==b'DDS ' and dds[84:88]==b'DXT1' and len(dds[128:])==512*512//2
r,g,b=im.resize((1,1),Image.Resampling.BOX).getpixel((0,0))
texture=struct.pack('<4s8I',b'ZTEX',0,int(zenkit.TextureFormat.DXT1),512,512,1,512,512,0xff000000|(r<<16)|(g<<8)|b)+dds[128:]
texture_path=out/'ORB_BODY_CLOTH_V0_C0-C.TEX';texture_path.write_bytes(texture)
decoded=zenkit.Texture.load(texture_path)
assert (decoded.width,decoded.height,decoded.mipmap_count,decoded.format)==(512,512,1,zenkit.TextureFormat.DXT1)
pixels=Image.frombytes('RGBA',(512,512),decoded.mipmap_rgba(0))
assert pixels.getchannel('A').getextrema()==(255,255)
reference=Image.open(io.BytesIO(dds)).convert('RGBA')
assert max(hi for lo,hi in ImageStat.Stat(ImageChops.difference(pixels,reference)).extrema)<=1
quality=ImageStat.Stat(ImageChops.difference(im,pixels.convert('RGB'))).mean
assert max(quality)<12,quality
stamp=datetime.datetime(2026,11,14,22,0,tzinfo=datetime.timezone.utc)
entries={'_WORK/DATA/ANIMS/_COMPILED/'+model.name:mesh,'_WORK/DATA/TEXTURES/_COMPILED/'+texture_path.name:texture}
vfs=zenkit.Vfs()
for name,data in entries.items():folder,leaf=name.rsplit('/',1);vfs.mkdir(folder).create(leaf,data,timestamp=stamp)
target=out/'OreboundCoop_Avatar.vdf';vfs.save(target,zenkit.GameVersion.GOTHIC2,stamp)
mount=zenkit.Vfs();mount.mount_disk(target)
for name,data in entries.items():assert mount.resolve(name).open().data==data
sha=hashlib.sha256(target.read_bytes()).hexdigest().upper()
catalogue=dict(Schema=1,Optional=True,File='Data/OreboundCoop_Avatar.vdf',SHA256=sha,Body='ORB_BODY_BABE0',Texture='ORB_BODY_CLOTH_V0_C0.TGA',BodyTextureVariant=0)
(root/'config/avatar-clothing.json').write_text(json.dumps(catalogue,indent=2)+'\n',encoding='utf-8')
header='#pragma once\nnamespace orebound::avatar { inline constexpr const char* ArchiveHash="'+sha+'"; inline constexpr const char* Body="ORB_BODY_BABE0"; }\n'
path=root/'src/OreboundCoop/include/coop/AvatarClothing.h'
if not path.exists() or path.read_text(encoding='utf-8')!=header:path.write_text(header,encoding='utf-8')
manifest=dict(catalogue,Entries=list(entries),Bytes=target.stat().st_size,SourceSHA256=hashlib.sha256(args.image.read_bytes()).hexdigest().upper(),NativeMeshSHA256=hashlib.sha256(original).hexdigest().upper(),GeometryUVSkeletonUnchanged=True,Format='DXT1',Size=[512,512],Mipmaps=1,Opaque=True,MeanAbsoluteRGBError=quality,GamesStarted=False,NativeVisualAcceptance=False)
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('PASS: private native female mesh unchanged geometrically; approved clothing encoded opaque BC1, single mip; independent texture decode and archive mount verified; no game started.')
