# gd-videoplayer

A GDExtension for godot 4.x to play video, this is a wrapper of ffmpeg for decode

 
### Getting started:

##### Requirement

- [git](https://git-scm.com) tools
- cURL tool
	- Which for windows you can use powershell (admin) to download: `choco install curl`

#### 1. Open build/windows_pull_submodule.bat
- This will clone the godot-cpp 4.1 in the root folder
- And it will download the ffmpeg 6.0 windows build from github and unzip in the src folder
- In the end your file structure will looks like below

- root
	- build
	- src
		- src
		- ffmpeg (Add)
	- godot-cpp (Add)
	
#### 2. Open build/windows_codegen.bat.

- This will generate the solution in the `GDExtensionTemplate-build` folder

#### 3. Open GDExtensionTemplate-build/GDExtensionTemplate.sln.

- You cannot just build the solution for some reason, I can't find a way to change Runtime library from /MDd to /MD
- So first, open property of GDExtensionTemplate project
- Change C/C++ -> Code Generation -> Runtime Library from /MDd to /MD
	- If you don't change this options you will getting bunch of link error
- After build is finish and success, the output will be in the `GDExtensionTemplate-build\GDExtensionTemplate` folder

### Repository structure:
- `project/` - Godot project boilerplate.
  - `addons/example/` - Files to be distributed to other projects.¹
  - `demo/` - Scenes and scripts for internal testing. Not strictly necessary.
- `src/` - Source code of this extension.
- `godot-cpp/` - Submodule needed for GDExtension compilation.

¹ Before distributing as an addon, all binaries for all platforms must be built and copied to the `bin/` directory. This is done automatically by GitHub Actions.

### Make it your own:
1. Rename `project/addons/example/` and `project/addons/example/example.gdextension`.
2. Replace `LICENSE`, `README.md`, and your code in `src/`.
3. Not required, but consider leaving a note about this template if you found it helpful!

### Distributing your extension on the Godot Asset Library with GitHub Actions:
1. Go to Repository→Actions and download the latest artifact.
2. Test the artifact by extracting the addon into a project.
3. Create a new release on GitHub, uploading the artifact as an asset.
4. On the asset, Right Click→Copy Link to get a direct file URL. Don't use the artifacts directly from GitHub Actions, as they expire.
5. When submitting/updating on the Godot Asset Library, Change "Repository host" to `Custom` and "Download URL" to the one you copied.
