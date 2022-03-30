import os
import re
import sys
import xml.etree.ElementTree as ElementTree

PARSE = 0
SKIP = 1
QUIT = 2

def YesOrNo(text):
    cont = raw_input(text)
    if cont in ("n", "no", "No", "NO"):
        return SKIP
    elif cont in ("q", "quit", "Q", "Quit", "e", "exit", "Exit"):
        return QUIT
    else:
        return PARSE

def ParseDirectory(absPath, outDir, outDirPostfix):
    print("Parsing directory " + absPath)
    for filename in sorted(os.listdir(absPath)):
        fullPath = absPath + "/" + filename
        if recursive and os.path.isdir(fullPath):
            if not outDirPostfix:
                ParseDirectory(fullPath, outDir + "/" + filename, filename)
            else:
                ParseDirectory(fullPath, outDir + "/" + filename, outDirPostfix + "/" + filename)
        else:
            if filename.endswith(".xml"):
                parse = True
                if selective:
                    action = YesOrNo("Extract " + abspath + "/" + filename + "? (y/n/q) ")
                    if action is QUIT:
                        break
                    elif action is SKIP:
                        parse = None

                if parse:
                    print("Parsing " + fullPath)
                    with open(fullPath) as file:
                        extractedDialogues = 0
                        xml = file.read()
                        versionString = xml[0:xml.index('\n')+1]
                        root = ElementTree.fromstring(re.sub(r"(<\?xml[^>]+\?>)", r"\1<root>", xml) + "</root>")
                        found = root.find("ActorObjects")
                        if not found is None:
                            for placeholder in found.findall("PlaceholderObject"):
                                if int(placeholder.attrib["special_type"]) == 0:
                                    parameters = placeholder.find("parameters")
                                    displayName = ""
                                    script = ""
                                    for parameter in parameters.findall("parameter"):
                                        name = parameter.attrib["name"]
                                        if name == "DisplayName":
                                            displayName = parameter.attrib["val"].encode('utf-8')
                                        elif name == "Script":
                                            script = parameter.attrib["val"].encode('utf-8')
                                            removeParameter = parameter
                                        elif name == "Dialogue":
                                            dialogueParameter = parameter
                                        
                                        if displayName and script:
                                            break
                                    if not displayName or not script:
                                        continue
                                    parameters.remove(removeParameter)

                                    fileWithoutExt = os.path.splitext(filename)[0]

                                    dialogueParameter.attrib["val"] = scriptPrefix + outDirPostfix + "/" + fileWithoutExt + "/" + displayName + ".txt"

                                    outdir = outDir + "/" + fileWithoutExt;
                                    if not os.path.exists(outdir):
                                        os.makedirs(outdir)

                                    outfile = open(outdir + "/" + displayName + ".txt", "wb+")
                                    outfile.write(script)
                                    outfile.close()
                                    extractedDialogues += 1
                    if extractedDialogues > 0:
                        tree = ElementTree.tostring(root).decode()
                        tree = tree[tree.index('\n') + 1 : tree.rindex('\n')]
                        if replaceOriginal:
                            with open(fullPath, "w+") as file:
                                file.write(versionString)
                                file.write(tree)
                            print("Output file to " + fullPath)
                        if outputXml:
                            with open(outDir + "/" + filename, "w+") as file:
                                file.write(versionString)
                                file.write(tree)
                            print("Output file to " + outDir + "/" + filename)
                        print("Extracted " + str(extractedDialogues) + " dialogues")

inDir = "."
outDir = "out"
scriptPrefix = ""
replaceOriginal = None
outputXml = None
selective = None
recursive = None

options = sys.argv[1:]
while options:
    if options[0] == "--in":
        inDir = options[1]
        options = options[2:]
    elif options[0] == "--out":
        outDir = options[1]
        options = options[2:]
    elif options[0] == "--prefix":
        scriptPrefix = options[1]
        options = options[2:]
    elif options[0] == "--replace":
        replaceOriginal = True
        options = options[1:]
    elif options[0] == "--dryrun":
        outputXml = True
        options = options[1:]
    elif options[0] == "--selective":
        selective = True
        options = options[1:]
    elif options[0] == "--recursive":
        recursive = True
        options = options[1:]
    else:
        print("Didn't recognize option \"" + options[0] + "\"")
        sys.exit()

inDir = os.path.abspath(inDir)
outDir = os.path.abspath(outDir)

if recursive:
    print("Recursively parsing files at " + inDir)
else:
    print("Parsing files at " + inDir)
print("Placing dialogues at " + outDir)
if replaceOriginal:
    print("Replacing original level .xml")
else:
    print("Keeping original level .xml")
if(outputXml):
    print("Outputting xml in out dir")

ParseDirectory(inDir, outDir, "")
