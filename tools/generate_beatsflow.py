#!/usr/bin/env python3
"""
生成鼓手风格段落 - 所有风格都以"动次打次"为骨架
骨架：Kick 1&3拍，Snare 2&4拍，Hi-hat 八分音符
"""
import numpy as np
import wave
import os

SAMPLE_RATE = 44100
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOUNDS_DIR = os.path.join(BASE_DIR, "assets", "sounds")
SONGS_DIR = os.path.join(BASE_DIR, "songs")

def load_wav_mono(path):
    with wave.open(path, 'rb') as wf:
        raw = wf.readframes(wf.getnframes())
        ch = wf.getnchannels()
        sw = wf.getsampwidth()
    if sw == 2:
        samples = np.frombuffer(raw, dtype=np.int16).astype(np.float64) / 32768.0
    else:
        samples = np.frombuffer(raw, dtype=np.int8).astype(np.float64) / 128.0
    if ch > 1:
        samples = samples.reshape(-1, ch).mean(axis=1)
    return samples

def mix_sample(mix, sample, pos, volume=1.0):
    end = min(pos + len(sample), len(mix))
    if end > pos >= 0:
        mix[pos:end] += sample[:end-pos] * volume

def ms_to_samples(ms):
    return int(ms * SAMPLE_RATE / 1000)

def generate(bpm, bars, pattern_func, filename, desc):
    beat = 60000.0 / bpm
    total_ms = beat * 4 * bars + 3000
    total_samples = int(total_ms * SAMPLE_RATE / 1000)
    mix = np.zeros(total_samples)
    pattern_func(mix, bpm, bars)
    max_val = np.max(np.abs(mix))
    if max_val > 0:
        mix = mix / max_val * 0.9
    data = (mix * 32767).astype(np.int16)
    filepath = os.path.join(SONGS_DIR, filename)
    with wave.open(filepath, 'wb') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(data.tobytes())
    print(f"  {desc}")

# 加载鼓采样
kick = load_wav_mono(os.path.join(SOUNDS_DIR, "kick.wav"))
snare = load_wav_mono(os.path.join(SOUNDS_DIR, "snare.wav"))
hihat = load_wav_mono(os.path.join(SOUNDS_DIR, "hihat.wav"))
tom_hi = load_wav_mono(os.path.join(SOUNDS_DIR, "tom_hi.wav"))
tom_mid = load_wav_mono(os.path.join(SOUNDS_DIR, "tom_mid.wav"))
tom_lo = load_wav_mono(os.path.join(SOUNDS_DIR, "tom_lo.wav"))
crash = load_wav_mono(os.path.join(SOUNDS_DIR, "crash.wav"))
ride = load_wav_mono(os.path.join(SOUNDS_DIR, "ride.wav"))

bpm = 120
beat = 60000.0 / bpm

# ========== 骨架函数 ==========
def add_skeleton(mix, t, beat, volume=1.0):
    """添加动次打次骨架：Kick 1&3, Snare 2&4, Hi-hat 八分"""
    # 动(1拍) - Kick
    mix_sample(mix, kick, ms_to_samples(t), 0.9 * volume)
    # 次(1拍后半) - Hi-hat
    mix_sample(mix, hihat, ms_to_samples(t + beat * 0.5), 0.5 * volume)
    # 打(2拍) - Snare
    mix_sample(mix, snare, ms_to_samples(t + beat), 0.85 * volume)
    # 次(2拍后半) - Hi-hat
    mix_sample(mix, hihat, ms_to_samples(t + beat * 1.5), 0.5 * volume)
    # 动(3拍) - Kick
    mix_sample(mix, kick, ms_to_samples(t + beat * 2), 0.9 * volume)
    # 次(3拍后半) - Hi-hat
    mix_sample(mix, hihat, ms_to_samples(t + beat * 2.5), 0.5 * volume)
    # 打(4拍) - Snare
    mix_sample(mix, snare, ms_to_samples(t + beat * 3), 0.85 * volume)
    # 次(4拍后半) - Hi-hat
    mix_sample(mix, hihat, ms_to_samples(t + beat * 3.5), 0.5 * volume)

