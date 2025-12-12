# 04.12.25 Hva har vi gjort i dag?
- Fått bygget node A
- Flashet thingy
- laget repository
- thingy LED vil ikke blinke
- får ingen utskrift i terminal

Dekoder tips
seq      = b[0]
temp     = int16_le(b[1:3]) / 100.0
rh       = uint16_le(b[3:5]) / 10.0
lat      = int32_le(b[5:9]) / 1e5
lon      = int32_le(b[9:13]) / 1e5
