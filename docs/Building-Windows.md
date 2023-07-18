# Building on Windows
**These instructions and scripts assume you will be using Microsoft Visual Studio 2022**. Building SatDump on Windows requires Microsoft Visual Studio 2019 or greater. If you are using another version of Visual Studio, update `scripts\Build.ps1` as necessary.

Compilation is currently only available for 64-bit Windows.

**On install, set Visual Studio up for use with:**
- Desktop development with C++
- Under "Individual components", install "Git for Windows" as well.

![image](https://github.com/JVital2013/goestools-win/assets/24253715/396cc01e-f35d-46ca-b2b4-e240170068de)

## Automated PowerShell Build Instructions
Using the automated PowerShell build scripts is the easiest way to build SatDump on Windows.

1. Open "Developer PowerShell for VS 2022" from the start Menu

    ![image](https://github.com/JVital2013/goestools-win/assets/24253715/ef7af001-c45e-4ee7-88e6-d9bb33d6a5fe)

2. cd to a directory where you want to build SatDump
3. Run the following commands, one at a time
    ```powershell
    git clone https://github.com/SatDump/SatDump
    cd SatDump
    .\scripts\Configure-vcpkg.ps1    # Downloads a preconfigured vcpkg to the correct
                                     # location and adds SDRPlay libraries.
    .\scripts\Build-SatDump.ps1      # Builds SatDump
    .\scripts\Finish-Release.ps1     # Copies all necessary files into the Release folder
    ```

Your build SatDump will be in the `build\Release` folder of the SatDump repo.

![Build Folder](https://github.com/SatDump/SatDump/assets/24253715/1731db36-7237-4ed5-8e71-7a36cc79305b)

## Building in Visual Studio for Debugging
Compiling in Visual Studio for Debugging is for advanced users who aren't afriad to get their hands dirty!

1. Open "Developer PowerShell for VS 2022" from the start Menu

    ![image](https://github.com/JVital2013/goestools-win/assets/24253715/ef7af001-c45e-4ee7-88e6-d9bb33d6a5fe)

2. cd to a directory where you want to build SatDump
3. Run the following commands, one at a time
    ```powershell
	git clone https://github.com/SatDump/SatDump
	cd SatDump
    .\scripts\Configure-vcpkg.ps1    # Downloads a preconfigured vcpkg to the correct
                                     # location and adds SDRPlay libraries.
	```
4. Keeping the developer console open in the background, open Microsoft Visual Studio. On the launch screen, select "Open a local folder", and select your cloned SatDump repo.

    ![Open Local Folder](https://github.com/SatDump/SatDump/assets/24253715/22f2258d-0cc4-4b6e-9c5c-5e0dd4fb8b53)

5. Once the project opens, select "Open CMake Settings Editor"

    ![Edit CMake Settings](https://github.com/SatDump/SatDump/assets/24253715/026d6ea5-9b6d-4560-899b-175bd8f13eef)

6. In CMake Settings, change the following items:
    - **CMake toolchain file:** set this to `<full path of your satdump repo>\vcpkg\scripts\buildsystems\vcpkg.cmake`
    - **CMake valiables and cache:** check "BUILD_MSVC"
    - **CMake Generator (under "show advanced setting):** set this to "Visual Studio 17 2022 Win64"

7. Press Ctrl+S to save the CMake Settings. You will usually get an error right away - this is normal. Either way, click Project > Delete Cache and Reconfigure. Confirm the prompt and let cmake rebuild its cache

    ![Delete Cache and Reconfigure](https://github.com/SatDump/SatDump/assets/24253715/9205663f-011f-4c52-a31d-f60795fa3822)

8. Once cmake finishes, click Build > Build All. Go get come coffee because this takes several minutes

    ![Build All](https://github.com/SatDump/SatDump/assets/24253715/3f0a38a1-3e98-41b7-bddb-5aa6c376aec4)

9. Once you get the "Build all succeded" message, go back to your developer terminal (it's still open, right?) and run `.\scripts\Finish-Debug.ps1`. This will copy all necessary DLL files into the debug folder.
10. That's it, you're ready to debug! To make any changes to the code and build again, just repeat steps 8 and 9 before launching the debugger. **Step 9 is important, because it copies the plugin DLLs into the right spot for SatDump to load them!**

## Debugging SatDump UI
Once you built SatDump for debugging in Windows, using the debugger is the same as it is with any other Visual Studio project.

1. At the top, set the startup item to "satdump-ui.exe" (not the installed one!)

    ![SatDump UI Startup Item](https://github.com/SatDump/SatDump/assets/24253715/e811a074-3f3c-4e2b-8bf9-7600a17b577b)

2. Click the green start icon to start debugging SatDump-UI. Breakpoints and other tested debugger functionality works as expected

    ![The Mind of Alan](https://github.com/SatDump/SatDump/assets/24253715/77ae1ddd-6507-4477-968f-b488e76f366b)

## Debugging SatDump CLI
Debugging the CLI version is similar to the UI, but you need to provide command line arguments

1. At the top, change the startup item to "satdump.exe"

    ![Satdump target](https://github.com/SatDump/SatDump/assets/24253715/92ff0872-6aec-476f-889c-de2468d66b82)

2. Go to Debug > Debug and Launch setting for SatDump

    ![Debug Settings](https://github.com/SatDump/SatDump/assets/24253715/c173425c-4cb5-4cbd-8020-ee1342dfffaa)

3. Add the satdump launch arguments in the form of an array. If you compose the argument list elsewhere and copy/paste it here, it will turn it into an array for you. You may need to clean it up, though

    ![Args](https://github.com/SatDump/SatDump/assets/24253715/0ae0ee59-bc0b-48da-80d5-de2dd3e0757a)

4. Click the green start icon to start debugging SatDump.

    ![Dumping all MSUMR Timestamps](https://github.com/SatDump/SatDump/assets/24253715/11c1ff2c-82db-47eb-9433-51624ff7727b)
