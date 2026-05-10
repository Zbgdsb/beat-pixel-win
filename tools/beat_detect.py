#!/usr/bin/env python3
"""
Beat Detection Script - 多后端自动切换
优先级: librosa > aubio > numpy
用法: python3 beat_detect.py <audio_file>
输出: JSON格式的节拍检测结果
"""
import sys
import json
import subprocess
import tempfile
import os
import numpy as np

def decode_with_ffmpeg(audio_path, sr=22050):
    """用ffmpeg解码音频为WAV"""
    tmp = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)
    tmp_path = tmp.name
    tmp.close()
    cmd = ['ffmpeg', '-y', '-i', audio_path, '-ar', str(sr), '-ac', '1', '-acodec', 'pcm_s16le', tmp_path]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return tmp_path

def try_load_audio(audio_path, sr=22050):
    """加载音频，优先直接读，失败用ffmpeg转"""
    try:
        import librosa
        y, _ = librosa.load(audio_path, sr=sr, mono=True)
        return y, sr
    except Exception:
        pass
    tmp = decode_with_ffmpeg(audio_path, sr)
    try:
        import librosa
        y, _ = librosa.load(tmp, sr=sr, mono=True)
        return y, sr
    finally:
        os.unlink(tmp)

# ========== 方案1: librosa ==========
def detect_librosa(audio_path):
    import librosa
    sr = 22050
    y, sr = try_load_audio(audio_path, sr)
    duration = len(y) / sr

    onset_env = librosa.onset.onset_strength(y=y, sr=sr, hop_length=512)
    tempo, beat_frames = librosa.beat.beat_track(y=y, sr=sr, hop_length=512, onset_envelope=onset_env, tightness=100)
    bpm = float(tempo) if not hasattr(tempo, '__len__') else float(tempo[0])

    beat_times = librosa.frames_to_time(beat_frames, sr=sr, hop_length=512) * 1000.0
    onset_frames = librosa.onset.onset_detect(y=y, sr=sr, hop_length=512, onset_envelope=onset_env, backtrack=True, units='frames')
    onset_times = librosa.frames_to_time(onset_frames, sr=sr, hop_length=512) * 1000.0

    if len(beat_times) > 2:
        intervals = np.diff(beat_times)
        cv = np.std(intervals) / (np.median(intervals) + 1e-10)
        confidence = max(0, min(100, 100 - cv * 200))
    else:
        confidence = 50.0

    onset_strength_at_beats = []
    for t in beat_times:
        frame = int(t * sr / (1000 * 512))
        onset_strength_at_beats.append(onset_env[frame] if frame < len(onset_env) else 0)

    if onset_strength_at_beats:
        strengths = np.array(onset_strength_at_beats)
        accent_threshold = np.mean(strengths) + 0.8 * np.std(strengths)
        accent_set = set(round(beat_times[i]) for i in range(len(beat_times)) if strengths[i] > accent_threshold)
    else:
        accent_set = set()

    all_times = sorted(set([round(t) for t in beat_times] + [round(t) for t in onset_times if t > 1000]))
    filtered = []
    for t in all_times:
        if t < 1000: continue
        if filtered and (t - filtered[-1]) < 150: continue
        filtered.append(t)

    return {
        "bpm": round(bpm, 1), "confidence": round(confidence, 1), "duration": round(duration, 2),
        "beats": [{"timeMs": int(t), "isAccent": bool(t in accent_set)} for t in filtered]
    }

