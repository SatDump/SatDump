# Building on Windows
Building SatDump on Windows requires Microsoft Visual Studio 2019 or greater. These instructions are assuming you use Visual Studio 2022.

On install, set Visual Studio up for use with:
- Desktop development with C++
- Under "Individual components", install "Git for Windows" as well.

![image](https://github.com/JVital2013/goestools-win/assets/24253715/396cc01e-f35d-46ca-b2b4-e240170068de)

**These instructions and scripts assume you will be using Microsoft Visual Studio 2022**. If you are using another version of Visual Studio, update `scripts\Build.ps1` as necessary.

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
    .\scripts\Dist-Release.ps1       # Copies all necessary files into the Release folder
    ```

Your build SatDump will be in the `build\Release` folder of the SatDump repo.

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
4. Open Microsoft Visual Studio. On the launch screen, select "Open a local folder", and browse to your SatDump repo

    TODO: Pic here

5. MORE HERE