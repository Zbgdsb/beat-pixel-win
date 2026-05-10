#!/usr/bin/env python3
"""
Beat Detection Script - spectral flux + DP beat tracking
用法: python3 beat_detect.py <audio_file>
输出: JSON (success, beat_times_ms, bpm, total_notes, error)
"""
import sys
import json
import subprocess
import tempfile
import os
import numpy as np
from scipy.ndimage import median_filter
from scipy.signal import correlate

def convert_to_wav(input_path, sr=44100):
    """ffmpeg转WAV: 任意格式 → 44100Hz mono 16bit WAV"""
    tmp = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)
    tmp.close()
    cmd = [
        'ffmpeg', '-y', '-i', input_path,
        '-ar', str(sr), '-ac', '1', '-acodec', 'pcm_s16le',
        '-loglevel', 'error', tmp.name
    ]
    r = subprocess.run(cmd)
    if r.returncode != 0 or not os.path.exists(tmp.name) or os.path.getsize(tmp.name) < 100:
        if os.path.exists(tmp.name): os.unlink(tmp.name)
        raise ValueError(f"ffmpeg转换失败 (code={r.returncode})")
    return tmp.name

def load_wav(path):
    """读WAV文件，返回float64 samples + sample_rate"""
    import wave
    with wave.open(path, 'rb') as wf:
        sr = wf.getframerate()
        n = wf.getnframes()
        ch = wf.getnchannels()
        sw = wf.getsampwidth()
        raw = wf.readframes(n)
    if sw == 2:
        samples = np.frombuffer(raw, dtype=np.int16).astype(np.float64) / 32768.0
    elif sw == 4:
        samples = np.frombuffer(raw, dtype=np.int32).astype(np.float64) / 2147483648.0
    else:
        raise ValueError(f"不支持的采样位深: {sw*8}bit")
    if ch > 1:
        samples = samples.reshape(-1, ch).mean(axis=1)
    return samples, sr

def compute_spectral_flux(y, sr, hop=512, n_fft=2048):
    """计算频谱通量（半波整流）"""
    n_frames = (len(y) - n_fft) // hop
    if n_frames <= 0:
        return np.array([]), sr

    window = np.hanning(n_fft)
    prev_mag = None
    flux = np.zeros(n_frames)

    for i in range(n_frames):
        start = i * hop
        frame = y[start:start + n_fft] * window
        mag = np.abs(np.fft.rfft(frame))
        if prev_mag is not None:
            diff = mag - prev_mag
            flux[i] = np.sum(np.maximum(0, diff))
        prev_mag = mag

    # 归一化
    if np.max(flux) > 0:
        flux = flux / np.max(flux)
    return flux, sr

def detect_onsets(flux, hop, sr, min_interval_ms=80):
    """自适应阈值onset检测"""
    n = len(flux)
    if n < 32:
        return []

    # 局部均值/标准差
    win = 16
    lmean = np.array([np.mean(flux[max(0,i-win):i+1]) for i in range(n)])
    lstd = np.array([np.std(flux[max(0,i-win):i+1]) for i in range(n)])
    gmean = np.mean(flux)
    gstd = np.std(flux)

    thresh = np.maximum(lmean + 0.6 * lstd, gmean + 0.3 * gstd)

    onsets = []
    for i in range(2, n - 1):
        if flux[i] > thresh[i] and flux[i] > flux[i-1] and flux[i] >= flux[i+1]:
            t_ms = i * hop * 1000.0 / sr
            if t_ms < 500:  # 跳过前0.5秒
                continue
            if onsets and (t_ms - onsets[-1]) < min_interval_ms:
                # 保留能量更大的
                if flux[i] > flux[int(onsets[-1] * sr / (1000 * hop))]:
                    onsets[-1] = t_ms
                continue
            onsets.append(t_ms)
    return onsets

def estimate_bpm_from_onsets(onsets_ms):
    """从onset间隔估计BPM（中位数法）"""
    if len(onsets_ms) < 3:
        return 120.0
    intervals = np.diff(onsets_ms)
    # 过滤极端值
    valid = intervals[(intervals > 200) & (intervals < 2000)]
    if len(valid) == 0:
        return 120.0
    median = np.median(valid)
    bpm = 60000.0 / median
    # 如果BPM太低或太高，翻倍或减半
    while bpm < 60: bpm *= 2
    while bpm > 200: bpm /= 2
    return round(bpm, 1)

