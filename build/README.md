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
	
#### 2. Open build/Godot/[Platform]_codegen.[bat/sh]

- This will generate the solution in the `{root}/GDExtensionTemplate-build` folder.

## For Unity Dev


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
