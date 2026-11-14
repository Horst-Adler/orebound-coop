"""Ninja discovery rules from the official patch-structure documentation."""
from pathlib import PurePosixPath
import re
import struct

def validate_package(archive,entries):
 name=PurePosixPath(archive).name
 if not re.fullmatch(r'[A-Za-z][A-Za-z0-9]*\.vdf',name,re.IGNORECASE):
  raise ValueError('Ninja archive names must start with a letter and contain only ASCII letters/digits')
 stem=name[:-4];prefix=('Ninja/'+stem+'/').lower()
 paths=list(entries)
 if not paths or len(set(x.casefold() for x in paths))!=len(paths):raise ValueError('Empty or duplicate Ninja source paths')
 for path in paths:
  parts=PurePosixPath(path).parts
  if '..' in parts or not path.lower().startswith(prefix):raise ValueError('Ninja directory must match archive basename: '+stem)
 required={('ninja/'+stem+'/content_g2.src').lower(),('ninja/'+stem+'/menu_g2.src').lower()}
 if not required.issubset({x.lower() for x in paths}):raise ValueError('Own content/menu source entry is missing')
 return stem

def validate_catalogue_case(raw):
 if len(raw)<296:raise ValueError('Truncated VDF header')
 count=struct.unpack_from('<I',raw,272)[0];start=struct.unpack_from('<I',raw,288)[0]
 if not count or start<296 or start+count*80>len(raw):raise ValueError('Invalid VDF catalogue bounds')
 for at in range(start,start+count*80,80):
  name=raw[at:at+64].rstrip(b'\0 ').decode('ascii')
  if not name or name!=name.upper():raise ValueError('Native VDFS catalogue names must be uppercase: '+name)
 return count
