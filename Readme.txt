acqu-ETSDK-Codebase - a Wolfenstein™: Enemy Territory™ Source Code Modification

This is the source code for the acqu-SDK. This source code is based on ETSDK version 2.6 for the game Wolfenstein: Enemy Territory. The primary goal of this source code modification is to enhance the already existing source code basis and to make it accessible as an alternative codebase for further use. This source code is under a "LIMITED USE SOFTWARE LICENSE AGREEMENT", which was shipped with the original version of ETSDK. 

This source code is not officially supported in any capacity by id Software, Activision or Splash Damage. Below you will find very basic instructions on how to get the source code working so that you can create your own Enemy Territory mods. For online help and resources, please visit www.castlewolfenstein.com or www.splashdamage.com or https://github.com/acqu/acqu-etsdk-codebase.

Windows:
========

The first thing you have to do is download a working copy of the repository found at https://github.com/acqu/acqu-etsdk-codebase and have Visual Studio Express 2008 installed (other versions of VS are unsupported).

1. Open the src folder.
2. Double-click on the wolf.sln file to open the project workspace.
3. In the build menu select 'Debug' or 'Release'.
4. At this point, you should be able to compile the code.

Linux:
======

The first thing you have to do is download a working copy of the repository found at https://github.com/acqu/acqu-etsdk-codebase and have cmake [1] version 2.8.1 or higher installed. You must also have gcc and g++ version 3.4 or greater installed.

Cmake is used to prepare the build environment. Two options are recommended:

A) Makefile-based build environment:

1. Type the following commands at a command prompt:
   cd /path/to/working-copy/src
   mkdir cmake; cd cmake
   cmake .. -G "Unix Makefiles"
   make
2. After a few minutes of compiling, you should have cgame, qagame and ui .so files in your directory.

B) IDE-based build environment (in this example CodeBlocks is used):

1. Type the following commands at a command prompt:
   cd /path/to/working-copy/src
   mkdir cmake; cd cmake
   cmake .. -G "CodeBlocks - Unix Makefiles"
2. This should place a ready-to-use IDE build environment in your cmake directory. Compile from there.
3. After a few minutes of compiling, you should have cgame, qagame and ui .so files in your directory.

Notes:

- Release builds go into the build directory.
- Also type cmake --help to get a full list of what IDE build environments are supported by cmake.

Mac:
====

(Currently unsupported)

The build directory:
====================

By default, this directory will contain release builds. In addition, helpful install scripts and additional directories and files which are needed for the mod are located there. Take a look at the .bat or .sh scripts respectively if you want to use the install scripts.

Usually everything which is located in this directory (except the install scripts) need to be packed into the mod.

Configuring the source:
=======================

This source code modification offers features possibly undesirable. In case you do not want to compile your binaries with these features enabled you can either remove them by hand or simply exclude them by conditional compilation.

In the former case you can search either the version control history associated with this feature and then remove the code line by line or you can skip that and directly search the code for references. In the latter case you can search the code for a reference template which spells "acqu-sdk (issue XX):", where XX is the issue number. Also the list of issues might be helpful, since each commit is usually associated with an issue. It can be found at https://github.com/acqu/acqu-etsdk-codebase/issues.

In case you want to use conditional compilation you can exclude certain features from the mod. By default all build settings compile these features in. In order to remove them you have to reset the corresponding precompiler, linker flags and include directory settings. This is different from platform to platform. On Windows you have to modify the project settings. On Linux when building with cmake you have to modify CMakeLists.txt. An example of how to do this with Lua is given in the wiki (TODO): https://github.com/acqu/acqu-etsdk-codebase/wiki. A listing of which additional files and directories are used for what feature and how to remove them from your build environment is given below.

A) Precompiler flags (- simply not define these in your project settings in case you decided against one of these features):

1) CAMTRACE_SUPPORT
2) OMNIBOT_SUPPORT
3) LUA_SUPPORT
4) XPSAVE_SUPPORT

B) Additional files and directories associated with each feature:

1) Camtrace:
    <none>
2) Omnibot:
    include:
        src/omnibot/Common
        src/omnibot/ET
    lib:
        required at runtime. Visit http://www.omni-bot.com/ for download.
    src:
        src/game/g_etbot_interface.cpp
        src/game/BotLoadLibrary.cpp
3) Lua:
    include: src/lua/lua5.1/include
    lib: src/lua/lua5.1/lib/static
    src: src/game/g_lua.c
4) XPSave:
    <TODO>

