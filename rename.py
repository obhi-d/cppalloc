import re
import sys

words = ""
with open(sys.argv[1], 'r') as f:
  words = f.read();

REG = r"(.+?)([A-Z])"

words = words.splitlines();

def snake(match):
    return match.group(1).lower() + "_" + match.group(2).lower()

results = [re.sub(REG, snake, w, 0) for w in words]

with open(sys.argv[1] + '.hpp', 'w') as f:
  f.write(words);

