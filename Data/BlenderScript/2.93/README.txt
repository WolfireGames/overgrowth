###########################################
Author: Nadhif Ginola & Constance
Date: 01-07-2021 & 05-16-2023
Wolfire Games

READ ME

###########################################

-- HOW TO INSTALL --

There is a video under the "videos" folder showing how it is installed.
Otherwise, below are the steps.

1) Drop the "io_anm" folder from the .rar file to "C:\Users\YourName\AppData\Roaming\Blender Foundation\Blender\2.93\scripts\addons"
2) Open up Blender, go to Edit >> Preferences >> Addons
3) Click Install and select the "__init__.py" from inside the io_anm folder in "C:\Users\YourName\AppData\Roaming\Blender Foundation\Blender\2.93\scripts\addons\io_anm"
4) Click Install again and select bone_io.zip from the archive
5) Search "Phoenix" in the addon search bar and the add-ons should be visible
6) Enable them by checking the checkbox
7) To verify that it is enabled, check that File >> Export >> Phoenix Animation (.anm) and Phoenix Bones (.phxbn) exist.

Enjoy! 


###########################################


- Short video guide to install the addon in Blender 2.93 exists under the "videos" folder. It does not, however, cover Step 4.
- "rabbit_rig" and "rabbit_rig253" are the working rigs.

- If you want to port an existing rig that you made using the rabbit_rig, you will have to make some modifications. These modifications are shown in the "Porting_Old_Rig" video.
-- You have to re-parent "w_tar" bones to root through edit mode
-- Unlink r_rig_props.py

###########################################

- To export, the same procedure still applies
-- 1) Make sure the rig is selected! (Important!)
-- 2) Make sure an IK bone is not selected, otherwise the script may crash!
-- 3) Save through Files >> Export >> Phoenix Animation (.anm)
