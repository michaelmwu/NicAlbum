# Sort playlists by path / filename while preserving Album Shuffle blocks
# sort.py PLAYLIST > OUTFILE
# 
# On Windows you need to output to a different file and copy it over the original, otherwise the original one will be erased

import sys
from string import rstrip
import re

class Song:
    length = 0
    title = ""
    path = ""
    type = 0
    
    def Print(self):
        global LABEL
        print LABEL + str(self.length) + "," + self.title
        print self.path

class Group:
    length = -1
    title = None
    path = None
    songs = None
    type = 1
    
    def __init__(self):
        self.songs = []
        self.path = ""
    
    def Print(self):
        global LABEL, SEPHEAD, SEPDISP
        print LABEL + str(self.length) + "," + self.title
        print SEPHEAD + self.path + SEPDISP + self.title
        
        for song in self.songs:
            song.Print()
    
    def AddSong(self, song):
        if len(self.path) == 0:
            self.path = song.path
    
        self.songs.append(song)

class Random:
    length = -1
    title = "=======Random======="
    
    def Print(self):
        global LABEL, SEPHEAD, SEPDISP
        print LABEL + str(self.length) + "," + self.title
        print SEPHEAD + SEPDISP + self.title
        
if len(sys.argv) > 1:
    file = sys.argv[1]
else:
    print >> sys.stderr, "Needs an argument\n"
    sys.exit()

f = open(file, 'r')

f.readline() # Remove the header

HEAD = "#EXTM3U"
LABEL = "#EXTINF:"
SEPHEAD = "separator://:"
SEPDISP = "?S="

MINSEPLEN = 5
SEPALBUM = "-----"
SEPRANDOM = "====="

# Parse stuff
items = []

parsemode = 0

labelre = re.compile("^" + re.escape(LABEL) + "(-?\d+),(.+)$")
sepre = re.compile("^" + re.escape(SEPHEAD) + "(.*)" + re.escape(SEPDISP) + "(.+)$")

curgroup = None

while True:
    line = f.readline()
    
    if rstrip(line) == "":
        break
    
    m = labelre.search(line)
    
    if not m:
        print >> sys.stderr, "FAILED LABEL: " + "hi"
        sys.exit()
    
    length = m.group(1)
    title = rstrip(m.group(2))
    
    line = f.readline()
    
    if title[:MINSEPLEN] == SEPALBUM:
        parsemode = 1
        curgroup = Group()
        curgroup.title = title;
        m = sepre.search(line)
        
        if not m:
            print >> sys.stderr, "FAILED SEPARATOR"
            sys.exit()
        
        items.append(curgroup)
    elif title[:MINSEPLEN] == SEPRANDOM:
        parsemode = 0
    else:
        item = Song()
        item.length = length
        item.title = rstrip(title)
        item.path = rstrip(line)
    
        if parsemode == 0:
            items.append(item)
        elif parsemode == 1:
            curgroup.AddSong(item)

f.close()

writemode = 0

items.sort(key=lambda item: item.path)

random = Random()

print HEAD
for item in items:
    if item.type == 0:
        if writemode == 1:
            random.Print()
        writemode = 0
    elif item.type == 1:
        writemode = 1
    
    item.Print()
