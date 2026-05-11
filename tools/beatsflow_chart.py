#!/usr/bin/env python3
"""
专业架子鼓模板谱面生成器
基于《架子鼓编写模板大全》 - 国风古风为主
"""
import numpy as np
import wave
import os
import subprocess
import json

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

def load_mp3_as_pcm(mp3_path, sr=44100):
    cmd = ['ffmpeg', '-i', mp3_path, '-f', 's16le', '-acodec', 'pcm_s16le',
           '-ar', str(sr), '-ac', '1', '-']
    result = subprocess.run(cmd, capture_output=True)
    if result.returncode != 0:
        return np.array([]), sr
    return np.frombuffer(result.stdout, dtype=np.int16).astype(np.float64) / 32768.0, sr

def mix_sample(mix, sample, pos, volume=1.0):
    end = min(pos + len(sample), len(mix))
    if end > pos >= 0:
        mix[pos:end] += sample[:end-pos] * volume

def ms_to_samples(ms, sr=44100):
    return int(ms * sr / 1000)

# ========== 鼓点函数

def pattern_chinese(mix, t, beat, sr, vol):
    """国风古风：拍1, 拍2后半, 拍3, 拍4后半"""
    # KD: 拍1, 拍2后半, 拍3, 拍4后半
    mix_sample(mix, kick, ms_to_samples(t, sr), 0.95 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 1.5, sr), 0.7 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 2, sr), 0.95 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 3.5, sr), 0.7 * vol)
    # SD: 拍2, 拍4
    mix_sample(mix, snare, ms_to_samples(t + beat, sr), 0.9 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat * 3, sr), 0.9 * vol)
    # 16分踩镲
    for i in range(16):
        v = 0.5 if i % 4 == 0 else (0.35 if i % 2 == 0 else 0.2)
        mix_sample(mix, hihat, ms_to_samples(t + beat * i * 0.25, sr), v * vol)

def pattern_jpop(mix, t, beat, sr, vol):
    """J-Pop甜系：活泼16分小切分"""
    mix_sample(mix, kick, ms_to_samples(t, sr), 0.95 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 0.75, sr), 0.7 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 1.25, sr), 0.65 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 2, sr), 0.95 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 2.75, sr), 0.7 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 3.25, sr), 0.65 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat, sr), 0.9 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat * 3, sr), 0.9 * vol)
    for i in range(16):
        v = 0.6 if i % 4 == 0 else (0.4 if i % 2 == 0 else 0.25)
        mix_sample(mix, hihat, ms_to_samples(t + beat * i * 0.25, sr), v * vol)

def pattern_trap(mix, t, beat, sr, vol):
    """Trap嘻哈：反拍军鼓"""
    mix_sample(mix, kick, ms_to_samples(t, sr), 0.95 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 1.5, sr), 0.8 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 2.5, sr), 0.8 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 3.75, sr), 0.7 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat * 1.5, sr), 0.9 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat * 3, sr), 0.9 * vol)
    for i in range(16):
        if i % 4 != 3:
            v = 0.5 if i % 4 == 0 else 0.3
            mix_sample(mix, hihat, ms_to_samples(t + beat * i * 0.25, sr), v * vol)

def pattern_punk(mix, t, beat, sr, vol):
    """朋克摇滚：双踩 + 16分踩镲打满"""
    for i in range(4):
        mix_sample(mix, kick, ms_to_samples(t + beat * i, sr), 0.9 * vol)
        mix_sample(mix, kick, ms_to_samples(t + beat * (i + 0.5), sr), 0.75 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat, sr), 0.9 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat * 3, sr), 0.9 * vol)
    for i in range(16):
        v = 0.6 if i % 4 == 0 else 0.4
        mix_sample(mix, hihat, ms_to_samples(t + beat * i * 0.25, sr), v * vol)

def pattern_pop_basic(mix, t, beat, sr, vol):
    """通用流行基础"""
    mix_sample(mix, kick, ms_to_samples(t, sr), 0.95 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 2, sr), 0.95 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat, sr), 0.9 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat * 3, sr), 0.9 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 1.5, sr), 0.7 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 3.5, sr), 0.7 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat * 0.5, sr), 0.25 * vol)
    mix_sample(mix, snare, ms_to_samples(t + beat * 2.5, sr), 0.25 * vol)
    for i in range(16):
        v = 0.6 if i % 4 == 0 else (0.4 if i % 2 == 0 else 0.25)
        mix_sample(mix, hihat, ms_to_samples(t + beat * i * 0.25, sr), v * vol)

def pattern_tom_fill(mix, t, beat, sr, vol):
    """Tom fill高→中→低"""
    mix_sample(mix, tom_hi, ms_to_samples(t, sr), 0.9 * vol)
    mix_sample(mix, tom_hi, ms_to_samples(t + beat * 0.33, sr), 0.8 * vol)
    mix_sample(mix, tom_hi, ms_to_samples(t + beat * 0.66, sr), 0.7 * vol)
    mix_sample(mix, tom_mid, ms_to_samples(t + beat, sr), 0.9 * vol)
    mix_sample(mix, tom_mid, ms_to_samples(t + beat * 1.33, sr), 0.8 * vol)
    mix_sample(mix, tom_mid, ms_to_samples(t + beat * 1.66, sr), 0.7 * vol)
    mix_sample(mix, tom_lo, ms_to_samples(t + beat * 2, sr), 0.9 * vol)
    mix_sample(mix, tom_lo, ms_to_samples(t + beat * 2.33, sr), 0.8 * vol)
    mix_sample(mix, tom_lo, ms_to_samples(t + beat * 2.66, sr), 0.7 * vol)
    mix_sample(mix, crash, ms_to_samples(t + beat * 3, sr), 0.8 * vol)
    mix_sample(mix, kick, ms_to_samples(t + beat * 3, sr), 0.9 * vol)

