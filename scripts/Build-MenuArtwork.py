"""Convert own menu art to a uniquely named opaque BC1 texture and VDF.

Only resize/format conversion; no creative editing. Replace only the requested
startup background; preserve original logos. Requires Pillow and zenkit.
"""
from pathlib import Path
import argparse,datetime,hashlib,io,json,struct,sys,array
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools/python-libs'))
from PIL import Image,ImageChops,ImageStat
import zenkit
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--image',required=True,type=Path)
parser.add_argument('--output',type=Path,default=root/'build/alpha/menu')
parser.add_argument('--loading-artwork-dir',type=Path,default=root/'assets/loading-screens/approved',help='Directory containing the three approved world illustrations; absent directory retains the generic fallback.')
args=parser.parse_args();source=args.image.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
def pixels_of(im):
 return im.get_flattened_data() if hasattr(im,'get_flattened_data') else im.getdata()
image=Image.open(source).convert('RGB').resize((2048,1024),Image.Resampling.LANCZOS)
mips=[image]
# Native GetTextureBuffer rejects compressed mip dimensions not divisible by
# four, even when an offline DDS decoder accepts them. As with stock menu art,
# use one full-resolution mip; the native view scales the image to the screen.
encoded=[];decoded=[]
for im in mips:
 assert im.width%4==0 and im.height%4==0,'Native DXT block dimensions must be divisible by four'
 stream=io.BytesIO();im.save(stream,format='DDS',pixel_format='DXT1');data=stream.getvalue()
 assert data[:4]==b'DDS ' and data[84:88]==b'DXT1'
 expected=((im.width+3)//4)*((im.height+3)//4)*8
 assert len(data[128:])==expected,(im.size,len(data[128:]),expected)
 encoded.append(data[128:]);decoded.append(Image.open(io.BytesIO(data)).convert('RGBA'))
red,green,blue=mips[-1].getpixel((0,0))
blob=struct.pack('<4s8I',b'ZTEX',0,int(zenkit.TextureFormat.DXT1),image.width,image.height,len(mips),image.width,image.height,(255<<24)|(red<<16)|(green<<8)|blue)+b''.join(reversed(encoded))
texture_name='OREBOUND_COOP_MENU-C.TEX';texture_path=out/texture_name;texture_path.write_bytes(blob)
check=zenkit.Texture.load(texture_path)
assert (check.width,check.height,check.mipmap_count)==(2048,1024,1)
assert check.format==zenkit.TextureFormat.DXT1
for level,im in enumerate(decoded):
 native=Image.frombytes('RGBA',im.size,check.mipmap_rgba(level))
 assert native.getchannel('A').getextrema()==(255,255),('nonopaque mip',level)
 assert max(hi for lo,hi in ImageStat.Stat(ImageChops.difference(native,im)).extrema)<=1,level
quality=ImageStat.Stat(ImageChops.difference(image,decoded[0].convert('RGB'))).mean
assert max(quality)<8,quality
# InitScreen_Open uses STARTSCREEN.TGA before any zCMenu is created. Match its
# original 1024-square RGB565/single-mip format; do not alter either logo texture.
boot=Image.open(source).convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS)
pixels=array.array('H',(((red>>3)<<11)|((green>>2)<<5)|(blue>>3) for red,green,blue in pixels_of(boot)))
if sys.byteorder!='little':pixels.byteswap()
boot_blob=struct.pack('<4s8I',b'ZTEX',0,8,1024,1024,1,1024,1024,0)+pixels.tobytes()
boot_name='STARTSCREEN-C.TEX';boot_path=out/boot_name;boot_path.write_bytes(boot_blob)
boot_check=zenkit.Texture.load(boot_path)
assert (boot_check.width,boot_check.height,boot_check.mipmap_count,int(boot_check.format))==(1024,1024,1,8)
boot_decoded=Image.frombytes('RGBA',(1024,1024),boot_check.mipmap_rgba(0))
assert boot_decoded.getchannel('A').getextrema()==(255,255)
boot_error=ImageStat.Stat(ImageChops.difference(boot,boot_decoded.convert('RGB'))).mean
assert max(boot_error)<8,boot_error
loading_names=['LOADING-C.TEX','LOADING_NEWWORLD-C.TEX','LOADING_OLDWORLD-C.TEX','LOADING_ADDONWORLD-C.TEX','LOADING_DRAGONISLAND-C.TEX']
# Native OpenLoadscreen selects loading_<world>.tga, with loading.tga as
# fallback. Keep its stock RGB565 layout and leave progress bars/logos alone.
textures={texture_name:blob,boot_name:boot_blob,**{name:boot_blob for name in loading_names}}
world_sources={
 'LOADING_OLDWORLD-C.TEX':('Minental','Orebound-Co-op-Minental-v2.png','ED10D42FDF69E978DD81E6F51CE19A8787E68C16670934288219917BD2719A10'),
 'LOADING_ADDONWORLD-C.TEX':('Jharkendar','Orebound-Co-op-Jharkendar-v5.png','55BCF98B2493DAF92BCA6140C3373222CD62140FAD6A780914469CB01570D0F7'),
 'LOADING_DRAGONISLAND-C.TEX':('Irdorath','Orebound-Co-op-Irdorath-v2.png','6995C7A179E2C50EE8F7ACD36871977DA3792764430C2F202ADFE1EF00546A62'),
}
loading_metadata={}
world_directory=args.loading_artwork_dir.resolve()
if world_directory.exists():
 # An incomplete or modified approved set must fail instead of silently
 # replacing a region with the wrong illustration. PNG originals stay intact.
 for name,(world,filename,approved_hash) in world_sources.items():
  path=world_directory/filename
  assert path.is_file(),('Approved world artwork missing',filename)
  assert hashlib.sha256(path.read_bytes()).hexdigest().upper()==approved_hash,('Approved image changed',filename)
  world_image=Image.open(path).convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS)
  world_pixels=array.array('H',(((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in pixels_of(world_image)))
  if sys.byteorder!='little':world_pixels.byteswap()
  data=struct.pack('<4s8I',b'ZTEX',0,8,1024,1024,1,1024,1024,0)+world_pixels.tobytes()
  texture=zenkit.Texture.load(data)
  assert (texture.width,texture.height,texture.mipmap_count,int(texture.format))==(1024,1024,1,8)
  decoded_world=Image.frombytes('RGBA',(1024,1024),texture.mipmap_rgba(0))
  assert decoded_world.getchannel('A').getextrema()==(255,255)
  error=ImageStat.Stat(ImageChops.difference(world_image,decoded_world.convert('RGB'))).mean
  assert max(error)<8,('World image color conversion',world,error)
  textures[name]=data
  (out/name).write_bytes(data)
  loading_metadata[name]={'World':world,'Source':filename,'SourceSHA256':approved_hash,'TextureSHA256':hashlib.sha256(data).hexdigest().upper(),'MeanAbsoluteRGBError':error,'Size':[1024,1024],'Format':'RGB565','Mipmaps':1}
for name in ['LOADING-C.TEX','LOADING_NEWWORLD-C.TEX']:
 loading_metadata[name]={'World':'Generic' if name=='LOADING-C.TEX' else 'Khorinis','Source':'Main artwork','SourceSHA256':hashlib.sha256(source.read_bytes()).hexdigest().upper(),'TextureSHA256':hashlib.sha256(textures[name]).hexdigest().upper(),'Size':[1024,1024],'Format':'RGB565','Mipmaps':1}
if loading_metadata.keys()>=world_sources.keys():
 assert len({hashlib.sha256(textures[name]).digest() for name in world_sources})==3,'World illustrations must be distinct'
 assert all(textures[name]!=boot_blob for name in world_sources),'Worlds must not reuse the generic image'
for name in loading_names:(out/name).write_bytes(textures[name])
vfs=zenkit.Vfs();stamp=datetime.datetime(2026,11,14,15,0,tzinfo=datetime.timezone.utc)
folder=vfs.mkdir('_WORK/DATA/TEXTURES/_COMPILED')
for name,data in textures.items():folder.create(name,data,timestamp=stamp)
target=out/'OreboundCoop_Menu.vdf';vfs.save(target,zenkit.GameVersion.GOTHIC2,stamp)
mount=zenkit.Vfs();mount.mount_disk(target)
for name,data in textures.items():
 node=mount.resolve('_WORK/DATA/TEXTURES/_COMPILED/'+name)
 assert node and node.is_file() and node.open().data==data
raw=target.read_bytes();count=struct.unpack_from('<I',raw,272)[0];catalog=struct.unpack_from('<I',raw,288)[0];entries=[]
for at in range(catalog,catalog+count*80,80):
 name=raw[at:at+64].decode('ascii').strip('\0 ');offset,size,flags,_=struct.unpack_from('<4I',raw,at+64)
 if not flags&0x80000000:assert raw[offset:offset+size]==textures[name];entries.append(name)
assert set(entries)==set(textures),'Only the own menu image, startup and approved loading backgrounds may be included'
sha=hashlib.sha256(raw).hexdigest().upper()
catalogue=dict(Schema=1,Optional=True,File='Data/OreboundCoop_Menu.vdf',SHA256=sha,Texture='OREBOUND_COOP_MENU.TGA',MenuABI=1,SourceSHA256=hashlib.sha256(source.read_bytes()).hexdigest().upper(),ArtworkRevision=2 if world_directory.exists() else 1,WorldSpecificLoading=world_directory.exists())
(root/'config/menu-artwork.json').write_text(json.dumps(catalogue,indent=2)+'\n',encoding='utf-8')
header='#pragma once\nnamespace orebound::menuart { inline constexpr const char* ArchiveHash="'+sha+'"; inline constexpr const char* TextureName="OREBOUND_COOP_MENU.TGA"; inline constexpr int MenuABI=1; }\n'
header_path=root/'src/OreboundCoop/include/coop/MenuArtworkHash.h'
if not header_path.exists() or header_path.read_text(encoding='utf-8')!=header:header_path.write_text(header,encoding='utf-8')
manifest=dict(catalogue,Bytes=len(raw),TextureSize=[2048,1024],Mipmaps=len(mips),Format='Opaque DXT1 / BC1',StartupTexture=dict(File=boot_name,Size=[1024,1024],Format='RGB565',Mipmaps=1,MeanAbsoluteRGBError=boot_error),LoadingTextures=loading_names,LoadingArtwork=loading_metadata,Entries=entries,AllMipPixelsVerified=True,IndependentVdfMountVerified=True,CompressionMeanAbsoluteRGBError=quality,OriginalResourcesOverridden=True,OverriddenOriginalResources=[boot_name]+loading_names,OriginalLogoResourcesOverridden=False,GamesStarted=False,NativeVisualAcceptance=False)
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('PASS: opaque single-mip BC1 menu plus stock-format RGB565 startup background; colors/VDF verified; original logos untouched. RGB error:',quality)
print('World loading artwork:',', '.join(item['World'] for item in loading_metadata.values()))
