import glob
from pathlib import Path
import os

htmlFiles = glob.glob('*.html')
tpl_files = glob.glob('*.tpl')
tpl = {}
for tpl_file in tpl_files:
    file = open(tpl_file, mode='r')
    name = os.path.splitext(tpl_file)[0]
    tpl[name] = file.read()
    file.close()
#print(tpl)
    
with open('../src/html.h', 'w') as f:
    f.write("#include<Arduino.h>\n")
    for htmlFile in htmlFiles:
        file = open(htmlFile, mode='r')
        content = file.read()
        file.close()
        for key, value in tpl.items():
            content = content.replace(f"%{key}%", value)
        content = content.replace("\r\n", "\\r\\n")
        content = content.replace("\n", "\\n")
        content = content.replace("\t", "\\t")
        #content = content.replace("  ", "")
        content = content.replace("\"", "\\\"")
        fname = Path(htmlFile).stem.upper()
        f.write("const PROGMEM char PAGE_%s[] = \"%s\";\n" % (fname, content))
