Compiling for windows
=====================

1. In powershell, set the [execution policy](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.security/set-executionpolicy?view=powershell-7.2) so that you are allowed to run the build script:

```ps1
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy Unrestricted
```

2. Download and install the latest version of cmake: https://cmake.org/download/
3. Download and install visual studio build tools: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022* . The workload / component that needs to be installed is "Desktop Development with C++". If you're using a non-english edition of windows, from the install you will need to switch to the "language packs" tab and install "english language pack"**.
4. Open "Developer Powershell"***.
5. From developer powershell, change to the build script dir (replacing `C:\path\to\` with the correct path to your clone of `megaglest-source`):

```ps1
cd C:\path\to\megaglest-source\mk\windoze
```

6. Build the game: 

```ps1
.\build-mg-vs-cmake.ps1
```

After the script has finished, the game should have built and all the exe's should be present inside `megaglest-source\mk\windoze`.


------------------------------------------
*If you want to try build tools which aren't vs2022 or vs2019, you may want to edit L110-118 in `build-mg-vs-cmake.ps1`. If you add support for another version of vs, please consider submitting a pull request with corresponding changes to megaglest-source git.

**https://github.com/microsoft/vcpkg/issues/24600

***https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2022


Extra notes
-----------

* Building for the first time can be very slow, because installing (and usually building) the packages from vcpkg is slow. Subsequent builds should be much faster, though.

* Required dependencies for building are obtained from [vcpkg](https://vcpkg.io). By default, the vcpkg folder is cloned into `mk\windoze\vcpkg` and dependencies are installed there. However, if you've already got vcpkg installed in a different location, you can set that in the build script:

```ps1
.\build-mg-vs-cmake.ps1 -vcpkg-location C:\path\to\vcpkg\
```

If you haven't installed vcpkg yet, you can still use this parameter to set a custom vcpkg location and the build script will clone vcpkg into there.

* The instructions here suggest installing the "Visual Studio build tools". However, an installation of the "Visual studio IDE" with "Desktop Development with C++" is perfectly fine (but not necessary).
