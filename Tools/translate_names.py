import os
import re
import sys
import xml.etree.ElementTree as ElementTree
from xml.dom import minidom
from googletrans import Translator

in_file = "."
out_dir = "."
shortcode = ""
language = ""

options = sys.argv[1:]
while options:
    if options[0] == "--in":
        in_file = options[1]
        options = options[2:]
    elif options[0] == "--out":
        out_dir = options[1]
        options = options[2:]
    elif options[0] == "--shortcode":
        shortcode = options[1]
        options = options[2:]
    elif options[0] == "--language":
        language = options[1]
        options = options[2:]
    else:
        print("Didn't recognize option \"" + options[0] + "\"")
        sys.exit()

if not shortcode or not language:
    print("--shortcode and --language must be specificed")
    sys.exit()

in_file = os.path.abspath(in_file)
out_dir = os.path.abspath(out_dir)

def ExtractLevels(in_file):
    print("Extracting maps from " + in_file)
    tree = ElementTree.parse(in_file)
    root = tree.getroot()
    campaign = root.find("Campaign")
    levels = {}
    for level in campaign.findall("Level"):
        title = level.attrib["title"]
        path = level.text
        levels[title] = path
    return levels

levels = ExtractLevels(in_file)

def ExtractLoadTip(in_file):
    print("Extracting load tip from " + in_file)
    with open(in_file) as file:
        xml = file.read();
        version = xml[0:xml.index('\n') + 1]
        root = ElementTree.fromstring(re.sub(r"(<\?xml[^>]+\?>)", r"\1<root>", xml) + "</root>")
        found = root.find("LevelScriptParameters")
        if not found is None:
            for parameter in found.findall("parameter"):
                if parameter.attrib["name"] == "Load Tip":
                    return parameter.attrib["val"].encode('utf-8')

in_dir = in_file[0:in_file.index("Data") + 5] + "Levels/"


translated_tips = {}
load_tips = []
level_titles = []
for title, path in levels.iteritems():
    tip = ExtractLoadTip(in_dir + path)
    if tip:
        translated_tips[path] = len(load_tips)
        load_tips.append(tip)
        level_titles.append(title)

translator = Translator()
if language == "English" or language == "english":
    print("Translating " + str(len(load_tips)) + " load tips to swedish and back...")
    load_tips = translator.translate(load_tips, dest = "Swedish", src = "English")
    for index, text in enumerate(load_tips):
        load_tips[index] = text.text
    load_tips = translator.translate(load_tips, dest = "English", src = "Swedish")
    print("Translating " + str(len(load_tips)) + " level titles to swedish and back...")
    level_titles = translator.translate(level_titles, dest = "Swedish", src = "English")
    for index, text in enumerate(level_titles):
        level_titles[index] = text.text
    level_titles = translator.translate(level_titles, dest = "English", src = "Swedish")
else:
    print("Translating " + str(len(load_tips)) + " load tips to " + language + "...")
    load_tips = translator.translate(load_tips, dest = language, src = "English")
    print("Translating " + str(len(load_tips)) + " level titles to " + language + "...")
    level_titles = translator.translate(level_titles, dest = language, src = "English")
print("Done translating")

for path, index in translated_tips.iteritems():
    full_path = out_dir + "/Localized/" + shortcode + "/Data/Levels/"  + path
    (dir_path, file_path) = os.path.split(full_path)
    file_path = file_path[0:file_path.rfind('.')] + "_meta.xml"
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)

    full_path = dir_path + "/" + file_path;
    print("Writing " + full_path)
    root = ElementTree.Element("LevelMeta")
    ElementTree.SubElement(root, "Title").text = level_titles[index].text
    ElementTree.SubElement(root, "LoadingTip").text = load_tips[index].text
    tree_string = ElementTree.tostring(root)
    out_xml = minidom.parseString(tree_string).toprettyxml(indent = "    ")
    with open(full_path, "w") as file:
        file.write(out_xml.encode('utf-8'))