# ========== 方案2: aubio ==========
def detect_aubio(audio_path):
    import aubio
    sr = 22050
    hop_s = 512

    tmp_path = None
    try:
        src = aubio.source(audio_path, sr, hop_s)
    except Exception:
        tmp_path = decode_with_ffmpeg(audio_path, sr)
        src = aubio.source(tmp_path, sr, hop_s)

    duration = src.duration / sr
    tempo_o = aubio.tempo("default", 1024, hop_s, sr)
    onset_o = aubio.onset("default", 1024, hop_s, sr)

    onset_times = []
    onset_energies = []
    while True:
        samples, read = src()
        if onset_o(samples):
            onset_times.append(onset_o.get_last_ms())
            onset_energies.append(float(onset_o.get_descriptor()))
        tempo_o(samples)
        if read < hop_s: break

    if tmp_path: os.unlink(tmp_path)

    bpm = float(tempo_o.get_bpm())
    if len(onset_times) > 10:
        intervals = np.diff(onset_times)
        ratio = np.median(intervals) / (60000.0 / bpm + 1e-10)
        confidence = 95.0 if (0.8 < ratio < 1.2 or 0.4 < ratio < 0.6) else 70.0
    else:
        confidence = 50.0

    if onset_energies:
        e = np.array(onset_energies)
        accent_thresh = np.mean(e) + 1.0 * np.std(e)
    else:
        accent_thresh = 0

    filtered = []
    for i, t in enumerate(onset_times):
        if t < 1000: continue
        if filtered and (t - filtered[-1]['timeMs']) < 150:
            if onset_energies[i] > filtered[-1]['energy']:
                filtered[-1] = {'timeMs': round(t), 'energy': onset_energies[i], 'isAccent': onset_energies[i] > accent_thresh}
            continue
        filtered.append({'timeMs': round(t), 'energy': onset_energies[i], 'isAccent': onset_energies[i] > accent_thresh})

    return {
        "bpm": round(bpm, 1), "confidence": round(confidence, 1), "duration": round(duration, 2),
        "beats": [{"timeMs": int(item['timeMs']), "isAccent": bool(item['isAccent'])} for item in filtered]
    }

# ========== 方案3: numpy (兜底) ==========
def detect_numpy(audio_path):
    sr = 22050
    tmp = decode_with_ffmpeg(audio_path, sr)
    with open(tmp, 'rb') as f:
        data = f.read()
    os.unlink(tmp)
    if len(data) < 2: raise ValueError("ffmpeg解码失败")
    pcm = np.frombuffer(data, dtype=np.int16).astype(np.float64) / 32768.0
    duration = len(pcm) / sr

    win, hop = 1024, 512
    nframes = (len(pcm) - win) // hop
    ste = np.zeros(nframes)
    for i in range(nframes):
        s = pcm[i*hop:i*hop+win]
        ste[i] = np.sum(s*s) / win

    flux = np.zeros(nframes)
    flux[1:] = np.maximum(0, ste[1:] - ste[:-1])

    lwin = 16
    lmean = np.array([np.mean(flux[max(0,i-lwin):i+1]) for i in range(nframes)])
    lstd = np.array([np.std(flux[max(0,i-lwin):i+1]) for i in range(nframes)])
    gmean, gstd = np.mean(flux), np.std(flux)
    thresh = np.maximum(lmean + 0.5*lstd, gmean + 0.2*gstd)

    onsets = []
    for i in range(2, nframes-1):
        if flux[i] > thresh[i] and flux[i] > flux[i-1] and flux[i] >= flux[i+1]:
            t = i * hop * 1000.0 / sr
            if t < 1000: continue
            if onsets and (t - onsets[-1]) < 150: continue
            onsets.append(t)

    # BPM via autocorrelation
    maxf = min(len(flux), int(30*sr/hop))
    sig = flux[:maxf] - np.mean(flux[:maxf])
    ac = np.correlate(sig, sig, 'full')[len(sig)-1:]
    minlag = int(60*sr/(hop*200))
    maxlag = min(int(60*sr/(hop*60)), len(ac)-1)
    if maxlag > minlag:
        peak = np.argmax(ac[minlag:maxlag]) + minlag
        bpm = 60.0*sr/(hop*peak)
    else:
        bpm = 120.0

    return {
        "bpm": round(bpm, 1), "confidence": 60.0, "duration": round(duration, 2),
        "beats": [{"timeMs": int(t), "isAccent": False} for t in onsets]
    }

def main():
    if len(sys.argv) < 2:
        print(json.dumps({"error": "usage: beat_detect.py <audio_file>"}))
        sys.exit(1)

    audio_path = sys.argv[1]

    # 按优先级尝试三种方案
    for name, func in [("librosa", detect_librosa), ("aubio", detect_aubio), ("numpy", detect_numpy)]:
        try:
            result = func(audio_path)
            result["backend"] = name
            print(json.dumps(result))
            return
        except Exception as e:
            continue

    print(json.dumps({"error": "all backends failed"}))
    sys.exit(1)

if __name__ == "__main__":
    main()
