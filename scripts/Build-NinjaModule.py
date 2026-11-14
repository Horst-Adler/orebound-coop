"""Build only own Ninja sources; requires separately installed zenkit 1.3.0.4."""
from pathlib import Path
import sys,datetime,hashlib,json
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools/python-libs'))
import zenkit
from NinjaPackage import validate_package,validate_catalogue_case
source=root/'src/OreboundCoop/ninja';out=root/'build/alpha/ninja';out.mkdir(parents=True,exist_ok=True)
stamp=datetime.datetime(2026,11,14,tzinfo=datetime.timezone.utc)
vfs=zenkit.Vfs();entries=[]
for file in sorted(source.rglob('*')):
 if not file.is_file():continue
 rel=file.relative_to(source).as_posix();data=file.read_text(encoding='ascii').replace('\r\n','\n').encode('ascii');folder,name=rel.rsplit('/',1)
 vfs.mkdir(folder.upper()).create(name.upper(),data,timestamp=stamp);entries.append((rel,data))
assert {x[0] for x in entries}=={'Ninja/OreboundCore/Content_G2.src','Ninja/OreboundCore/Content/Bridge.d','Ninja/OreboundCore/Menu_G2.src','Ninja/OreboundCore/Menu/Branding.d'},'Unexpected Ninja source files'
target=out/'OreboundCore.vdf'
validate_package(target.name,[x[0] for x in entries])
vfs.save(target,zenkit.GameVersion.GOTHIC2,stamp)
validate_catalogue_case(target.read_bytes())
mount=zenkit.Vfs();mount.mount_disk(target)
for rel,data in entries:
 node=mount.resolve(rel);assert node and node.is_file() and node.open().data==data,rel
sha=hashlib.sha256(target.read_bytes()).hexdigest().upper()
header=root/'src/OreboundCoop/include/coop/NinjaArchiveHash.h'
text='#pragma once\nnamespace orebound::extensions { inline constexpr const char* CoreArchiveHash="'+sha+'"; }\n'
if not header.exists() or header.read_text(encoding='utf-8')!=text:header.write_text(text,encoding='utf-8',newline='\n')
(out/'manifest.json').write_text(json.dumps(dict(ABI=2,File=target.name,PatchName=target.stem,NinjaDiscoveryValidated=True,CatalogueUppercaseValidated=True,SHA256=sha,Bytes=target.stat().st_size,Entries=[x[0] for x in entries],GamesStarted=False),indent=2),encoding='utf-8')
print('PASS: deterministic own Ninja VDF; four entries independently mounted and byte-verified; ABI 2; no foreign sources included.')
