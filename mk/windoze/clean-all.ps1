"Cleaning build files."
Remove-Item $(Join-Path $PSScriptRoot *.exe)
Remove-Item -Recurse $(Join-Path $PSScriptRoot build\)
"Cleaning build files completed."
