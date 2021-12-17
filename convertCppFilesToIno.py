import os
import glob

files = glob.glob("examples/*/*.cpp")

for f in files:
    print(f)
    ff = f.replace(".cpp", ".ino")
    os.rename(f, ff)
