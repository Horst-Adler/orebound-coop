[CmdletBinding(SupportsShouldProcess=$true)]
param([Parameter(Mandatory=$true)][string]$ManifestPath)
$ErrorActionPreference='Stop'
if(Get-Process Gothic2 -ErrorAction SilentlyContinue){throw 'Close Gothic before renderer rollback.'}
$manifest=Get-Content -LiteralPath $ManifestPath -Raw|ConvertFrom-Json
if($manifest.Schema -ne 2){throw 'Use the matching historical restore tool for an older manifest.'}
$base=[IO.Path]::GetFullPath((Join-Path $manifest.GamePath 'System')).TrimEnd('\');$backup=Split-Path -Parent ([IO.Path]::GetFullPath($ManifestPath))
foreach($row in $manifest.Files){
 $target=[IO.Path]::GetFullPath((Join-Path $base $row.Relative))
 if(-not $target.StartsWith($base+'\',[StringComparison]::OrdinalIgnoreCase) -or $target -ne $row.Target){throw 'Renderer restore target outside declared installation.'}
 if((Get-FileHash -LiteralPath $target).Hash -ne $row.SHA256){throw 'Renderer changed after installation; inspect before restoring.'}
 if($row.Existed){$saved=Join-Path ($backup+'\Files') $row.Relative;if((Get-FileHash -LiteralPath $saved).Hash -ne $row.BeforeSHA256){throw 'Original renderer backup hash mismatch.'}}
}
if(-not $PSCmdlet.ShouldProcess($base,'Restore only renderer files recorded in this backup')){return}
foreach($row in $manifest.Files){if($row.Existed){Copy-Item -LiteralPath (Join-Path ($backup+'\Files') $row.Relative) -Destination $row.Target -Force}else{Remove-Item -LiteralPath $row.Target}}
Write-Output 'Renderer rollback complete; personal settings and game state retained.'
