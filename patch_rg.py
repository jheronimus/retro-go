import re

with open('rg_tool.py', 'r') as f:
    content = f.read()

new_apps = """PROJECT_APPS = {
  'launcher':     [0, 16, 1048576],
  'libxnes':      [0, 16, 1048576],
  'smsplus-gx':   [0, 16, 1048576],
  'temper':       [0, 16, 1048576],
  'walnut_cgb':   [0, 16, 1048576],
}"""

content = re.sub(r'PROJECT_APPS = \{.*?\n\}', new_apps, content, flags=re.DOTALL)

with open('rg_tool.py', 'w') as f:
    f.write(content)
