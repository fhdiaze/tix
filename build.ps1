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
$outdir = ".\bin"
if (!(Test-Path $outdir)) {
    New-Item -ItemType Directory -Path $outdir | Out-Null
}

$output = Join-Path $outdir "$base.exe"

# Read flags from file
$flags = Get-Content "compile_flags.txt" |
    Where-Object { $_.Trim() -ne "" -and -not $_.StartsWith("//") } |
    ForEach-Object { $_.Trim().TrimEnd(',') } |
    Where-Object { $_ -ne "" }
#$flags += "-fsanitize=address"
#$flags += "-o0"

clang @flags $SourceFile -o $output