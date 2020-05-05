from datetime import datetime
from os.path import dirname, join
cwd = dirname(__file__)
now = datetime.now()
dt_string = now.strftime("%d/%m/%Y %H:%M")
print("date and time =", dt_string)
with open(join(cwd, './Source/version.h'), 'w') as f:
    chars = ["'{0}'".format(char) for char in dt_string]
    code = """extern const uint8 zclFreePadApp_DateCode[] = {{ {0}, {1} }};""".format(len(chars), ', '.join(chars))
    f.write(code)