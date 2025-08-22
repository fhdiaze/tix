# Get the base filename without extension
$SourceFile = "./src/main.c"
$outdir = ".\bin"
$output = Join-Path $outdir "tix.exe"

# Read flags from file
$flags = Get-Content "compile_flags.txt" |
    Where-Object { $_.Trim() -ne "" -and -not $_.StartsWith("//") } |
    ForEach-Object { $_.Trim().TrimEnd(',') } |
    Where-Object { $_ -ne "" }
#$flags += "-fsanitize=address"
#$flags += "-o0"

clang @flags $SourceFile -o $output