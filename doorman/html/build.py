import glob
from pathlib import Path



htmlFiles = glob.glob('*.html')
with open('../src/html.h', 'w') as f:
    f.write("#include<Arduino.h>\n")
    for htmlFile in htmlFiles:
        file = open(htmlFile, mode='r')
        content = file.read()
        file.close()
        content = content.replace("\r\n", "\\r\\n")
        content = content.replace("\n", "\\n")
        content = content.replace("\t", "\\t")
        content = content.replace("  ", "")
        content = content.replace("\"", "\\\"")
        fname = Path(htmlFile).stem.upper()
        f.write("const char PAGE_%s[] = \"%s\";\n" % (fname, content))
