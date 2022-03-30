---
format: Markdown
...

# Introduction 

As CMake ages and Apple leaps forward the disconnect between the two makes compiling CMake projects on modern OS X require a few hoops to jump through (which is complicated by the fact that -- at the moment -- Overgrowth is a 32-bit build, a legacy thing on Apple platforms).  I'm doing this from the point of view of Xcode 7, but the steps for 6.x are pretty much the same.

This is what I figured out by trial and error. Build systems are my Kryptonite.  If you find an easier way -- please let me know!  Always wanting to learn new tricks.

# Process

## 1. SDL Code Signing

There is some form of corrupted, improper or old code signing on the SDL framework as distributed.  This will cause your application to crash on startup without a lot of feedback.  You need to resign the framework.  Furthermore, Apple has tightened their signing requirements for the layout of files frameworks before signing.  So first you must delete all the .svn directories. 

So, from terminal -- in the <source base>*/Libraries/SDL2-2.0.3/* directory
	
	find . -name .svn -exec rm -rf {} \;
	codesign -f -s - SDL2.framework

And the framework should be ready to work without crashing.

## 2. CMake 

If cmake doesn't find a compiler, run cc in the terminal or start xcode a first time to agree to the terms of use.

I'm assuming you're using the default GUI client and you know how to use it. You should add setting of

	-DCMAKE_OSX_SYSROOT=macosx

to avoid having to manually change the Base SDK in Xcode.

So Configure to use Xcode and then generate into your choice of build directory.  

![CMake GUI](/img/OSXCMake/1-OSXCMake-CMake.png)

Now the fun begins.

## 3. Build Settings

1. Launch Xcode and load the project created in the root of your build directory.

2. We need to change the target to Overgrowth as it'll build a bunch of superfluous projects instead.

![Change Target](/img/OSXCMake/2-OSXCMake-Target.png)

3. Note that by default it'll compile Debug, so you may want to change this to Release.  To bring up scheme quickly hit **command-<**.

![alt text](/img/OSXCMake/3-OSXCMake-Release.png)

4. If we built at this point we'd have a whole bunch of strange errors.  Since the project CMake generates may as well have been written on cuneiform tablets from the point of view of Xcode, we have to update the project.  First select the Overgrowth project from the lefthand column (or the menu item won't appear) and select **Editor|Validate Settings**.  It'll present a long list of proposed changes -- accept them.

![Validate Settings](/img/OSXCMake/5-OSXCMake-Validate.png)

5. Now we have to undo some of what the previous step did.  Select the Overgrowth project from the lefthand column (**Project Navigator**).  Again, select the the **Overgrowth** project from the lefthand column of the center pane and then change the view to **Combined**.  Change the **Architectures** to **32-bit Intel (i386)** and for good measure remove **x86**64** from the **Valid Architectures.  In order to remove another dumb bit of CMake screwup change the **Base SDK** to **Latest OS X**. If you added the Cmake command line option above this should not be necessary.
Don't worry about this restricting your build to the latest OS, Apple has changed the way the platform SDKs work.  All the supported SDKs have been amalgamated.  The actual OS X version is determined below by changing **OS X Deployment Target** in the **Deployment** section (this will default to 10.6).  (Note that this is new in Xcode 7 -- things are a bit simpler in Xcode 6).

If you hit Build at this point -- you should get a successful compile.  In order to make this a viable project we need to undo a last little bit of CMake insanity and change a few settings so you can run and debug.

## 4. Execution Parameters

1. First set we need to set a custom Working Directory. This is set in the scheme (again **command-<**).  Under **Options** find **Working Directory** and select the root of Overgrowth Data folder (the one currently in Dropbox).

![Change Working Directory](/img/OSXCMake/7-OSXCMake-Working Directory.png)

2. Now for the biggest hack.  Still in the scheme go back to **Info** and look at executable.  This **looks** like it's the Overgrowth executable, but this is the one that's built original, not the one that CMake's custom scripts move into the application bundle.  So we need to point this at the executable in the bundle.  The annoying part is that we can't **Show Package Contents** in the selection dialog box -- so we need to work around this somehow.  Go into finder, go to the Release directory in the root of your build directory.  Assuming you've built the application you should see two executables and the actual app bundle. Right click and **Show Package Contents** and go into Contents and then (temporarily) drag the MacOS folder to Finder sidebar. 

![Finder Hack](/img/OSXCMake/8-OSXCMake-Finder.png)

3.  Back in Xcode with scheme and **Info** and under **Executable** scroll down till you find **Other...**.  A selection window will come up and you show the sidebar (if you need to) to find the MacOS folder from the last step.  Selecting this will show inside the bundle and **now** you can select the overgrowth executable. 

![Change Executable](/img/OSXCMake/9-OSXCMake-Executable.png)

4. Finally, if you haven't done so go to **/Users/*<username>*/Library/Application Support/Overgrowth** and fill in the **extra_data_path** in config.txt to be the root of your *source* directory (i.e. the one from git). On my system this is:

>	extra_data_path: /Users/mjb/Documents/Development/Overgrowth/

Congratulations.  You can now compile, run and debug with some of the best tools available.  Now go make a great game.
