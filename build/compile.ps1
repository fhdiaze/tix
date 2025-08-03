param(
    [Parameter(Mandatory=$true)]
    [string]$SourceFile
)

if (!(Test-Path $SourceFile)) {
    Write-Host "Source file not found: $SourceFile"
    exit 1
}

# Get the base filename without extension
$base = [System.IO.Path]::GetFileNameWithoutExtension($SourceFile)
$outdir = ".\target"
if (!(Test-Path $outdir)) {
    New-Item -ItemType Directory -Path $outdir | Out-Null
}

$output = Join-Path $outdir "$base.exe"

$flags = @()
$flags += "-std=c23"
$flags += "-Wall"
$flags += "-Werror"
$flags += "-Wextra"
$flags += "-Wsign-compare"
$flags += "-xc"
$flags += "-fuse-ld=lld"
$flags += "-pedantic"
#$flags += "-fsanitize=address"
#$flags += "-o0"

clang @flags $SourceFile -o $output