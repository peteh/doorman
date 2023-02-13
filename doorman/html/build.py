import glob
from pathlib import Path



htmlFiles = glob.glob('*.html')
with open('../src/html.h', 'w') as f:
    
    for htmlFile in htmlFiles:
        file = open(htmlFile, mode='r')
        content = file.read()
        file.close()
        content = content.replace("\r\n", "")
        content = content.replace("\n", "")
        content = content.replace("\t", "")
        content = content.replace("  ", "")
        content = content.replace("\"", "\\\"")
        fname = Path(htmlFile).stem.upper()
        f.write("#define PAGE_%s \"%s\"\n" % (fname, content))
