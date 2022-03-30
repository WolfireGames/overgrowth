---
format: Markdown
...

# Mod Loading Design

*Note that this document is WIP and any thing written here might be changed*

## Rationale

Modding for Overgrowth has become a popular pasttime for many of our fans. This is largely enabled by Angel Script layer giving modders insight and freedom play around.
The current way of modding involved replacing existing content and script-files in the games data folder, usually making mods incompatible and demanding to keep updated as we modify the game and overwrite the modified files. It also creates issues for users who wish to restore their game to the original state. Therefore it's important to simplify modding and make it more modular and easier for the end user.

The mod loading system should fullfill the following features for mods

- Separated to individual folders or packages.
- Have the possibility to co-exist with other mods.
- Remove the necessity to overload or replace existing script files.
- Allow overloading of default files (only one active mod should be able to mask a single file to prevent odd behaviour)
- Enabled to intercommunicate.
- Enabled to have unique names, version numbers.
- Enabled to indicate depencies on other mods.
- Be possible to disable from within the game.
- Automatically detected.
- Be able to specify what version of the game they depend on.

## Overview

## Folder structure

Mods are usually installed in the game Data folder, in the Mods folder, it can look something like this

Overgrowth/Data/Mods/

In the mods folder there is one folder for each mod.

A mod contains at least a mod.xml file. But usually other data as well such as Scripts, Shaders, Models, Textures or Levels, All these folders are contained in the Data folder, resulting in the following structure

~~~
Overgrowth/Data/Mods/
   - my_mod/
       - mod.xml
       - Data/
          - Scripts/
          - Shaders/
          - Models/
          - Textures/
          - Levels/
~~~

## mod.xml - file

The mod.xml file is the primary meta-file for a mod. It's the file that the game searches for and parses to understand how to integrate the mod into the game. Following is a complete example.

    <? xml version="1.0" ?>
    <Mod>
        <!-- The ID is necessary and should never change between versions of the same mod -->
        <Id>unique-name</Id> 
        <!-- The human readable name is presented in UI's -->
        <Name>Human Readable Name</Name> 
        <!-- Version number, can have any formatting you wish. -->
        <Version>1.2.5</Version>

        <Thumbnail>unique_mod.png</Thumbnail>
        
        <!-- Specification for which version of the game that is supported. It's possible to specify more than one and use wildcards -->
        <SupportedVersion>a216</SupportedVersion>
        <SupportedVersion>a215-10*</SupportedVersion>

        <!-- It's possible to specify dependencies of other mods -->
        <ModDependency>
            <Id>other-mod</Id>
            <Version>1.0</Version> <!-- It's possible to specify multiple versions -->
            <Version>1.1</Version>
            <Version>2.*</Version> <!-- And even wildcards -->
        </ModDependency>
        <ModDependency>
            <Id>other-mod2</Id>
            <Version>1.0.0</Version> 
        </ModDependency>

        <!-- Files normally don't replace the standard files of the games, using these it's possible. -->
        <!-- If more than one mod specifies the same file, they are incompatible to run simultaneously -->
        <!-- If a file collides with a system file and isn't listed here it's going to generate an error -->
        <OverloadFile>GLSL/shader.vert</OverloadFile>
        <OverloadFile>Scripts/aschar.as</OverloadFile>

        <!-- Hooks are entrypoints for the engine into the mod -->
        <!-- They are necessary for mods that aren't used through hotspots, levels or similar but instead are always on-->
        <Hook>
            <!-- Available types are ModLoad, LevelLoad, Update, Draw, LevelClose and ModClose, EventReceive-->
            <Type>Update</Type>
            <!-- Path to script file containing the registered function -->
            <File>Scripts/aschar.as</File>
            <!-- Name of the function to be called -->
            <Function>Update</Function>
            <!-- Optional filter for EventReceive calls, these are inclusive, so it's possible to specify multiple. -->
            <Filter>set_player_position</Filter>
            <Filter>change_player_position</Filter>
        </Hook>
        <Hook>
            <Type>ModLoad</Type>
            <File>Scripts/mymodinit.as</File>
            <Function>Init</Function>
        </Hook>
    </Mod>

## Modding related API

### Mod meta data

It's possible to request the state of the loaded mods with the following function.

    array<ModInstance> GetLoadedModsList();

### Message API

One way for mods to globally communicated with eachothers is through the message API. It consists of the following functions.

    ModMessageSend( string message_name, string message );

It's possible to Hook a function to receive messages in a mod. An example is the following

    EventMessageReceive( string mod_name, string message_name, string message );
    
