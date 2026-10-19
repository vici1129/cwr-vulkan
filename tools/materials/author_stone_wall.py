"""Author new stone/mortar relief and unlit reflectance; never reads stock pixels.

Python + Pillow. Usage: python author_stone_wall.py build/stone-wall-art
Lossless ARGB8888 PAA maps are emitted with a full mip chain for existing loading.
UV atlas: upper 1/16 fired-clay cap, remaining area rubble. Assumed body 4x1.2m.
The height field represents shallow stone bevels and chisel marks, not lighting.
"""
import math
import random
import sys
import struct
from array import array
from pathlib import Path
from PIL import Image

N = 1024
out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
rng = random.Random(917)
cols, rows = 13, 7
seeds = {(x, y): (x + rng.uniform(.18, .82), y + rng.uniform(.18, .82),
                      rng.uniform(.19, .31), rng.uniform(.82, .92), rng.uniform(-1, 1))
         for y in range(-1, rows + 1) for x in range(-1, cols + 1)}
h = array('f', [0]) * (N * N)
color, rough = bytearray(N * N * 4), bytearray(N * N)

def srgb(v):
    return round(255 * (12.92 * v if v <= .0031308 else 1.055 * v ** (1 / 2.4) - .055))

def smooth(v):
    v = max(0, min(1, v))
    return v * v * (3 - 2 * v)

for y in range(N):
    v = (y + .5) / N
    for x in range(N):
        u = (x + .5) / N
        i = y * N + x
        if v < 1 / 16:
            joint = min((u * 30) % 1, 1 - (u * 30) % 1)
            stone = smooth((joint - .015) / .075)
            h[i] = .003 * stone
            rgb = tuple(.26 * (1-stone) + c * stone for c in (.27, .13, .075))
            r = .9
        else:
            xx, yy = u * cols, (v - 1 / 16) * rows / (15 / 16)
            cx, cy = int(xx), int(yy)
            nearest = []
            for sy in range(cy - 1, cy + 2):
                for sx in range(cx - 1, cx + 2):
                    px, py, a, r, phase = seeds[sx, sy]
                    d = (xx - px)**2 + (yy - py)**2
                    nearest.append((d, a, r, phase))
            nearest.sort()
            d, a, r, phase = nearest[0]
            # Difference of nearest squared distances approximates distance to
            # the Voronoi edge. This creates explicit bevel geometry, in metres.
            edge = (nearest[1][0] - d) * .10 + .002*math.sin(u*751+math.sin(v*439)*2)*math.sin(v*633)
            stone = smooth((edge - .003) / .012)
            grain = (math.sin(u*617 + phase*13) * math.sin(v*413 + phase*7) +
                     .35*math.sin(u*1321 + v*941))
            h[i] = .007*stone + (.00025*grain+.0006*math.sin(u*131+phase)*math.sin(v*117))*stone
            mottling = .02*math.sin(u*91+phase*5)*math.sin(v*67)+.008*grain
            a = max(.1, a + mottling)
            rgb = tuple(.24*(1-stone) + a*c*stone for c in (1.04, 1.01, .96))
            r = .96*(1-stone) + r*stone
        color[i*4:i*4+4] = bytes((*[srgb(c) for c in rgb], 255))
        rough[i] = round(255 * min(1, r / .9))

normal = bytearray(N * N * 4)
for y in range(N):
    for x in range(N):
        i = y*N+x
        dx = (h[y*N+min(x+1,N-1)] - h[y*N+max(0,x-1)]) * N / 8
        dy = (h[min(y+1,N-1)*N+x] - h[max(0,y-1)*N+x]) * N / 2.56
        inv = 1 / math.sqrt(dx*dx+dy*dy+1)
        normal[i*4:i*4+4] = bytes((round(127.5*(1-dx*inv)), round(127.5*(1-dy*inv)),
                                  round(127.5*(1+inv)), rough[i]))
def write_maps(name, data):
    image = Image.frombytes('RGBA',(N,N),data)
    image.save(out/(name+'.png'))
    with (out/(name+'.paa')).open('wb') as f:
        f.write(struct.pack('<HH',0x8888,0))
        while True:
            r,g,b,a = image.split()
            payload=Image.merge('RGBA',(b,g,r,a)).tobytes()
            f.write(struct.pack('<HH',*image.size)+len(payload).to_bytes(3,'little')+payload)
            if image.width == 1: break
            image=image.resize((image.width//2,image.height//2),Image.Resampling.BOX)
        f.write(b'\0'*4)
write_maps('albedo',color)
write_maps('normal-roughness',normal)
print(out)
