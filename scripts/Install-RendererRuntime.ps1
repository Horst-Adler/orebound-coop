[CmdletBinding(SupportsShouldProcess=$true)]
param(
 [Parameter(Mandatory=$true)][string]$GamePath,
 [string]$ArchivePath=(Join-Path (Split-Path -Parent $PSScriptRoot) 'downloads\GD3D11-v17.9.7.zip'),
 [string]$BackupRoot=(Join-Path (Split-Path -Parent $PSScriptRoot) 'local\renderer-backups'),
 [switch]$CheckOnly
)
$ErrorActionPreference='Stop'
if(Get-Process Gothic2 -ErrorAction SilentlyContinue){throw 'Close Gothic before renderer maintenance.'}
$game=[IO.Path]::GetFullPath($GamePath).TrimEnd('\');$system=Join-Path $game 'System'
if(-not(Test-Path -LiteralPath (Join-Path $system 'Gothic2.exe'))){throw 'Not a Gothic II installation.'}
$expected='44CA8EA5966CF5E793525CCA6385114D67016FAFBF66B10E23C1F15B811F9BD0'
if((Get-FileHash -LiteralPath $ArchivePath).Hash -ne $expected){throw 'Supply the documented original GD3D11 v17.9.7 archive.'}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($ArchivePath))
try{
 $rows=@();$seen=@{}
 foreach($entry in $zip.Entries){
  if($entry.FullName.EndsWith('/')){continue}
  $relative=$entry.FullName.Replace('/','\');$target=[IO.Path]::GetFullPath((Join-Path $system $relative))
  if([IO.Path]::IsPathRooted($relative) -or $relative.Split('\') -contains '..' -or -not $target.StartsWith($system+'\',[StringComparison]::OrdinalIgnoreCase) -or $seen.ContainsKey($relative)){throw 'Unsafe/duplicate renderer entry.'}
  $seen[$relative]=$true
  if([IO.Path]::GetExtension($relative) -in '.ini','.json','.xml'){throw 'Unexpected personal configuration in pinned archive.'}
  $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
  try{$hash=([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}
  $exists=Test-Path -LiteralPath $target -PathType Leaf;$before=if($exists){(Get-FileHash -LiteralPath $target).Hash}else{$null}
  $rows+=[ordered]@{Relative=$relative;Target=$target;Existed=$exists;BeforeSHA256=$before;SHA256=$hash;Changed=($before -ne $hash)}
 }
 if(-not $seen.ContainsKey('ddraw.dll')){throw 'Renderer entry point missing.'}
 $changed=@($rows|Where-Object Changed);$missing=@($rows|Where-Object {-not $_.Existed})
 if($CheckOnly){[ordered]@{GamePath=$game;Version='17.9.7';Files=$rows.Count;Changed=$changed.Count;Missing=$missing.Count;Ready=($changed.Count -eq 0)}|ConvertTo-Json -Compress;return}
 if(-not $PSCmdlet.ShouldProcess($game,'Back up and restore GD3D11 files; preserve all personal settings')){return}
 $backup=Join-Path ([IO.Path]::GetFullPath($BackupRoot)) ((Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
 New-Item -ItemType Directory -Path $backup -Force|Out-Null
 $configuration=@(Get-ChildItem -LiteralPath $system -File -Recurse|Where-Object {$_.Extension -in '.ini','.json','.xml' -and $_.FullName -notmatch '\\(ZweiklangLogs|ZweiklangCoopState|OreboundLogs|OreboundCoopState)\\'}|ForEach-Object {[ordered]@{File=$_.FullName;SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash}})
 foreach($row in $changed){if($row.Existed){$saved=Join-Path ($backup+'\Files') $row.Relative;New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force|Out-Null;Copy-Item -LiteralPath $row.Target -Destination $saved;if((Get-FileHash -LiteralPath $saved).Hash -ne $row.BeforeSHA256){throw 'Renderer backup verification failed.'}}}
 $manifest=[ordered]@{Schema=2;Version='17.9.7';GamePath=$game;ArchiveSHA256=$expected;Source='https://github.com/kirides/GD3D11/releases/tag/v17.9.7';CreatedAt=(Get-Date -Format o);Files=$changed;Configurations=$configuration;GameStarted=$false;Verified=$false}
 $manifestPath=Join-Path $backup 'manifest.json';$manifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $manifestPath -Encoding UTF8
 foreach($row in $changed){
  New-Item -ItemType Directory -Path (Split-Path -Parent $row.Target) -Force|Out-Null
  $entry=$zip.GetEntry($row.Relative.Replace('\','/'));$stream=$entry.Open();$temp=$row.Target+'.orebound-renderer-new'
  if(Test-Path -LiteralPath $temp){throw 'Existing renderer temporary file requires inspection.'}
  try{$outStream=[IO.File]::Create($temp);try{$stream.CopyTo($outStream)}finally{$outStream.Dispose()}}finally{$stream.Dispose()}
  if((Get-FileHash -LiteralPath $temp).Hash -ne $row.SHA256){throw 'Renderer extraction hash mismatch.'}
  Move-Item -LiteralPath $temp -Destination $row.Target -Force
 }
 foreach($row in $rows){if((Get-FileHash -LiteralPath $row.Target).Hash -ne $row.SHA256){throw ('Renderer verification failed: '+$row.Relative)}}
 foreach($row in $configuration){if((Get-FileHash -LiteralPath $row.File).Hash -ne $row.SHA256){throw 'Personal configuration unexpectedly changed.'}}
 $manifest.Verified=$true;$manifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $manifestPath -Encoding UTF8
 [ordered]@{GamePath=$game;Version='17.9.7';Files=$rows.Count;Restored=$changed.Count;MissingBefore=$missing.Count;ConfigurationsUnchanged=$configuration.Count;BackupManifest=$manifestPath;GameStarted=$false;Verified=$true}|ConvertTo-Json -Compress
}finally{$zip.Dispose()}
