#!/usr/bin/python

import commands
import glob
import os
import shutil
import struct
import sys

UNPACK_DIR = "unpack-directory"
TEMP_7Z_FILE = "file.7z"

# process command line argument
if len(sys.argv) < 2:
  print "USAGE: repack-vgm-7z.py </path/to/file.rsn>"
  sys.exit(1)
vgm7zfile = sys.argv[1]
print "repacking " + vgm7zfile

# unpack archive to a temporary location
os.mkdir(UNPACK_DIR)
os.chdir(UNPACK_DIR)
shutil.copy(vgm7zfile, TEMP_7Z_FILE)
command_string = "p7zip -d " + TEMP_7Z_FILE
print command_string
(status, output) = commands.getstatusoutput(command_string)
if status != 0:
  print "could not unpack " + vgm7zfile

# get an alphabetical list of all SPC files in the archive
vgmfiles = glob.glob("*.vgm")
vgmfiles.sort()

# the first offset will be located after the header (16 bytes), file count
# (4 bytes) and list of offsets (number of files * 4)
offset = 16 + 4 + (4 * len(vgmfiles))
fileblob = ""

# write a new file
packfile = open(vgm7zfile + ".gamemusic", "wb")
packfile.write("Game Music Files")
packfile.write(struct.pack(">I", len(vgmfiles)))

for vgm in vgmfiles:
  filedata = open(vgm, "rb").read()
  fileblob += filedata
  packfile.write(struct.pack(">I", offset))
  offset += len(filedata)
  print vgm

packfile.write(fileblob)
packfile.close()

# clean up
print "deleting temporary unpacked files..."
os.chdir("..")
shutil.rmtree(UNPACK_DIR)
