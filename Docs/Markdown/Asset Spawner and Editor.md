# Asset Spawner and Editor

## Introduction
I have been thinking about how to make it easier to make content for Overgrowth. My conclusion so far is that working with XML files is something that is done a lot and kind of cumbersome: Navigating the folders, copying files, typing in paths, remembering what different options you have and so on. It takes time, is error prone and is not a lot of fun.

So it would be really cool I think if all of that could be done from within the game. You can make a new object in there and then it just gives you the fields to fill in, you can browse to find textures and there can be drop down lists, checkboxes and so on where appropriate. This makes it faster, easier and more fun to make new objects. It could also serve as an improved spawner.

Since objects are taken straight from their folders instead of being hard coded in, we can use a folder tree as a quick and easy way to filter objects. If you click one of the folders in the tree, all objects in that folder and all its sub-folders are displayed on the right side.

![Mockup of browser](/item-spawner-editor-mockup.png)

## Structure
There is a tab based interface like in the current spawner. Each tab is created by creating an XML file in a specific directory. This XML file specifies the following.

- What folder to look for XML files in to list in this tab
- Are the files listed in this tab spawnable?
- What type of data does the elements of interest in the XML files in this tab contain? (bool, drop down, file path, etc.)

Example of an XML file for an Object tab:

~~~ { .xml }
<?xml version="1.0" ?>
<tab>
    <description>
        Static objects
    </description>
    <name>Object</name>
    <path>Data/Objects</path>
    <spawnable>true</spawnable>
    <elements>
        <Model>
            <description>
                The 3D representation of the object.
            </description>
            <type>path</type>
            <root>Data/Objects</root>
            <extension>obj</extension>
        </Model>
        <ColorMap>
            <description>
                Color texture to use for object.
            </description>
            <type>image</type>
            <root>Data/Textures</root>
            <extension>tga,png,tif</extension>
        </ColorMap>
        <NormalMap>
            <description>
                Normal texture to use for object.
            </description>
            <type>image</type>
            <root>Data/Textures</root>
            <extension>tga,png,tif</extension>
        </NormalMap>
        <TranslucencyMap>
            <description>
                Adds its color to the surface multiplied by the amount of light it receives, this effect ignores the normal of the surface, so if one side receives light from for instance the sun, the other side will also get the color added. If unspecified it defaults to 0.0 (no translucency).
            </description>
            <note>
                Only used in plant shader.
            </note>
            <type>image</type>
            <root>Data/Textures</root>
            <extension>tga,png,tif</extension>
        </TranslucencyMap>
        <WindMap>
            <description>
                The intensity of the vertex wind effect. If unspecified it defaults to 1.0 (active).
            </description>
            <note>
                Only used in plant shader.
            </note>
            <type>image</type>
            <root>Data/Textures</root>
            <extension>tga,png,tif</extension>
        </WindMap>
        <ShaderName>
            <description>
                Name of shader to use.
            </description>
            <type>dropdown</type>
            <item>cubemap</item>
            <item>cubemapobj</item>
            <item>cubemapitem</item>
            <item>cubemapobjitem</item>
            <item>cubemapalpha</item>
            <item>cubemapobjchar</item>
            <item>plant</item>
        </ShaderName>
        <MaterialPath>
            <description>
                Changes what particles and sound effects are emitted from the surface when stepped on etc.
            </description>
            <type>dropdown</type>
            <path>Data/Materials</path>
        </MaterialPath>
        <flags>
            <attribute>
                <description>
                    Disables physics collisions with this object.
                </description>
                <name>no_collision</name>
                <type>bool</type>
            </attribute>
            <attribute>
                <description>
                    The backsides of the object gets rendered as well.
                </description>
                <name>double_sided</name>
                <type>bool</type>
            </attribute>
            <attribute>
                <description>
                    Makes the object give some resistance when passed through while the object wobbles a bit and generates leaf particles. If a character jumps into this object at high enough speed they will ragdoll.
                </description>
                <name>bush_collision</name>
                <type>bool</type>
            </attribute>
        </flags>
    </elements>
</tab>
~~~

## Modes
### Spawner mode
Since it is specified if the files listed by a tab are spawnable or not there could be another mode in this editor where all editing capabilities are turned off. In this mode you can use it to spawn objects instead like in the current spawner, so it would replace the current spawner. in this mode all tabs that are not specified to contain spawnable things are hidden.

This eliminates the need to manually fill the spawn menu with items, which is another thing that is time consuming, boring and error prone to do, it is also easy to miss adding some items. I would suggest that each item specifies what thumbnail it should use in its own XML file. If there is no thumbnail it falls back to a default.

### Editor mode
In this mode you can see all tabs, including those that contain XML files that cannot be spawned, such as levels and particle XMLs. If you click an item in this mode a new window opens up for that item where you can edit the elements that have been specified in the settings for this tab. A button can be pressed to open up elements that have not been specified in that XML for editing, just in case you want to edit something that is very uncommon to edit.

![mockup of xml editor](/xml-editor.png)

## Mod support
It should be built with mods in mind, meaning it also looks for items in the same folders in mods as well. There should be a button in the browser that brings up a menu where you can choose what mods you want to display content from (including the main game).
