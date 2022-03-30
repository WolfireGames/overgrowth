import os
import sys
from collections import OrderedDict
from googletrans import Translator

def YesOrNo(text):
    cont = raw_input(text)
    return not cont in ("n", "no", "No", "NO")

def Clear():
    os.system("cls" if os.name == "nt" else "clear")

def GetSayCharacter(line):
    words = line.split(" ")
    return words[0] + " " + words[1] + " " + words[2]

def PrintTranslation(original, translated):
    for line in original.values():
        print(line)
    print("------------------------------")
    for line in translated.values():
        print(line)

def Translate(lines):
    translatedLines = OrderedDict()
    while True:
        translatedLines.clear()
        for lineNr, line in lines.iteritems():
            Clear()
            PrintTranslation(lines, translatedLines)
            print(line)
            translation = raw_input("Translation: ")
            translatedLines[lineNr] = GetSayCharacter(line) + " \"" + translation + "\""

        Clear()
        PrintTranslation(lines, translatedLines)
        if YesOrNo("Keep? y/n: "):
            break
    return translatedLines

def TranslateDir(path):
    print("Translating directory: " + path)
    for filename in os.listdir(path):
        filename = path + "/" + filename
        if not filename.endswith(".txt"):
            continue

        lines = [];
        with open(filename) as file:
            lines = file.readlines()

        sayLines = OrderedDict() # key is line number, value is line
        lineNr = 0
        for line in lines:
            if line.startswith("say"):
                sayLines[lineNr] = line.rstrip()
            lineNr += 1
        if len(sayLines) > 0:
            for line in sayLines.values():
                print(line)

            if not YesOrNo("Translate? y/n: "):
                continue
            else:
                translated = Translate(sayLines)

            print("Writing file to " + filename)
            with open(filename + ".out", "w+") as file:
                for lineNr, line in enumerate(lines):
                    if lineNr in translated:
                        file.write(translated[lineNr] + "\n")
                    else:
                        file.write(line)
            if keepOriginals:
                os.rename(filename, filename + ".orig")
            os.rename(filename + ".out", filename)

def TranslateDirRec(path):
    filesAndDir = os.listdir(path)
    TranslateDir(path)
    for name in filesAndDir:
        current = path + "/" + name
        if os.path.isdir(current):
            TranslateDirRec(current)

def TranslateDirQuick(path, targetLanguage):
    print("Translating directory: " + path)
    for filename in os.listdir(path):
        filepath = path + "/" + filename
        if not filename.endswith(".txt"):
            continue

        lines = [];
        with open(filepath) as file:
            lines = file.readlines()

        sayLines = OrderedDict() # key is line number, value is line
        lineNr = 0
        for line in lines:
            if line.startswith("say"):
                sayLines[lineNr] = line.rstrip()
            lineNr += 1
        if len(sayLines) > 0:
            translated = {}
            count = 0
            for lineNr, line in sayLines.iteritems():
                if targetLanguage:
                    prefix = GetSayCharacter(line)
                    line = line[len(prefix):]
                    translated_line = translator.translate(line, dest=targetLanguage).text.replace(u"\u2019", "'").encode('utf-8')
                    translated[lineNr] = prefix + " " + translated_line
                else:
                    translated[lineNr] = GetSayCharacter(line) + " \"" + filename + ": " + str(count) + "\""
                    count += 1

            print("Writing file to " + filepath)
            with open(filepath + ".out", "w+") as file:
                for lineNr, line in enumerate(lines):
                    if lineNr in translated:
                        file.write(translated[lineNr] + "\n")
                    else:
                        file.write(line)
            if keepOriginals:
                os.rename(filepath, filepath + ".orig")
            os.rename(filepath + ".out", filepath)

def TranslateDirQuickRec(path, targetLanguage):
    filesAndDir = os.listdir(path)
    TranslateDirQuick(path, targetLanguage)
    for name in filesAndDir:
        current = path + "/" + name
        if os.path.isdir(current):
            TranslateDirQuickRec(current, targetLanguage)

recursive = None
quick = None
keepOriginals = True
targetLanguage = ""

inDir = (".")

options = sys.argv[1:]
if not options:
    print("Parameters:\n\t--in - Directory to translate\n\t--recursive - Translate files in subdirectories\n\t--dontkeep - Don't make copies of original files before overwriting\n\t--quick - Don't actually translte, just write out some placeholder text. Useful for testing things sometimes\n\t--autotrans - Automatically translate to the given language")
    sys.exit()

while options:
    if options[0] == "--recursive":
        recursive = True
        options = options[1:]
    elif options[0] == "--quick":
        quick = True
        options = options[1:]
    elif options[0] == "--in":
        inDir = options[1]
        options = options[2:]
    elif options[0] == "--dontkeep":
        keepOriginals = None
        options = options[1:]
    elif options[0] == "--autotrans":
        targetLanguage = options[1]
        options = options[2:]
    else:
        print("Didn't recognize option \"" + options[0] + "\"")
        sys.exit()

inDir = os.path.abspath(inDir);

if recursive:
    print("Recursively translating " + inDir)
else:
    print("Translating only " + inDir)
if targetLanguage:
    translator = Translator()
    if quick:
        print("Cannot have autotrans and quick at the same time, ignoring quick")
        quick = None

    print("Automatically translating to " + targetLanguage)
if quick:
    print("Performing quick translation")

if quick or targetLanguage:
    if recursive:
        TranslateDirQuickRec(inDir, targetLanguage)
    else:
        TranslateDirQuick(inDir, targetLanguage)
else:
    if recursive:
        TranslateDirRec(inDir)
    else:
        TranslateDir(inDir)
