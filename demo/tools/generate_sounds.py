"""Syntezuje próbki dźwięków do sceny demo "dzwiek" (WAV, 22050 Hz, 16 bit, mono)."""
import math, random, struct, sys, wave

RATE = 22050
out = sys.argv[1]
random.seed(7)

def save(name, samples):
    peak = max(1e-9, max(abs(s) for s in samples))
    scale = 0.85 / peak
    with wave.open(f"{out}/{name}", "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, s * scale)) * 32767)) for s in samples))

def env(i, n, attack=0.005, release=None):
    t = i / RATE
    a = min(1.0, t / attack) if attack > 0 else 1.0
    r = 1.0 if release is None else max(0.0, 1 - i / n) ** release
    return a * r

def tone(freq, t, shape="sine"):
    p = (freq * t) % 1.0
    if shape == "sine": return math.sin(2 * math.pi * p)
    if shape == "square": return 1.0 if p < 0.5 else -1.0
    if shape == "tri": return 4 * abs(p - 0.5) - 1
    if shape == "saw": return 2 * p - 1

# moneta: dwa tony
n = int(RATE * 0.28); s = []
for i in range(n):
    t = i / RATE
    f = 988 if t < 0.07 else 1319
    s.append(0.5 * tone(f, t, "square") * env(i, n, 0.002, 1.5))
save("coin.wav", s)

# kroki: stuknięcia szumu filtrowanego dolnoprzepustowo, trzy barwy
for k, (cut, length) in enumerate([(0.08, 0.10), (0.12, 0.09), (0.06, 0.11)], start=1):
    n = int(RATE * length); s = []; y = 0.0
    for i in range(n):
        y += cut * (random.uniform(-1, 1) - y)
        s.append(y * env(i, n, 0.001, 3))
    save(f"step{k}.wav", s)

# uderzenie: opadający niski ton + krótki szum
n = int(RATE * 0.35); s = []; y = 0.0
for i in range(n):
    t = i / RATE
    f = 140 * math.exp(-t * 8) + 50
    y += 0.3 * (random.uniform(-1, 1) - y)
    s.append((math.sin(2 * math.pi * f * t) + y * math.exp(-t * 40)) * env(i, n, 0.001, 2))
save("hit.wav", s)

# klik interfejsu
n = int(RATE * 0.03)
save("click.wav", [tone(2000, i / RATE) * env(i, n, 0.0005, 2) for i in range(n)])

# ogień: szum brązowy z trzaskami, zapętlony bez szwu (przenikanie końca z początkiem)
n = int(RATE * 3.0); fade = int(RATE * 0.3); s = []; y = 0.0; pop = 0.0
for i in range(n + fade):
    y = 0.995 * y + 0.03 * random.uniform(-1, 1)
    if random.random() < 0.0015: pop = random.uniform(0.5, 1.0)
    pop *= 0.985
    s.append(y + pop * random.uniform(-1, 1) * 0.6)
loop = s[:n]
for i in range(fade):
    a = i / fade
    loop[i] = loop[i] * a + s[n + i] * (1 - a)
save("fire.wav", loop)

def song(name, bpm, chords, shape, bass_shape, bars=4):
    beat = 60 / bpm
    n = int(RATE * beat * 4 * bars); s = [0.0] * n
    step = beat / 2  # ósemki
    for bar in range(bars):
        root, third, fifth = chords[bar % len(chords)]
        notes = [root, third, fifth, root * 2, fifth, third, root * 2, fifth]
        for k, f in enumerate(notes):
            start = int(RATE * (bar * 4 * beat + k * step)); length = int(RATE * step)
            for i in range(length):
                if start + i < n:
                    s[start + i] += 0.25 * tone(f * 2, i / RATE, shape) * env(i, length, 0.004, 1.2)
        start = int(RATE * bar * 4 * beat); length = int(RATE * 4 * beat)
        for i in range(length):
            if start + i < n:
                s[start + i] += 0.35 * tone(root / 2, i / RATE, bass_shape) * env(i, length, 0.01, 0.6)
    save(name, s)

C, D, E, F, G, A, B = 261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88
song("theme.wav", 112, [(C, E, G), (A / 2, C, E), (F / 2, A / 2, C), (G / 2, B / 2, D)], "tri", "sine")
song("boss.wav", 150, [(A / 2, C, E), (A / 2, C, E), (F / 2, A / 2, C), (E / 2, G / 2 * 1.0595, B / 2)], "square", "saw")
print("ok")
