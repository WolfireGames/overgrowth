###########################################
Author: Nadhif Ginola
Date: 01-07-2021
Wolfire Games

READ ME

###########################################

-- HOW TO INSTALL --

There is a video under the "videos" folder showing how it is installed.
Otherwise, below are the steps.

1) Drop the "io_anm" folder from the .rar file to "C:\Users\YourName\AppData\Roaming\Blender Foundation\Blender\2.93\scripts\addons"
2) Open up Blender, go to Edit >> Preferences >> Addons
3) Click Install and select the "__init__.py" from inside the io_anm folder in "C:\Users\YourName\AppData\Roaming\Blender Foundation\Blender\2.93\scripts\addons\io_anm"
4) Search "Phoenix" in the addon search bar and the add-on should be visible
5) Enable it by checking the checkbox
6) To verify that it is enabled, check that File >> Export >> Phoenix Animation (.anm) exists.

Enjoy! 


###########################################

- The Exporter is the only functional part of this update and it is on purpose.

- Short video guide to install the addon in Blender 2.93 exists under the "videos" folder.
- "rabbit_rig" and "rabbit_rig253" are the working rigs.

- If you want to port an existing rig that you made using the rabbit_rig, you will have to make some modifications. These modifications are shown in the "Porting_Old_Rig" video.
-- You have to re-parent "w_tar" bones to root through edit mode
-- Unlink r_rig_props.py

###########################################

- To export, the same procedure still applies
-- 1) Make sure the rig is selected! (Important!)
-- 2) Save through Files >> Export >> Phoenix Animation (.anm)




