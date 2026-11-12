"""Neuron Forge - synthèse des effets sonores du jeu (Python pur, sans dépendance).

Chaque son est généré en WAV 16 bits mono 44,1 kHz dans RawAssets/Audio/,
puis importé dans Unreal par nf_assets.py. Utilisable aussi hors d'Unreal :
    python nf_audio.py <dossier_de_sortie>
"""
import math
import os
import random
import struct
import sys
import wave

SAMPLE_RATE = 44100


def _envelope(t, duration, attack=0.005, release=0.08):
    """Enveloppe attaque / relâchement linéaire."""
    if t < attack:
        return t / attack
    if t > duration - release:
        return max(0.0, (duration - t) / release)
    return 1.0


def _bell(freq, t):
    """Timbre de cloche douce : fondamentale + harmoniques amorties."""
    return (math.sin(2 * math.pi * freq * t)
            + 0.45 * math.sin(2 * math.pi * freq * 2.0 * t) * math.exp(-t * 6)
            + 0.20 * math.sin(2 * math.pi * freq * 3.01 * t) * math.exp(-t * 9))


def _render(duration, generator):
    count = int(duration * SAMPLE_RATE)
    return [generator(i / SAMPLE_RATE) for i in range(count)]


def _normalize(samples, peak=0.85):
    top = max(1e-9, max(abs(s) for s in samples))
    return [s * peak / top for s in samples]


def _write(path, samples):
    samples = _normalize(samples)
    with wave.open(path, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples))


# --- Sons du jeu -----------------------------------------------------------

def validation():
    """Son de validation : arpège montant do-mi-sol-do, la dernière note tenue."""
    notes = [(523.25, 0.00), (659.25, 0.11), (783.99, 0.22), (1046.50, 0.33)]
    duration = 1.6

    def gen(t):
        value = 0.0
        for freq, start in notes:
            local = t - start
            if local >= 0:
                decay = 2.2 if freq < 1000 else 1.4
                value += _bell(freq, local) * math.exp(-local * decay)
        return value * _envelope(t, duration, release=0.3)
    return _render(duration, gen)


def click():
    """Clic de bouton mural : impulsion brève et aiguë."""
    duration = 0.06
    rng = random.Random(1)

    def gen(t):
        tone = math.sin(2 * math.pi * 1800 * t) * math.exp(-t * 90)
        noise = (rng.random() * 2 - 1) * math.exp(-t * 200) * 0.4
        return tone + noise
    return _render(duration, gen)


def floor_press():
    """Appui sur le bouton au sol : choc sourd puis bip montant."""
    duration = 0.35

    def gen(t):
        thump = math.sin(2 * math.pi * (90 - 40 * t) * t) * math.exp(-t * 25)
        beep = 0.0
        if t > 0.08:
            local = t - 0.08
            freq = 600 + 1200 * local
            beep = 0.5 * math.sin(2 * math.pi * freq * local) * _envelope(local, duration - 0.08, release=0.05)
        return thump + beep
    return _render(duration, gen)


def training_start():
    """Lancement de l'entraînement : balayage montant électronique."""
    duration = 0.7

    def gen(t):
        # Fréquence 250 Hz * 2^(2,2 t / durée) : la phase est l'intégrale de cette fréquence
        phase = 2 * math.pi * 250 * duration / (2.2 * math.log(2)) * (2 ** (t / duration * 2.2))
        square_ish = math.tanh(3 * math.sin(phase))
        return 0.6 * square_ish * _envelope(t, duration, attack=0.02, release=0.15)
    return _render(duration, gen)


def failure():
    """Échec : deux tons descendants légèrement saturés."""
    duration = 0.8

    def gen(t):
        freq = 330 if t < 0.35 else 247
        local = t if t < 0.35 else t - 0.35
        length = 0.35 if t < 0.35 else 0.45
        tone = math.tanh(2.5 * math.sin(2 * math.pi * freq * local))
        return tone * _envelope(local, length, attack=0.01, release=0.12)
    return _render(duration, gen)