# ========== 1. Rock Basic (纯骨架) ==========
def rock_basic(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        add_skeleton(mix, t, beat)

# ========== 2. Rock + Crash (骨架+crash开头) ==========
def rock_crash(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        if bar == 0:
            mix_sample(mix, crash, ms_to_samples(t), 0.8)
        add_skeleton(mix, t, beat)

# ========== 3. Rock + 加重底鼓 (骨架+3拍后半加kick) ==========
def rock_heavy(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        add_skeleton(mix, t, beat)
        # 3拍后半加kick
        mix_sample(mix, kick, ms_to_samples(t + beat * 2.5), 0.7)

# ========== 4. Rock + 双底鼓 (骨架+每拍都有kick) ==========
def rock_double_kick(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        add_skeleton(mix, t, beat)
        # 每拍后半加kick
        for i in range(4):
            mix_sample(mix, kick, ms_to_samples(t + beat * (i + 0.5)), 0.6)

# ========== 5. Rock + 开放踩镲 (骨架+开放踩镲) ==========
def rock_open_hh(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        # 动 - Kick
        mix_sample(mix, kick, ms_to_samples(t), 0.9)
        # 开放踩镲
        mix_sample(mix, crash, ms_to_samples(t + beat * 0.5), 0.4)
        # 打 - Snare
        mix_sample(mix, snare, ms_to_samples(t + beat), 0.85)
        # 次 - Hi-hat
        mix_sample(mix, hihat, ms_to_samples(t + beat * 1.5), 0.5)
        # 动 - Kick
        mix_sample(mix, kick, ms_to_samples(t + beat * 2), 0.9)
        # 开放踩镲
        mix_sample(mix, crash, ms_to_samples(t + beat * 2.5), 0.4)
        # 打 - Snare
        mix_sample(mix, snare, ms_to_samples(t + beat * 3), 0.85)
        # 次 - Hi-hat
        mix_sample(mix, hihat, ms_to_samples(t + beat * 3.5), 0.5)

# ========== 6. Rock + 切分 (骨架+切分音) ==========
def rock_syncopation(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        add_skeleton(mix, t, beat)
        # 切分音：1拍后半加kick，3拍后半加snare
        mix_sample(mix, kick, ms_to_samples(t + beat * 0.5), 0.6)
        mix_sample(mix, snare, ms_to_samples(t + beat * 2.5), 0.6)

# ========== 7. Rock + Tom fill (骨架+每4小节最后一小节tom fill) ==========
def rock_tom_fill(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        if bar < bars - 1:
            add_skeleton(mix, t, beat)
        else:
            # 最后一小节：Tom fill
            mix_sample(mix, tom_hi, ms_to_samples(t), 0.9)
            mix_sample(mix, tom_hi, ms_to_samples(t + beat * 0.33), 0.8)
            mix_sample(mix, tom_hi, ms_to_samples(t + beat * 0.66), 0.7)
            mix_sample(mix, tom_mid, ms_to_samples(t + beat), 0.9)
            mix_sample(mix, tom_mid, ms_to_samples(t + beat * 1.33), 0.8)
            mix_sample(mix, tom_mid, ms_to_samples(t + beat * 1.66), 0.7)
            mix_sample(mix, tom_lo, ms_to_samples(t + beat * 2), 0.9)
            mix_sample(mix, tom_lo, ms_to_samples(t + beat * 2.33), 0.8)
            mix_sample(mix, tom_lo, ms_to_samples(t + beat * 2.66), 0.7)
            mix_sample(mix, crash, ms_to_samples(t + beat * 3), 0.8)
            mix_sample(mix, kick, ms_to_samples(t + beat * 3), 0.9)

# ========== 8. Rock + Snare fill (骨架+每4小节snare连击) ==========
def rock_snare_fill(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        if bar < bars - 1:
            add_skeleton(mix, t, beat)
        else:
            # 最后一小节：Snare连击
            for i in range(8):
                vol = 0.9 - i * 0.05
                mix_sample(mix, snare, ms_to_samples(t + beat * i * 0.5), vol)
            mix_sample(mix, crash, ms_to_samples(t + beat * 3.5), 0.8)
            mix_sample(mix, kick, ms_to_samples(t + beat * 3.5), 0.9)

# ========== 9. Funk (骨架+十六分hihat) ==========
def funk(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        # 骨架
        mix_sample(mix, kick, ms_to_samples(t), 0.9)
        mix_sample(mix, snare, ms_to_samples(t + beat), 0.85)
        mix_sample(mix, kick, ms_to_samples(t + beat * 2), 0.9)
        mix_sample(mix, snare, ms_to_samples(t + beat * 3), 0.85)
        # 十六分hihat
        for i in range(16):
            vol = 0.5 if i % 2 == 0 else 0.3
            mix_sample(mix, hihat, ms_to_samples(t + beat * i * 0.25), vol)
        # 切分kick
        mix_sample(mix, kick, ms_to_samples(t + beat * 1.5), 0.6)
        mix_sample(mix, kick, ms_to_samples(t + beat * 3.5), 0.6)

# ========== 10. Shuffle (骨架+摇摆hihat) ==========
def shuffle(mix, bpm, bars):
    beat = 60000.0 / bpm
    shuffle_off = beat * 0.33
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        # 骨架
        mix_sample(mix, kick, ms_to_samples(t), 0.9)
        mix_sample(mix, snare, ms_to_samples(t + beat), 0.85)
        mix_sample(mix, kick, ms_to_samples(t + beat * 2), 0.9)
        mix_sample(mix, snare, ms_to_samples(t + beat * 3), 0.85)
        # 摇摆hihat
        for i in range(4):
            mix_sample(mix, hihat, ms_to_samples(t + beat * i), 0.6)
            mix_sample(mix, hihat, ms_to_samples(t + beat * i + shuffle_off), 0.4)

# ========== 11. Half Time (骨架简化版) ==========
def half_time(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        # 动 - Kick (只在1拍)
        mix_sample(mix, kick, ms_to_samples(t), 0.9)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 0.5), 0.5)
        # 次 - Hi-hat
        mix_sample(mix, hihat, ms_to_samples(t + beat), 0.5)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 1.5), 0.5)
        # 打 - Snare (只在3拍)
        mix_sample(mix, snare, ms_to_samples(t + beat * 2), 0.85)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 2.5), 0.5)
        # 次 - Hi-hat
        mix_sample(mix, hihat, ms_to_samples(t + beat * 3), 0.5)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 3.5), 0.5)

# ========== 12. Jazz (骨架变体+ride) ==========
def jazz(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        # 骨架用ride代替hihat
        mix_sample(mix, kick, ms_to_samples(t), 0.7)
        mix_sample(mix, ride, ms_to_samples(t), 0.5)
        mix_sample(mix, ride, ms_to_samples(t + beat * 0.66), 0.3)
        mix_sample(mix, snare, ms_to_samples(t + beat), 0.4)
        mix_sample(mix, ride, ms_to_samples(t + beat), 0.5)
        mix_sample(mix, ride, ms_to_samples(t + beat * 1.66), 0.3)
        mix_sample(mix, kick, ms_to_samples(t + beat * 2), 0.7)
        mix_sample(mix, ride, ms_to_samples(t + beat * 2), 0.5)
        mix_sample(mix, ride, ms_to_samples(t + beat * 2.66), 0.3)
        mix_sample(mix, snare, ms_to_samples(t + beat * 3), 0.4)
        mix_sample(mix, ride, ms_to_samples(t + beat * 3), 0.5)
        mix_sample(mix, ride, ms_to_samples(t + beat * 3.66), 0.3)

# ========== 13. Metal (骨架+双踩) ==========
def metal(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        # 骨架
        mix_sample(mix, kick, ms_to_samples(t), 0.9)
        mix_sample(mix, snare, ms_to_samples(t + beat), 0.85)
        mix_sample(mix, kick, ms_to_samples(t + beat * 2), 0.9)
        mix_sample(mix, snare, ms_to_samples(t + beat * 3), 0.85)
        # 双踩：每拍后半加kick
        for i in range(4):
            mix_sample(mix, kick, ms_to_samples(t + beat * (i + 0.5)), 0.7)
        # 十六分hihat
        for i in range(16):
            mix_sample(mix, hihat, ms_to_samples(t + beat * i * 0.25), 0.3)

# ========== 14. Latin (骨架+切分) ==========
def latin(mix, bpm, bars):
    beat = 60000.0 / bpm
    for bar in range(bars):
        t = 2000 + bar * beat * 4
        # 骨架
        mix_sample(mix, kick, ms_to_samples(t), 0.9)
        mix_sample(mix, snare, ms_to_samples(t + beat), 0.85)
        mix_sample(mix, kick, ms_to_samples(t + beat * 2), 0.9)
        mix_sample(mix, snare, ms_to_samples(t + beat * 3), 0.85)
        # 切分hihat
        mix_sample(mix, hihat, ms_to_samples(t), 0.5)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 0.5), 0.3)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 1.5), 0.5)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 2), 0.3)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 3), 0.5)
        mix_sample(mix, hihat, ms_to_samples(t + beat * 3.5), 0.3)

