#!/usr/bin/env python3
import collections, glob, os, re, subprocess, sys, tempfile

LIMIT = int(sys.argv[1]) if len(sys.argv) > 1 else 2304
ROOT = "isr_handler"
FLAGS = ("-m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -mgeneral-regs-only "
         "-mno-sse -mno-sse2 -mno-mmx -mno-80387 -fcf-protection=none -nostdlib -fcallgraph-info=su -c").split()

tmp = tempfile.mkdtemp()

sources = [os.path.join(d, f) for d, _, fs in os.walk(".") for f in fs
           if f.endswith(".c") and not d.startswith(("./User", "./tests", "./.git"))]

for i, src in enumerate(sorted(sources)):
    subprocess.run(["gcc"] + FLAGS + [src, "-o", os.path.join(tmp, "%d.o" % i)],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

nodes, edges, byname = {}, collections.defaultdict(set), collections.defaultdict(list)

for f in glob.glob(os.path.join(tmp, "*.ci")):
    text = open(f).read()
    for m in re.finditer(r'node: \{ title: "([^"]+)" label: "([^"]*)"', text):
        size = re.search(r"(\d+) bytes", m.group(2))
        if size:
            name = m.group(1).split(":")[-1]
          
            nodes[m.group(1)] = (name, int(size.group(1)))
          
            byname[name].append(m.group(1))
    for m in re.finditer(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"', text):
        edges[m.group(1)].add(m.group(2))

if ROOT not in byname:
    sys.exit("stack_depth: could not find %s (is gcc 10+ installed?)" % ROOT)

memo, active = {}, set()
def depth(t):
    if t in memo: return memo[t]
    if t in active: return (0, [])
  
    active.add(t)
   
    name, size = nodes[t]
    best = (0, [])
 
    for e in edges.get(t, ()):
        r = e if e in nodes else (byname[e][0] if len(byname.get(e, [])) == 1 else None)
        if r and "indirect" not in e:
            d = depth(r)
            if d[0] > best[0]: best = d
    active.discard(t)
  
    memo[t] = (size + best[0], [(name, size)] + best[1])
  
    return memo[t]

total, chain = depth(byname[ROOT][0])

print("interrupt path worst case: %d bytes (limit %d, a task stack is 4096)" % (total, LIMIT))

print("  " + " -> ".join("%s(%d)" % c for c in chain))

if total > LIMIT:
    print("FAIL: the interrupt path got deeper than the limit; shrink a big local buffer on that path")
    sys.exit(1)
print("PASS")