def door_move():
    """Porte coulissante : souffle filtré et ronronnement de moteur."""
    duration = 1.6
    rng = random.Random(7)
    state = {"low": 0.0}

    def gen(t):
        noise = rng.random() * 2 - 1
        state["low"] += 0.04 * (noise - state["low"])	# passe-bas simple
        hum = 0.35 * math.sin(2 * math.pi * 70 * t) + 0.15 * math.sin(2 * math.pi * 140 * t)
        swell = math.sin(math.pi * min(1.0, t / duration))
        return (state["low"] * 3.0 + hum) * swell
    return _render(duration, gen)


def level_complete():
    """Fin de niveau : petite fanfare de trois accords."""
    chords = [((523.25, 659.25, 783.99), 0.0), ((587.33, 739.99, 880.0), 0.28), ((659.25, 830.61, 987.77, 1318.5), 0.56)]
    duration = 2.0

    def gen(t):
        value = 0.0
        for freqs, start in chords:
            local = t - start
            if local >= 0:
                for freq in freqs:
                    value += 0.5 * _bell(freq, local) * math.exp(-local * 1.8)
        return value * _envelope(t, duration, release=0.4)
    return _render(duration, gen)


def greenhouse_ambience():
    """Ambiance de la serre (boucle de 12 s) : brise douce, nappe chaude, quelques gazouillis."""
    duration = 12.0
    count = int(duration * SAMPLE_RATE)
    fade = int(1.0 * SAMPLE_RATE)
    rng = random.Random(2026)

    # Brise : bruit filtré, rendu bouclable par fondu enchaîné entre la fin et le début
    wind, state = [], 0.0
    for _ in range(count + fade):
        state += 0.02 * (rng.uniform(-1.0, 1.0) - state)
        wind.append(state)
    for i in range(fade):
        a = i / fade
        wind[i] = wind[i] * a + wind[count + i] * (1.0 - a)
    wind = wind[:count]

    # Fréquences arrondies pour un nombre entier de périodes sur la boucle
    def looped(freq):
        return round(freq * duration) / duration
    pad = [looped(f) for f in (130.81, 164.81, 196.0, 261.63)]
    chirps = [(1.4, 2600.0), (1.65, 2900.0), (4.3, 3300.0), (7.6, 2500.0), (7.82, 2750.0), (8.05, 3000.0), (10.2, 3500.0)]

    def gen(t):
        i = min(int(t * SAMPLE_RATE), count - 1)
        breath = 0.6 + 0.4 * math.sin(2 * math.pi * t / 6.0)
        value = 3.0 * wind[i] * breath
        for k, freq in enumerate(pad):
            value += 0.05 * math.sin(2 * math.pi * freq * t) * (0.7 + 0.3 * math.sin(2 * math.pi * t / (12.0 / (k + 1))))
        for start, freq in chirps:
            local = t - start
            if 0.0 <= local < 0.12:
                sweep = freq * (1.0 + 0.5 * local / 0.12)
                value += 0.12 * math.sin(2 * math.pi * sweep * local) * math.sin(math.pi * local / 0.12)
        return value
    return _render(duration, gen)


def grow():
    """Pousse des plantes : glissando doux et montant (gamme pentatonique)."""
    notes = [392.0, 440.0, 523.25, 587.33, 659.25, 783.99, 880.0, 1046.5]
    duration = 2.2

    def gen(t):
        value = 0.0
        for k, freq in enumerate(notes):
            local = t - 0.12 * k
            if local >= 0:
                value += 0.35 * _bell(freq, local) * math.exp(-local * 2.5)
        return value * _envelope(t, duration, release=0.5)
    return _render(duration, gen)


SOUNDS = {
    "S_NF_Validation": validation,
    "S_NF_Click": click,
    "S_NF_FloorPress": floor_press,
    "S_NF_TrainingStart": training_start,
    "S_NF_Failure": failure,
    "S_NF_DoorMove": door_move,
    "S_NF_LevelComplete": level_complete,
    "S_NF_Greenhouse": greenhouse_ambience,
    "S_NF_Grow": grow,
}

# Sons joués en boucle
LOOPING = {"S_NF_Greenhouse"}


def generate_all(output_dir):
    """Génère tous les sons ; renvoie la liste des fichiers créés."""
    os.makedirs(output_dir, exist_ok=True)
    paths = []
    for name, generator in SOUNDS.items():
        path = os.path.join(output_dir, name + ".wav")
        _write(path, generator())
        paths.append(path)
    return paths


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    for p in generate_all(out):
        print(p)
