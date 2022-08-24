# Build MegaGlest on Windows.
# Author: James Sherratt.
param(${vcpkg-location})

$sword = [char]::ConvertFromUtf32(0x2694)
Write-Output "=====$sword MegaGlest $sword====="
""
<#
.SYNOPSIS
#

.DESCRIPTION
Test if a command exists.

.PARAMETER cmdName
Comamnd to test

.PARAMETER errorAdvice
Error advice to give user if command fails

.PARAMETER errorAction
(optional) Etion after error

.EXAMPLE
Test-Command "7z" "7z not found. Please install from 7zip.org" 'Write-Warning "7zip will not be used."'

.NOTES
General notes
#>
function Test-Command {
    param(
        $cmdName,
        $errorAdvice,
        $errorAction
    )

    if (Get-Command $cmdName -errorAction SilentlyContinue) {
        "Found $cmdName."
        ""
    }
    else {
        "The command '$cmdName' does not exist."
        "$errorAdvice"
        ""

        if ( !$errorAction ) {
            Exit
        }
        else {
            Invoke-Expression $errorAction
        }
    }
}

function Write-Title {
    param (
        $titleText
    )
    $titleText
    "-" * $titleText.Length
}

Write-Title "Updating git source"

Test-Command "git" "Please download and install git-scm https://git-scm.com/"
git pull
""

Write-Title "Setup vcpkg"
Test-Command "cmake" "Please download and install CMake: https://cmake.org/download/. (For 64 bit windows, select 'cmake-x.y.z-windows-x86_64.msi'.)"

if ( !${vcpkg-location} ) {
    ${vcpkg-location} = Join-Path $PSScriptRoot \vcpkg
    "Vcpkg location not set. Setting it to ${vcpkg-location}."
}
else {
    ${vcpkg-location} = $(Resolve-Path ${vcpkg-location}).ToString()
}

if ( Test-Path ${vcpkg-location} ) {
    "Found vcpkg."
}
else {
    "Vcpkg not found. Cloning. https://github.com/microsoft/vcpkg.git."
    git clone "https://github.com/microsoft/vcpkg.git" ${vcpkg-location}
    "Installing vcpkg."
    & "$(Join-Path ${vcpkg-location} bootstrap-vcpkg.bat)"
}

"Installing vcpkg and MegaGlest dependencies."
Set-Location ${vcpkg-location}
& "$(Join-Path $PSScriptRoot install-deps-vcpkg.ps1)"
if (!$?) {
    "Installing deps with vcpkg failed. Please check your vcpkg git repo is configured correctly."
    Set-Location $PSScriptRoot
    Exit
}
Set-Location $PSScriptRoot
""

Write-Title "Build MegaGlest"

$toolchainPath = $(Join-Path ${vcpkg-location} \scripts\buildsystems\vcpkg.cmake)
$buildFolder = $(Join-Path $PSScriptRoot build)
$topLevelTargetDir = $(Resolve-Path $(Join-Path $PSScriptRoot ../../)).ToString()

$vsVersion=(msbuild --version | select -Last 1).Split(".")[0] -as [int]

if ($vsVersion -eq 17) {
    $vsProjType = "Visual Studio 17 2022"
}
elseif ($vsVersion -eq 16) {
    $vsProjType = "Visual Studio 16 2019"
}
else {
    $vsProjType = "Visual Studio 17 2022"
}

cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE:STRING=$toolchainPath -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE "-H$topLevelTargetDir" "-B$buildFolder" -G $vsProjType -T host=x64 -A x64 -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
cmake --build $buildFolder --config Release --target ALL_BUILD


if ($?) {
    "Build succeeded. megaglest.exe, megaglest_editor.exe and megaglest_g3dviewer.exe can be found in mk/windoze/."
}
else {
    "Build failed. Please make sure you have installed VS C++ tools (2019 or 2022): https://visualstudio.microsoft.com/downloads ."
    "If you have installed all the relevant tools and you still can't build MegaGlest, try running '.\clean-build.ps1'. Then run this script again."
    "Make sure this script is running in developer powershell: https://docs.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell ."
    "If MegaGlest still fails to build, please help us by submitting a bug report at https://github.com/MegaGlest/megaglest-source/issues."
}
