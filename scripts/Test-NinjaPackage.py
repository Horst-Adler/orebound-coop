"""Regression for ignored underscore archives and mismatched Ninja folders."""
from NinjaPackage import validate_package,validate_catalogue_case
import struct
entries=['Ninja/OreboundCore/Content_G2.src','Ninja/OreboundCore/Content/Bridge.d','Ninja/OreboundCore/Menu_G2.src','Ninja/OreboundCore/Menu/Branding.d']
assert validate_package('OreboundCore.vdf',entries)=='OreboundCore'
assert validate_package('OREBOUNDCORE.VDF',entries)=='OREBOUNDCORE'
for name in ['OreboundCoop_Core.vdf','ZweiklangCoop_Core.vdf','1Orebound.vdf','Orebound-Core.vdf','Orebound Core.vdf','OreboundCore.zip']:
 try:validate_package(name,entries)
 except ValueError:pass
 else:raise AssertionError('Invalid Ninja name accepted: '+name)
for invalid in [[x.replace('OreboundCore','ZweiklangCore') for x in entries],entries[:-2],entries+[entries[0].upper()],entries+['Ninja/OreboundCore/../Other/Unsafe.d']]:
 try:validate_package('OreboundCore.vdf',invalid)
 except ValueError:pass
 else:raise AssertionError('Invalid Ninja layout accepted')
raw=bytearray(376);struct.pack_into('<I',raw,272,1);struct.pack_into('<I',raw,288,296);raw[296:301]=b'NINJA'
assert validate_catalogue_case(raw)==1
raw[296:301]=b'Ninja'
for invalid in [raw,raw[:300]]:
 try:validate_catalogue_case(invalid)
 except ValueError:pass
 else:raise AssertionError('Native-incompatible catalogue accepted')
print('PASS: 15 Ninja discovery/catalogue cases; names, old underscore regression, folder/basename matching, required entries, duplicates, traversal, uppercase native names and bounds.')
