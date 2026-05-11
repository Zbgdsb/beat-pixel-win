#!/usr/bin/env python3
"""
生成一个整合所有鼓手风格的完整展示WAV
每个风格播放8小节，自动过渡到下一个
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
    length = end - pos
    if length > 0 and pos >= 0:
        mix[pos:end] += sample[:length] * volume

def ms_to_samples(ms):
    return int(ms * SAMPLE_RATE / 1000)

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
bar_ms = beat * 4
bars_per_style = 8

# 定义所有风格
styles = [
    ("Rock Basic (动次打次)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.5),
        (0.5, hihat, 0.4),
        (1, snare, 0.85), (1, hihat, 0.5),
        (1.5, hihat, 0.4),
        (2, kick, 0.9), (2, hihat, 0.5),
        (2.5, hihat, 0.4),
        (3, snare, 0.85), (3, hihat, 0.5),
        (3.5, hihat, 0.4),
    ]),
    ("Rock Heavy (强力摇滚)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.5),
        (0.5, hihat, 0.4),
        (1, snare, 0.85), (1, hihat, 0.5),
        (1.5, hihat, 0.4),
        (2, kick, 0.9), (2, hihat, 0.5),
        (2.5, kick, 0.8), (2.5, hihat, 0.4),
        (3, snare, 0.85), (3, hihat, 0.5),
        (3.5, hihat, 0.4),
    ]),
    ("Pop Basic (流行基本)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.5),
        (0.5, hihat, 0.4),
        (1, snare, 0.85), (1, hihat, 0.5),
        (1.5, hihat, 0.4),
        (2, kick, 0.9), (2, hihat, 0.5),
        (2.5, hihat, 0.4),
        (3, snare, 0.85), (3, hihat, 0.5),
        (3.5, kick, 0.7), (3.5, hihat, 0.4),
    ]),
    ("Funk Basic (放克基本)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.5),
        (0.25, hihat, 0.3), (0.5, hihat, 0.4), (0.75, hihat, 0.3),
        (1, snare, 0.85), (1, hihat, 0.5),
        (1.25, hihat, 0.3), (1.5, kick, 0.7), (1.5, hihat, 0.4), (1.75, hihat, 0.3),
        (2, kick, 0.9), (2, hihat, 0.5),
        (2.25, hihat, 0.3), (2.5, hihat, 0.4), (2.75, hihat, 0.3),
        (3, snare, 0.85), (3, hihat, 0.5),
        (3.25, hihat, 0.3), (3.5, hihat, 0.4), (3.75, hihat, 0.3),
    ]),
    ("Hip-Hop Basic (嘻哈基本)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.5),
        (0.5, hihat, 0.4),
        (1, snare, 0.85),
        (1.5, kick, 0.7), (1.5, hihat, 0.4),
        (2, hihat, 0.5),
        (2.5, hihat, 0.4),
        (3, snare, 0.85),
        (3.5, hihat, 0.4),
    ]),
    ("Ballad Basic (抒情基本)", lambda mix, t: [
        (0, kick, 0.8), (0, ride, 0.4),
        (1, ride, 0.3),
        (2, snare, 0.7), (2, ride, 0.4),
        (3, ride, 0.3),
    ]),
    ("Shuffle (摇摆节奏)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.5),
        (0.66, hihat, 0.3),
        (1, snare, 0.85), (1, hihat, 0.5),
        (1.66, hihat, 0.3),
        (2, kick, 0.9), (2, hihat, 0.5),
        (2.66, hihat, 0.3),
        (3, snare, 0.85), (3, hihat, 0.5),
        (3.66, hihat, 0.3),
    ]),
    ("Half Time (半拍节奏)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.5),
        (1, hihat, 0.4),
        (2, snare, 0.85), (2, hihat, 0.5),
        (3, hihat, 0.4),
    ]),
    ("Jazz Ride (爵士)", lambda mix, t: [
        (0, kick, 0.7), (0, ride, 0.5),
        (0.66, ride, 0.3),
        (1, snare, 0.4), (1, ride, 0.5),
        (1.66, ride, 0.3),
        (2, kick, 0.7), (2, ride, 0.5),
        (2.66, ride, 0.3),
        (3, snare, 0.4), (3, ride, 0.5),
        (3.66, ride, 0.3),
    ]),
    ("Latin (拉丁切分)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.5),
        (0.5, hihat, 0.3),
        (1, snare, 0.85),
        (1.5, kick, 0.7), (1.5, hihat, 0.5),
        (2, hihat, 0.3),
        (2.5, snare, 0.6),
        (3, hihat, 0.5),
        (3.5, hihat, 0.3),
    ]),
    ("Reggae (雷鬼)", lambda mix, t: [
        (0, kick, 0.9),
        (0.5, hihat, 0.4),
        (1.5, snare, 0.85), (1.5, hihat, 0.4),
        (2, kick, 0.9),
        (2.5, hihat, 0.4),
        (3.5, snare, 0.85), (3.5, hihat, 0.4),
    ]),
    ("Metal Double Bass (金属双踩)", lambda mix, t: [
        (0, kick, 0.9), (0, hihat, 0.4),
        (0.25, kick, 0.6), (0.5, kick, 0.8), (0.5, hihat, 0.3), (0.75, kick, 0.6),
        (1, snare, 0.85), (1, hihat, 0.4),
        (1.25, kick, 0.6), (1.5, kick, 0.8), (1.5, hihat, 0.3), (1.75, kick, 0.6),
        (2, kick, 0.9), (2, hihat, 0.4),
        (2.25, kick, 0.6), (2.5, kick, 0.8), (2.5, hihat, 0.3), (2.75, kick, 0.6),
        (3, snare, 0.85), (3, hihat, 0.4),
        (3.25, kick, 0.6), (3.5, kick, 0.8), (3.5, hihat, 0.3), (3.75, kick, 0.6),
    ]),
]

# Tom fill 系列（每个风格的最后一小节用不同的fill）
tom_fills = [
    # 高→中→低连击
    lambda mix, t: [
        (0, tom_hi, 0.9), (0.33, tom_hi, 0.8), (0.66, tom_hi, 0.7),
        (1, tom_mid, 0.9), (1.33, tom_mid, 0.8), (1.66, tom_mid, 0.7),
        (2, tom_lo, 0.9), (2.33, tom_lo, 0.8), (2.66, tom_lo, 0.7),
        (3, crash, 0.8), (3, kick, 0.9),
    ],
    # 低→中→高连击
    lambda mix, t: [
        (0, tom_lo, 0.9), (0.33, tom_lo, 0.8), (0.66, tom_lo, 0.7),
        (1, tom_mid, 0.9), (1.33, tom_mid, 0.8), (1.66, tom_mid, 0.7),
        (2, tom_hi, 0.9), (2.33, tom_hi, 0.8), (2.66, tom_hi, 0.7),
        (3, crash, 0.8), (3, kick, 0.9),
    ],
    # 瀑布式
    lambda mix, t: [
        (0, tom_hi, 0.9), (0.25, tom_hi, 0.8), (0.5, tom_hi, 0.7),
        (0.75, tom_mid, 0.9), (1, tom_mid, 0.8), (1.25, tom_mid, 0.7),
        (1.5, tom_lo, 0.9), (1.75, tom_lo, 0.8), (2, tom_lo, 0.7),
        (2.25, tom_mid, 0.9), (2.5, tom_mid, 0.8),
        (2.75, tom_hi, 0.9), (3, tom_hi, 0.8),
        (3.5, crash, 0.8), (3.5, kick, 0.9),
    ],
    # 单击交替
    lambda mix, t: [
        (0, tom_hi, 0.9), (0.25, tom_mid, 0.8), (0.5, tom_lo, 0.9), (0.75, tom_hi, 0.8),
        (1, tom_mid, 0.9), (1.25, tom_lo, 0.8), (1.5, tom_hi, 0.9), (1.75, tom_mid, 0.8),
        (2, tom_lo, 0.9), (2.25, tom_hi, 0.8), (2.5, tom_mid, 0.9), (2.75, tom_lo, 0.8),
        (3, crash, 0.8), (3, kick, 0.9),
    ],
    # 双击
    lambda mix, t: [
        (0, tom_hi, 0.9), (0.25, tom_hi, 0.8), (0.5, tom_hi, 0.9), (0.75, tom_hi, 0.8),
        (1, tom_mid, 0.9), (1.25, tom_mid, 0.8), (1.5, tom_mid, 0.9), (1.75, tom_mid, 0.8),
        (2, tom_lo, 0.9), (2.25, tom_lo, 0.8), (2.5, tom_lo, 0.9), (2.75, tom_lo, 0.8),
        (3, crash, 0.8), (3, kick, 0.9),
    ],
    # Tom+Snare交替
    lambda mix, t: [
        (0, tom_hi, 0.9), (0.25, snare, 0.7), (0.5, tom_mid, 0.9), (0.75, snare, 0.7),
        (1, tom_lo, 0.9), (1.25, snare, 0.7), (1.5, tom_mid, 0.9), (1.75, snare, 0.7),
        (2, tom_hi, 0.9), (2.25, snare, 0.7), (2.5, tom_lo, 0.9), (2.75, snare, 0.7),
        (3, crash, 0.8), (3, kick, 0.9),
    ],
]

# 计算总时长
total_styles = len(styles)
total_bars = total_styles * bars_per_style
total_ms = bar_ms * total_bars + 5000  # 多5秒余量
total_samples = int(total_ms * SAMPLE_RATE / 1000)
mix = np.zeros(total_samples)

print("🥁 正在生成完整展示...")
print()

# 生成每个风格
for style_idx, (name, pattern_func) in enumerate(styles):
    print(f"  [{style_idx+1}/{total_styles}] {name}")
    
    for bar in range(bars_per_style):
        t_ms = 2000 + (style_idx * bars_per_style + bar) * bar_ms
        t_beat = t_ms  # 每拍的起始时间
        
        if bar < bars_per_style - 1:
            # 普通小节：使用风格pattern
            for beat_pos, drum, vol in pattern_func(mix, t_beat):
                mix_sample(mix, drum, ms_to_samples(t_beat + beat * beat_pos), vol)
        else:
            # 最后一小节：Tom fill
            fill_idx = style_idx % len(tom_fills)
            for beat_pos, drum, vol in tom_fills[fill_idx](mix, t_beat):
                mix_sample(mix, drum, ms_to_samples(t_beat + beat * beat_pos), vol)

# 归一化
max_val = np.max(np.abs(mix))
if max_val > 0:
    mix = mix / max_val * 0.9

# 转int16
data = (mix * 32767).astype(np.int16)

# 写WAV
filepath = os.path.join(SONGS_DIR, "beatsflow_FULL_SHOW.wav")
with wave.open(filepath, 'wb') as wf:
    wf.setnchannels(1)
    wf.setsampwidth(2)
    wf.setframerate(SAMPLE_RATE)
    wf.writeframes(data.tobytes())

duration = total_ms / 1000
print()
print(f"✅ 完整展示已生成: {filepath}")
print(f"   时长: {duration:.0f}秒 ({duration/60:.1f}分钟)")
print(f"   包含 {total_styles} 种风格，每种 {bars_per_style} 小节")
print(f"   每个风格最后一小节是不同的 Tom fill")
