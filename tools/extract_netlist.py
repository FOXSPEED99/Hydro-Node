import olefile, struct, sys
#!/usr/bin/env python3
"""
Extract a netlist from the Hydro Node Altium schematic.

The schematic has no net labels and no power ports - every connection is drawn
geometry - so connectivity has to be rebuilt from wire vertices, junctions and
pin coordinates. Re-run this after any schematic edit to check the netlist
against HYDRO-NODE-REFERENCE.md section 1.

    pip install olefile
    python3 tools/extract_netlist.py
"""

from collections import defaultdict

p = sys.argv[1] if len(sys.argv) > 1 else "Hydro Node Device/Hydro Node Parts & Schematic/Schematic/Hydro-Node.SchDoc"
o=olefile.OleFileIO(p); d=o.openstream('FileHeader').read()
i=0; recs=[]
while i+4<=len(d):
    ln=struct.unpack('<I',d[i:i+4])[0]; n=ln&0xFFFFFF; i+=4
    recs.append(d[i:i+n].rstrip(b'\x00').decode('latin-1')); i+=n

def parse(s):
    o={}
    for kv in s.split('|'):
        if '=' in kv:
            k,v=kv.split('=',1); o[k]=v
    return o

P=[parse(r) for r in recs]
comps={}   # idx -> dict
for idx,r in enumerate(P):
    if r.get('RECORD')=='1':
        comps[idx-1]={'lib':r.get('LibReference'),'des':None,'pins':[]}
for idx,r in enumerate(P):
    if r.get('RECORD')=='34':
        oi=int(r.get('OwnerIndex',-1))
        if oi in comps: comps[oi]['des']=r.get('Text')

pins=[]
for idx,r in enumerate(P):
    if r.get('RECORD')=='2':
        oi=int(r.get('OwnerIndex',-1))
        x=int(r['Location.X']); y=int(r['Location.Y'])
        pl=int(r.get('PinLength',0)); pc=int(r.get('PinConglomerate',0))
        ori=pc&3  # 0=right,1=up,2=left,3=down
        dx,dy=[(1,0),(0,1),(-1,0),(0,-1)][ori]
        # Location is the *connect* end? test both
        pin={'owner':oi,'name':r.get('Name'),'des':r.get('Designator'),
             'a':(x,y),'b':(x+dx*pl,y+dy*pl)}
        pins.append(pin)
        if oi in comps: comps[oi]['pins'].append(pin)

wires=[]
for r in P:
    if r.get('RECORD')=='27':
        n=int(r['LocationCount'])
        pts=[(int(r['X%d'%k]),int(r['Y%d'%k])) for k in range(1,n+1)]
        wires.append(pts)
juncs=set()
for r in P:
    if r.get('RECORD')=='29':
        juncs.add((int(r['Location.X']),int(r['Location.Y'])))

segs=[]
for w in wires:
    for a,b in zip(w,w[1:]): segs.append((a,b))

def on_seg(pt,s):
    (x1,y1),(x2,y2)=s; x,y=pt
    if (x2-x1)*(y-y1)-(y2-y1)*(x-x1)!=0: return False
    return min(x1,x2)<=x<=max(x1,x2) and min(y1,y2)<=y<=max(y1,y2)

# union-find over segment indices + pin ids
parent={}
def find(a):
    parent.setdefault(a,a)
    while parent[a]!=a: parent[a]=parent[parent[a]]; a=parent[a]
    return a
def uni(a,b): parent[find(a)]=find(b)

for i,s in enumerate(segs): find(('S',i))
# segments sharing endpoints, or endpoint of one on another
for i,s in enumerate(segs):
    for j,t in enumerate(segs):
        if i>=j: continue
        pts_i=set(s); pts_j=set(t)
        if pts_i & pts_j: uni(('S',i),('S',j)); continue
        if any(on_seg(p,t) for p in s) or any(on_seg(p,s) for p in t):
            uni(('S',i),('S',j)); continue
        # crossing with explicit junction
        for jp in juncs:
            if on_seg(jp,s) and on_seg(jp,t): uni(('S',i),('S',j)); break

pinnodes=[]
for k,pin in enumerate(pins):
    cd=comps.get(pin['owner'],{}).get('des','?')
    label="%s.%s(%s)"%(cd,pin['des'],pin['name'])
    hit=False
    for end in ('a','b'):
        pt=pin[end]
        for i,s in enumerate(segs):
            if on_seg(pt,s): uni(('P',k),('S',i)); hit=True
    pinnodes.append((label,hit,pin))

nets=defaultdict(list)
for k,(label,hit,pin) in enumerate(pinnodes):
    if hit: nets[find(('P',k))].append(label)

print("=== COMPONENTS ===")
for idx,c in sorted(comps.items(), key=lambda kv: str(kv[1]['des'])):
    print(c['des'], c['lib'], len(c['pins']))
print()
print("=== NETS (%d) ==="%len(nets))
for n,(k,v) in enumerate(sorted(nets.items(), key=lambda kv:-len(kv[1])),1):
    if len(v)>=1: print("NET%02d [%d]: %s"%(n,len(v),', '.join(sorted(v))))
print()
print("=== UNCONNECTED PINS ===")
for label,hit,pin in pinnodes:
    if not hit: print(label, pin['a'], pin['b'])