def pattern_crash_in(mix, t, beat, sr, vol):
    """Crash开头 + 加重版"""
    mix_sample(mix, crash, ms_to_samples(t, sr), 0.85 * vol)
    pattern_punk(mix, t, beat, sr, vol)

# ========== 主函数
def generate_chart(mp3_path):
    print(f"🎵 正在分析: {mp3_path}")
    
    audio, sr = load_mp3_as_pcm(mp3_path)
    if len(audio) == 0:
        print("❌ 无法读取MP3")
        return
    duration = len(audio) / sr
    print(f"   时长: {duration:.1f}秒")
    
    beat_detect_path = os.path.join(BASE_DIR, 'tools', 'beat_detect.py')
    cmd = ['python3', beat_detect_path, mp3_path]
    result = subprocess.run(cmd, capture_output=True, text=True)
    try:
        data = json.loads(result.stdout)
        bpm = data.get('bpm', 120)
    except:
        bpm = 120
    beat_ms = 60000.0 / bpm
    bar_ms = beat_ms * 4
    print(f"   BPM: {bpm}")
    
    print("\n📊 分析音频能量...")
    window_samples = int(0.5 * sr)
    energies = []
    for i in range(0, len(audio), window_samples):
        chunk = audio[i:i+window_samples]
        if len(chunk) > 0:
            energies.append(np.sqrt(np.mean(chunk ** 2)))
    energies = np.array(energies)
    max_e = np.max(energies)
    if max_e > 0:
        energies = energies / max_e
    
    total_bars = int(duration * 1000 / bar_ms)
    total_segments = total_bars // 4
    print(f"   总段落: {total_segments}")
    
    print("\n🥁 段落分配:")
    patterns = []
    for seg in range(total_segments):
        seg_dur = 4 * beat_ms / 1000
        start_w = int(seg * seg_dur / 0.5)
        end_w = int((seg + 1) * seg_dur / 0.5)
        start_w = max(0, min(start_w, len(energies) - 1))
        end_w = max(start_w + 1, min(end_w, len(energies)))
        energy = np.mean(energies[start_w:end_w])
        
        if seg == 0:
            p = "crash_in"
        elif seg == total_segments - 1:
            p = "tom_fill"
        elif energy > 0.7:
            p = "punk"
        elif energy > 0.55:
            p = "trap"
        elif energy > 0.4:
            p = "jpop"
        elif energy > 0.25:
            p = "chinese"
        else:
            p = "pop"
        
        patterns.append(p)
        
        if seg < 5 or seg >= total_segments - 3 or seg % 5 == 0:
            print(f"   段落 {seg+1:2d}: 能量{energy:.3f} → {p}")
        elif seg == 5:
            print(f"   ...")
    
    print("\n🎵 生成鼓点...")
    mix = audio.copy()
    drum_vol = 0.6
    
    pattern_funcs = {
        "pop": pattern_pop_basic,
        "jpop": pattern_jpop,
        "chinese": pattern_chinese,
        "trap": pattern_trap,
        "punk": pattern_punk,
    }
    
    for seg in range(total_segments):
        t_ms = 2000 + seg * 4 * bar_ms
        p = patterns[seg]
        
        for bar in range(4):
            t = t_ms + bar * bar_ms
            if p == "crash_in":
                pattern_crash_in(mix, t, beat_ms, sr, drum_vol)
            elif p == "tom_fill":
                if bar < 3:
                    pattern_chinese(mix, t, beat_ms, sr, drum_vol)
                else:
                    pattern_tom_fill(mix, t, beat_ms, sr, drum_vol)
            else:
                pattern_funcs[p](mix, t, beat_ms, sr, drum_vol)
    
    max_val = np.max(np.abs(mix))
    if max_val > 0:
        mix = mix / max_val * 0.9
    
    output_path = os.path.join(SONGS_DIR, "天下_music_beatsflow.wav")
    data = (mix * 32767).astype(np.int16)
    with wave.open(output_path, 'wb') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sr)
        wf.writeframes(data.tobytes())
    
    print(f"\n✅ 已生成: {output_path}")
    print(f"   文件大小: {os.path.getsize(output_path) / 1024 / 1024:.1f}MB")
    print(f"   风格: 国风古风为主 + 高能量J-Pop/Trap")

# 加载鼓采样
print("🥁 加载鼓采样...")
kick = load_wav_mono(os.path.join(SOUNDS_DIR, "kick.wav"))
snare = load_wav_mono(os.path.join(SOUNDS_DIR, "snare.wav"))
hihat = load_wav_mono(os.path.join(SOUNDS_DIR, "hihat.wav"))
tom_hi = load_wav_mono(os.path.join(SOUNDS_DIR, "tom_hi.wav"))
tom_mid = load_wav_mono(os.path.join(SOUNDS_DIR, "tom_mid.wav"))
tom_lo = load_wav_mono(os.path.join(SOUNDS_DIR, "tom_lo.wav"))
crash = load_wav_mono(os.path.join(SOUNDS_DIR, "crash.wav"))
ride = load_wav_mono(os.path.join(SOUNDS_DIR, "ride.wav"))

mp3_path = os.path.join(SONGS_DIR, "天下 music.mp3")
if os.path.exists(mp3_path):
    generate_chart(mp3_path)
else:
    print(f"❌ 找不到文件: {mp3_path}")
