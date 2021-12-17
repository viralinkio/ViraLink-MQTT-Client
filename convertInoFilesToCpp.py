import os
import glob

files = glob.glob("examples/*/*.ino")

for f in files:
    print(f)
    ff = f.replace(".ino", ".cpp")
    os.rename(f, ff)