# ========== 生成 ==========
print("🥁 正在生成鼓手风格段落（动次打次骨架版）...")
print()

print("🔥 骨架基础版:")
generate(bpm, 8, rock_basic, "beatsflow_01_rock_basic.wav", "1. Rock Basic (纯骨架)")
generate(bpm, 8, rock_crash, "beatsflow_02_rock_crash.wav", "2. Rock + Crash (骨架+crash)")
generate(bpm, 8, rock_heavy, "beatsflow_03_rock_heavy.wav", "3. Rock Heavy (加重底鼓)")
generate(bpm, 8, rock_double_kick, "beatsflow_04_rock_dblkick.wav", "4. Rock + 双底鼓")

print()
print("🎵 骨架变化版:")
generate(bpm, 8, rock_open_hh, "beatsflow_05_rock_openhh.wav", "5. Rock + 开放踩镲")
generate(bpm, 8, rock_syncopation, "beatsflow_06_rock_sync.wav", "6. Rock + 切分音")
generate(bpm, 8, funk, "beatsflow_07_funk.wav", "7. Funk (十六分hihat)")
generate(bpm, 8, shuffle, "beatsflow_08_shuffle.wav", "8. Shuffle (摇摆)")
generate(bpm, 8, half_time, "beatsflow_09_halftime.wav", "9. Half Time (骨架简化)")
generate(bpm, 8, jazz, "beatsflow_10_jazz.wav", "10. Jazz (ride变体)")
generate(bpm, 8, metal, "beatsflow_11_metal.wav", "11. Metal (双踩)")
generate(bpm, 8, latin, "beatsflow_12_latin.wav", "12. Latin (切分)")

print()
print("🔥 Tom Fill 版:")
generate(bpm, 8, rock_tom_fill, "beatsflow_13_rock_tomfill.wav", "13. Rock + Tom fill")
generate(bpm, 8, rock_snare_fill, "beatsflow_14_rock_snarefill.wav", "14. Rock + Snare fill")

print()
print("🎵 所有段落已生成到 songs/ 目录")
print("   全部基于「动次打次」骨架：Kick 1&3, Snare 2&4, Hi-hat 八分")
