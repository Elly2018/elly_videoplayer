# gd-videoplayer

A GDExtension for godot 4.x to play video, this is a wrapper of ffmpeg for decode

 
### Getting started:

##### Requirement

- [git](https://git-scm.com) tools
- cURL tool
	- Which for windows you can use powershell (admin) to download: `choco install curl`.

#### 1. Open build/windows_pull_submodule.bat
- This will clone the godot-cpp 4.1 in the root folder.
- And it will download the ffmpeg 6.0 windows build from github and unzip in the src folder.
- In the end your file structure will looks like below.

- root
	- build
	- src
		- src
		- ffmpeg (Add)
	- godot-cpp (Add)
	
#### 2. Open build/windows_codegen.bat.

- This will generate the solution in the `GDExtensionTemplate-build` folder.

#### 3. Open GDExtensionTemplate-build/GDExtensionTemplate.sln.

- You cannot just build the solution for some reason, I can't find a way to change Runtime library from /MDd to /MD.
- So first, open property of GDExtensionTemplate project.
- Change C/C++ -> Code Generation -> Runtime Library from /MDd to /MD.
	- If you don't change this options you will getting bunch of link error.
- After build is finish and success, the output will be in the `GDExtensionTemplate-build\GDExtensionTemplate` folder.

### Repository structure:
- `build/` - All the build scripts in here.
- `src/` - Source code of this extension.
	- `ffmpeg/` - FFmpeg library.
	- `src/` - Wrapper source code.
- `godot-cpp/` - Submodule needed for GDExtension compilation.

### Platfrom CheckList
| Platform | Video | Audio | XR Support |
|-|-|-|-|
| Windows | O | X | X |
| Windows | X | X | X |
| MacOS | X | X | X |
| MacOS | X | X | X |
| Linux | X | X | X |
| Linux | X | X | X |
| Android | X | X | X |
| Android | X | X | X |
| IOS | X | X | X |
| IOS | X | X | X |
| Web | X | X | X |
| Web | X | X | X |