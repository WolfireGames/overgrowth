#Level Format

This document tries to document the level xml format, as it's become quite the doozy

Following is an (possibly incomplete) example of a level.

~~~{ .xml }
<?xml version="2.0" ?>
<Level>
    <Type>saved</Type>
    <Name></Name>
    <Description></Description>
    <Shader>post</Shader>
    <Terrain>
        <Heightmap>Data/Textures/Terrain/red_desert/red_desert_hm.png</Heightmap>
        <DetailMap></DetailMap>
        <ColorMap>Data/Textures/Terrain/red_desert/red_desert_c.tga</ColorMap>
        <WeightMap>Data/Textures/Terrain/red_desert/red_desert_w.png</WeightMap>
        <DetailMaps>
            <DetailMap colorpath="Data/Textures/Terrain/DetailTextures/pebbles.tga" normalpath="Data/Textures/Terrain/DetailTextures/pebbles_normal.tga" materialpath="Data/Materials/rocks.xml" />
            <DetailMap colorpath="Data/Textures/Terrain/DetailTextures/pebbly_sand.tga" normalpath="Data/Textures/Terrain/DetailTextures/pebbly_sand_normal.tga" materialpath="Data/Materials/gravel.xml" />
            <DetailMap colorpath="Data/Textures/Terrain/DetailTextures/sandwaves.tga" normalpath="Data/Textures/Terrain/DetailTextures/sandwaves_normal.tga" materialpath="Data/Materials/sand.xml" />
            <DetailMap colorpath="Data/Textures/Terrain/DetailTextures/rubble.tga" normalpath="Data/Textures/Terrain/DetailTextures/rubble_normal.tga" materialpath="Data/Materials/rock.xml" />
        </DetailMaps>
        <DetailObjects />
    </Terrain>
    <OutOfDate NavMesh="false" />
    <AmbientSounds />
    <Script>dev/max_dev.as</Script>
    <LevelScriptParameters>
        <parameter name="Achievements" type="string" val="flawless, no_injuries, no_kills" />
        <parameter name="Extra AO" type="string" val="0" />
        <parameter name="Level Boundaries" type="string" val="1" />
        <parameter name="Shared Camera" type="int" val="0" />
        <parameter name="Objectives" type="string" val="destroy_all" />
        <parameter name="Player Limit" type="int" val="8" />
        <parameter name="Sky Rotation" type="string" val="0" />
    </LevelScriptParameters>
    <Sky>
        <DomeTexture>Data/Textures/skies/bright_dusk.tga</DomeTexture>
        <SunAngularRad>0.153056</SunAngularRad>
        <SunColorAngle>353.589</SunColorAngle>
        <RayToSun r0="0.849746" r1="0.499721" r2="0.167961" />
        <ExtraAO>0</ExtraAO>
        <SkyRotation>0</SkyRotation>
    </Sky>
    <ActorObjects>
        <CameraObject t0="41.6294" t1="18.2004" t2="7.17772" s0="1" s1="1" s2="1" r0="0.939857" r1="1.49012e-08" r2="-0.341568" r3="0" r4="0.28639" r5="0.544967" r6="0.78803" r7="0" r8="0.186143" r9="-0.838458" r10="0.512191" r11="0" r12="0" r13="0" r14="0" r15="1" id="1">
            <parameters />
        </CameraObject>
        <EnvObject t0="45.769" t1="12.0035" t2="0.676721" s0="0.99999" s1="0.99999" s2="0.99999" r0="1" r1="0" r2="0" r3="0" r4="0" r5="1" r6="0" r7="0" r8="0" r9="0" r10="1" r11="0" r12="0" r13="0" r14="0" r15="1" id="2" color_r="0" color_g="0" color_b="0" type_file="Data/Objects/Buildings/arena/arena_vip_roof.xml">
            <parameters />
        </EnvObject>
        <Decal t0="42.0671" t1="11.8726" t2="2.53486" s0="1" s1="1" s2="1" r0="1" r1="0" r2="0" r3="0" r4="0" r5="1" r6="0" r7="0" r8="0" r9="0" r10="1" r11="0" r12="0" r13="0" r14="0" r15="1" id="3" type_file="Data/Objects/Decals/crete_stains/pooled_stain.xml" i="0" color_r="1" color_g="1" color_b="1">
            <parameters />
        </Decal>
        <ActorObject t0="41.5972" t1="13.121" t2="13.8292" s0="1" s1="1" s2="1" r0="-0.835025" r1="0" r2="-0.550212" r3="0" r4="0" r5="1" r6="0" r7="0" r8="0.550212" r9="0" r10="-0.835025" r11="0" r12="0" r13="0" r14="0" r15="1" id="8" type_file="Data/Objects/IGF_Characters/IGF_TurnerActor.xml" is_player="1">
            <parameters>
                <parameter name="Aggression" type="string" val="0.5" />
                <parameter name="Attack Damage" type="string" val="1.0" />
                <parameter name="Attack Knockback" type="string" val="1.0" />
                <parameter name="Attack Speed" type="string" val="1.0" />
                <parameter name="Block Follow-up" type="string" val="0.5" />
                <parameter name="Block Skill" type="string" val="0.5" />
                <parameter name="Character Scale" type="float" val="1" />
                <parameter name="Damage Resistance" type="string" val="1.0" />
                <parameter name="Ear Size" type="float" val="1" />
                <parameter name="Fat" type="float" val="0.5" />
                <parameter name="Ground Aggression" type="float" val="0.5" />
                <parameter name="Left handed" type="int" val="0" />
                <parameter name="Lives" type="int" val="1" />
                <parameter name="Movement Speed" type="string" val="1.0" />
                <parameter name="Muscle" type="float" val="0.5" />
                <parameter name="Static" type="int" val="0" />
                <parameter name="Teams" type="string" val="turner" />
            </parameters>
            <Connections>
                <Connection id="-1" />
            </Connections>
            <ItemConnections />
            <Palette>
                <Color label="Cloth" channel="0" red="1" green="1" blue="1" />
                <Color label="Rope" channel="1" red="1" green="1" blue="1" />
                <Color label="Fur" channel="2" red="0.537255" green="0.537255" blue="0.537255" />
            </Palette>
        </ActorObject>
        <Decal t0="43.1464" t1="12.1156" t2="4.39636" s0="1" s1="1" s2="1" r0="1" r1="0" r2="0" r3="0" r4="0" r5="1" r6="0" r7="0" r8="0" r9="0" r10="1" r11="0" r12="0" r13="0" r14="0" r15="1" id="4" type_file="Data/Objects/Decals/rugs/red_trim_rug.xml" i="0" color_r="1" color_g="1" color_b="1">
            <parameters />
        </Decal>
    </ActorObjects>
</Level>
~~~