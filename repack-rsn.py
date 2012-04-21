#!/usr/bin/python

import commands
import glob
import os
import shutil
import struct
import sys

UNPACK_DIR = "unpack-directory"

# process command line argument
if len(sys.argv) < 2:
  print "USAGE: repack-rsn.py <file.rsn>"
  sys.exit(1)
rsnfile = sys.argv[1]
print "repacking " + rsnfile

# unpack archive to a temporary location
command_string = "unrar x " + rsnfile + " " + UNPACK_DIR + "/ -y"
print command_string
(status, output) = commands.getstatusoutput(command_string)
if status != 0:
  print "could not unpack " + rsnfile

# get an alphabetical list of all SPC files in the archive
spcfiles = glob.glob(UNPACK_DIR + "/*.spc")
spcfiles.sort()

# the first offset will be located after the header (16 bytes), file count
# (4 bytes) and list of offsets (number of files * 4)
offset = 16 + 4 + (4 * len(spcfiles))
fileblob = ""

# write a new file
packfile = open(rsnfile + ".gamemusic", "wb")
packfile.write("Game Music Files")
packfile.write(struct.pack(">I", len(spcfiles)))

for spc in spcfiles:
  filedata = open(spc, "rb").read()
  fileblob += filedata
  packfile.write(struct.pack(">I", offset))
  offset += len(filedata)
  print spc

packfile.write(fileblob)
packfile.close()

# clean up
print "deleting temporary unpacked files..."
shutil.rmtree(UNPACK_DIR)
