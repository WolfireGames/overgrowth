# How To Compile Overgrowth

To compile and run Overgrowth, you first need to have installed a commercial version of the game. 
If you're on steam, you'll also want to make sure you're using the internal_testing branch to have the latest set of data-files for the game.
The reason for this is that you want the Data/ folder from the game.

These instructions assume you've already managed to download the project and have it in a local folder. There's no need to try and initialize the submodules unless you are an official developer of Wolfire, as those repos are private and primarily used by the build system for deployment. Make sure that the folder depth isn't too great, as this can cause some issues. From now we'll assume you've downloaded the repo into the following folder on windows ```C:\overgrowth``` and ```~/overgrowth``` on unix-like systems.

# Dependencies

## Windows

To compile Overgrowth on windows you'll need the following to be installed before you start:
* CMake https://github.com/Kitware/CMake/releases/download/v3.23.0/cmake-3.23.0-windows-x86_64.msi
* Visual Studio 2022 Community Edition https://visualstudio.microsoft.com/downloads/

## MacOSX

On MacOSX, you'll need to install XCode and CMake before you can compile the project.

After that you may need to start XCode once before building the project, to ensure that everything is configured. You may also have to install the command line XCode tools.

## Generic Linux

You'll need a GCC compiler, OpenGL, CMake, SDL2, SDL2_net, GTK 2, Ogg, Vorbis, FreeImage, OpenAL, libjpeg, Theora, bzip2 and FreeType.

## Ubuntu

The following command should install all necessary dependencies to build Overgrowth.
```sudo apt install build-essential cmake mesa-common-dev libsdl2-dev libsdl2-net-dev libgtk2.0-dev libogg-dev libvorbis-dev libfreeimage-dev libopenal-dev libjpeg-dev libtheora-dev libbz2-dev libfreetype-dev```

# How To Compile

## Windows

1. Make sure you have an installed copy of Overgrowth and find the path to the install. If you've installed the game via Steam you can get the path by right clicking the game in the Steam games list, press Properties, go to Local Files and press Browse. This will open explorer window with the game path, copy that path into your clipboard. example: ```E:\SteamLibrary\steamapps\common\Overgrowth```

2. Go to the Overgrowth git repo folder and create a new subfolder named "Build" example ```C:\overgrowth\Build```

3. Open a windows cmd (alternatively a git bash console) in the build folder you just created.

4. Run the following command ```cmake ../Projects -DAUX_DATA="E:/SteamLibrary/steamapps/common/Overgrowth"```. Replace the path with your own Overgrowth install, and switch the path separators from ```\``` to ```/```

5. Open the generated ```Overgrowth.sln``` file using Visual Studio 2022

6. Press the "Local Windows Debugger" button to compile and start the game.

## MacOSX

1. Go to the folder of the project
2. Create a build directory ```% mkdir Build```
3. Run cmake to generate Makefiles, include the path to an already installed instance of Overgrowth ```% cmake ../Projects -DAUX_DATA="/path/to/installed/Overgrowth.app/Contents/MacOS"```
4. Run make to build the game ```% make```
5. Run the game by first moving to the project directory ```% cd ..```
6. Then start the game ```% ./Build/Overgrowth.app/Contents/MacOS/Overgrowth```

## Linux

1. Go to the folder of the project
2. Create a build directory ```$ mkdir Build```
3. Run cmake to generate Makefiles, include the path to an already installed instance of Overgrowth ```$ cmake ../Projects -DAUX_DATA="/path/to/installed/Overgrowth"```
4. Run make to build the game ```$ make```
5. Run the game by first moving to the project directory ```$ cd ..```
6. Then start the game ```$ Build/Release/Overgrowth.x86_64```
