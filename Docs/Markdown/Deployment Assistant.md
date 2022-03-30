# Summary

The Overgrowth Deployment Assistant is meant to be used to deploy assets for
alpha releases to make sure that all assets that need to be included are
included and that they are processed. This will make the game download smaller,
decrease load times and give us better control over exactly what is and isn't
included in a release.

This tool is also intended to be available to modders so they can package their
mods into a minimal size (and potentially into single binary mod files packages
for simplified handling)

Therefore the functionality is planned to be compiled into a dynamic library
and have a simple CLI wrapper program, allowing it to be baked into the main
Overgrowth binary on demand. Separating it from the main Overgrowth removes any
dependency on OpenGL or OpenAL, making it capable of running on a headless
server.

# Features

- Supply an asset (object, texture, character, terrain, etc.), process all
  assets required for that asset and put them in the specified folder.
- Supply a manifest containing a list of levels and assets that then get
  processed one after another as if they were individual input commands,
construct a complete file list and then copy it to a final destination.

# Example usage

`ogda`


# Flags

### Required flags

`-w --write-dir`: Where to output processed files to.

`-s --source-dir`: Folder to read from

### Optional flags

`-b --blacklist`: Can be used in conjunction with --recursive to exclude specific files.

`-c --clean`: Clears the destination folder before doing anything else.

`--dry-run`: Only outputs a report of missing files, non-processed files and processed
files. Does not perform any processing.

# Job file

This is an XML file with a list of commands for the asset packer to handle. It
might look something like this:

~~~
<?xml version="2.0" ?>
<!-- Root node -->
<Job name="main game data">
    <Filters>
        <!-- Apply the dxt5 converter to all texture types that ends in .tga --> 
        <Filter type="texture" source_pattern="*.tga" converter="dxt5"></Filter>
        <!-- Generate a navmesh for all files which are of type level -->
        <Filter type="level" converter="navmesh"></Filter>
    </Filters>
 
    <!-- The add_deps flag tells the level item reader to add new items from the levels structure, like textures, decals, etc -->
    <Items>
        <Item type="level" add_deps="true">Data/Levels/max_dev_1.xml</Item>
        <Item type="character">Data/Characters/Turner.xml</Item>
        <Item type="decal">Data/Objects/Decals/black_dust.xml</Item>
        <!-- We don't _like_ recursive rules, so they only work with copy. It's useful for folders who essentially are going to be retained in their full source structure. -->
        <Item type="copy" recursive="true">Data/UI/</Item>
    </Items>
</Job>
~~~

# Design

The system is relatively simple in its overall construction.

It's a stateful machine that loads a Job File and preloads its content.

It then does a "dry run" in any item that might be an xml-file, adding any
dependencies to the item list, continuously extending.

For each item that is listed but doesn't exist on disk, there is a warning.
