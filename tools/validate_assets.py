"""Validate asset structure and Android XML, not visual/device behavior."""
from pathlib import Path
import math, struct, xml.etree.ElementTree as ET
p=Path(__file__).resolve().parents[1]
lines=(p/'app/src/main/assets/models/critter.obj').read_text().splitlines()
v=[list(map(float,l.split()[1:])) for l in lines if l.startswith('v ')]
n=[l for l in lines if l.startswith('vn ')];uv=[l for l in lines if l.startswith('vt ')]
faces=[l.split()[1:] for l in lines if l.startswith('f ')]
assert len(v)==len(n)==len(uv) and 0<len(v)<65536
assert all(math.isfinite(c) for row in v for c in row)
assert min(row[1] for row in v)>=0
for face in faces:
    assert len(face)==3
    assert all(1<=int(k)<=len(v) for f in face for k in f.split('/'))
assert len(faces)*3<65536
png=(p/'app/src/main/assets/models/critter.png').read_bytes()
assert png.startswith(b'\x89PNG\r\n\x1a\n') and struct.unpack('!II',png[16:24])==(64,16)
for f in (p/'app/src/main').rglob('*.xml'): ET.parse(f)
print(f'PASS: {len(v)} vertices, {len(faces)} triangles, PNG dimensions, all Android XML')
