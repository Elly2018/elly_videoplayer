# Build Helper

Before you start use the scripts, make sure you have below tools installed.

- Windows
	- Requirement:
		- [CMake](https://cmake.org/download/) tool.
		- [Git](https://git-scm.com) tool.
		- cURL tool
			- Which for windows you can use powershell (admin) to download: `choco install curl`.
- MacOS
- Linux
	- Requirement:
		- CMake - `sudo apt-get -y install cmake`
		- Git - `sudo apt-get -y install git`

## For Godot Dev

### Getting started:

Windows, Linux platform build system use Visual Studio 17 2022

If you are using MSYS to build the linux version you can use build/msys_install.sh for install require package

In order to setup the project\
Check this page for build process

#### 1. Open build/Godot/[Platform]_pull_submodule.[bat/sh]

- This will clone the godot-cpp 4.2 in the root folder.
- And it will download the ffmpeg 6.0 windows build from github and unzip in the src folder.
- In the end your file structure will looks like below.

- root
	- build
	- src
		- src
		- ffmpeg (Add if desktop)
	- godot-cpp (Add)
	- mobile
    	- libs
        	- ffmpeg-kit.aar (Add if android)
	
#### 2. Open build/Godot/[Platform]_codegen.[bat/sh]

- This will generate the solution in the `{root}/GDExtensionTemplate-build` folder.

#### 3. Build the solution in `{root}/GDExtensionTemplate-build/GDExtensionTemplate.sln`

- Some version of visual studio won't react to the cmake treat warning as error is off setting, In that case the build will failed. you can turn it off in the `Project Setting -> C/C++ -> General -> Treat Warnings As Errors`

## For Unity Dev

#### 1. Open build/Unity/[Platform]_pull_submodule.[bat/sh]

- This will clone the unity native plugin in the src folder.
- And it will download the ffmpeg 6.0 windows build from github and unzip in the src folder.
- In the end your file structure will looks like below.

- root
	- build
	- src
		- src
		- unity-native (Add)
		- ffmpeg (Add if desktop)
	- mobile
    	- libs
        	- ffmpeg-kit.aar (Add if android)

#### 2. Open build/Unity/[Platform]_codegen.[bat/sh]

- This will generate the solution in the `{root}/UnityExtensionTemplate-build` folder.

#### 3. Build the solution in `{root}/UnityExtensionTemplate-build/UnityExtensionTemplate.sln`

- Some version of visual studio won't react to the cmake treat warning as error is off setting, In that case the build will failed. you can turn it off in the `Project Setting -> C/C++ -> General -> Treat Warnings As Errors`
- Another failed reason is lost the opengl libs, Which in this case you will getting spam a bunch of Link error, go to `PlatformBase.h` and change line 51 from `#define SUPPORT_OPENGL_UNIFIED 1` to `#define SUPPORT_OPENGL_UNIFIED 0`

## For Unreal Dev


## For Native Dev



## All Script Description

### [Platform]_pull.submodule

Download the require packages on the internet

### [Platform]_codegen

This will 

## Utility Script

### [Platform]_build_dependencise

- Require environment variable
	- projectpath - your project path

### [Platform]_build_post

- Require environment variable
	- projectpath - your project path