def dp_beat_tracking(onset_env, sr, hop, initial_bpm):
    """动态规划beat tracking（类似madmom DBN的核心思想）"""
    n = len(onset_env)
    if n < 10:
        return []

    # BPM对应的帧间隔
    beat_interval = int(round(60.0 * sr / (initial_bpm * hop)))
    if beat_interval < 2:
        beat_interval = 2

    # DP: 每个位置的最优得分
    score = np.full(n, -np.inf)
    prev = np.full(n, -1, dtype=int)
    score[0] = 0

    # 允许的间隔范围（±20%）
    min_gap = max(2, int(beat_interval * 0.8))
    max_gap = min(n - 1, int(beat_interval * 1.2) + 1)

    for i in range(1, n):
        # 跳过当前帧（不选为beat）
        if score[i-1] > score[i]:
            score[i] = score[i-1]
            prev[i] = prev[i-1] if prev[i-1] >= 0 else i-1

        # 尝试从前一个beat位置转移过来
        for gap in range(min_gap, max_gap + 1):
            j = i - gap
            if j < 0:
                continue
            # 转移得分 = 前一位置得分 + 当前onset能量 - 惩罚
            transition_score = score[j] + onset_env[i] * 2.0
            # 间隔越接近理想间隔，惩罚越小
            gap_penalty = abs(gap - beat_interval) * 0.1
            transition_score -= gap_penalty
            if transition_score > score[i]:
                score[i] = transition_score
                prev[i] = j

    # 回溯找到最优路径
    beats = []
    idx = n - 1
    # 找得分最高的结束位置
    best_end = np.argmax(score)
    idx = best_end

    while idx >= 0 and prev[idx] != idx:
        beats.append(idx)
        idx = prev[idx]
        if idx < 0:
            break
    if idx >= 0:
        beats.append(idx)

    beats.reverse()
    return beats

def grid_align(onsets_ms, bpm, tolerance_ms=30):
    """BPM网格对齐：把onset对齐到最近的节拍网格点"""
    if bpm <= 0 or len(onsets_ms) == 0:
        return onsets_ms

    beat_interval = 60000.0 / bpm  # 每拍毫秒数

    # 找到第一个onset附近的网格起点
    first = onsets_ms[0]
    # 网格起点 = first 对齐到 beat_interval
    grid_start = round(first / beat_interval) * beat_interval

    aligned = []
    for t in onsets_ms:
        # 计算最近的网格点
        grid_idx = round((t - grid_start) / beat_interval)
        grid_t = grid_start + grid_idx * beat_interval
        if abs(t - grid_t) <= tolerance_ms:
            aligned.append(round(grid_t))
        else:
            aligned.append(round(t))
    return aligned

def filter_sparse(onsets_ms, min_interval_ms=80):
    """过滤太密的点"""
    if not onsets_ms:
        return []
    result = [onsets_ms[0]]
    for t in onsets_ms[1:]:
        if t - result[-1] >= min_interval_ms:
            result.append(t)
    return result

def main():
    if len(sys.argv) < 2:
        print(json.dumps({"success": False, "error": "usage: beat_detect.py <audio_file>", "beat_times_ms": [], "bpm": 0, "total_notes": 0}))
        sys.exit(1)

    input_path = sys.argv[1]
    wav_path = None

    try:
        # 1. 转WAV
        wav_path = convert_to_wav(input_path, sr=44100)

        # 2. 读音频
        y, sr = load_wav(wav_path)

        # 3. 频谱通量
        flux, sr = compute_spectral_flux(y, sr, hop=512, n_fft=2048)

        # 4. onset检测
        onsets = detect_onsets(flux, hop=512, sr=44100, min_interval_ms=80)

        # 5. BPM估计
        bpm = estimate_bpm_from_onsets(onsets)

        # 6. DP beat tracking
        beat_frames = dp_beat_tracking(flux, sr=44100, hop=512, initial_bpm=bpm)

        # 7. 转毫秒
        beat_times = sorted(set([round(f * 512 * 1000.0 / 44100) for f in beat_frames if f * 512 * 1000.0 / 44100 > 500]))

        # 8. 合并onset和beat（去重）
        all_times = sorted(set(beat_times + [round(t) for t in onsets]))

        # 9. 过滤太密
        all_times = filter_sparse(all_times, min_interval_ms=80)

        # 10. BPM网格对齐
        all_times = grid_align(all_times, bpm, tolerance_ms=30)

        # 11. 再次过滤
        all_times = filter_sparse(all_times, min_interval_ms=80)

        result = {
            "success": True,
            "beat_times_ms": [int(t) for t in all_times],
            "bpm": float(bpm),
            "total_notes": len(all_times),
            "error": ""
        }

    except Exception as e:
        result = {
            "success": False,
            "beat_times_ms": [],
            "bpm": 0,
            "total_notes": 0,
            "error": str(e)
        }
    finally:
        if wav_path and os.path.exists(wav_path):
            os.unlink(wav_path)

    print(json.dumps(result))

if __name__ == "__main__":
    main()
