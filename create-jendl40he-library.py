import sys
import os

import wget
import zipfile

import openmc
import subprocess


neutronurl = "https://rpg.jaea.go.jp/download/ace_lib/acej40he/neu.zip"
xsdirurl = "https://rpg.jaea.go.jp/download/ace_lib/acej40he/xsdir_j40he"

neutronzipfilename = os.path.basename(neutronurl)
if not os.path.exists(neutronzipfilename):
    wget.download(neutronurl)
else:
    print("Using already downloaded file '{:s}'. If further errors occur, delete the file.".format(neutronzipfilename))

xsdirfilename = os.path.basename(xsdirurl)
if not os.path.exists(xsdirfilename):
    wget.download(xsdirurl)
else:
    print("Using already downloaded file '{:s}'. If further errors occur, delete the file.".format(xsdirfilename))

print("Unpacking ACE files")
with zipfile.ZipFile(neutronzipfilename,"r") as zip_ref:
    zip_ref.extractall("./")    

print("Calling OpenMC conversion script")
subprocess.run(["openmc-ace-to-hdf5", "--xsdir", xsdirfilename, "-d", "jendl40he"])
    
