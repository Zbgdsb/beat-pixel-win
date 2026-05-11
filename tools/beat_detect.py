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

def compute_spectral_flux_bands(y, sr, hop=512, n_fft=2048):
    """分频段计算频谱通量（底鼓/军鼓/镲片三个频段）"""
    n_frames = (len(y) - n_fft) // hop
    if n_frames <= 0:
        return None, None, None, sr

    window = np.hanning(n_fft)
    prev_mag = None
    
    # 三个频段：底鼓(20~120Hz)、军鼓/通鼓(180~800Hz)、镲片(2000~15000Hz)
    freqs = np.fft.rfftfreq(n_fft, 1.0/sr)
    
    # 频段掩码
    kick_mask = (freqs >= 20) & (freqs <= 120)        # 底鼓频段
    snare_mask = (freqs >= 180) & (freqs <= 800)      # 军鼓/通鼓频段
    cymbal_mask = (freqs >= 2000) & (freqs <= 15000)  # 镲片频段
    
    flux_kick = np.zeros(n_frames)
    flux_snare = np.zeros(n_frames)
    flux_cymbal = np.zeros(n_frames)

    for i in range(n_frames):
        start = i * hop
        frame = y[start:start + n_fft] * window
        mag = np.abs(np.fft.rfft(frame))
        if prev_mag is not None:
            diff = mag - prev_mag
            # 分频段半波整流通量
            flux_kick[i] = np.sum(np.maximum(0, diff[kick_mask]))
            flux_snare[i] = np.sum(np.maximum(0, diff[snare_mask]))
            flux_cymbal[i] = np.sum(np.maximum(0, diff[cymbal_mask]))
        prev_mag = mag

    # 分别归一化
    for flux in [flux_kick, flux_snare, flux_cymbal]:
        if np.max(flux) > 0:
            flux /= np.max(flux)

    return flux_kick, flux_snare, flux_cymbal, sr

def detect_onsets_band(flux, hop, sr, min_interval_ms=80, drum_type=""):
    """单频段onset检测（带鼓类型标签）"""
    n = len(flux)
    if n < 32:
        return []

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
            if t_ms < 500:
                continue
            if onsets and (t_ms - onsets[-1]['time']) < min_interval_ms:
                if flux[i] > flux[int(onsets[-1]['time'] * sr / (1000 * hop))]:
                    onsets[-1] = {'time': t_ms, 'energy': flux[i], 'drum_type': drum_type}
                continue
            onsets.append({'time': t_ms, 'energy': flux[i], 'drum_type': drum_type})
    return onsets

def classify_drum_type(onset, y, sr, hop=512, n_fft=2048):
    """进一步细分鼓组类型（通鼓→高/中/低，镲片→hihat/ride/crash）"""
    frame_idx = int(onset['time'] * sr / (1000 * hop))
    start = frame_idx * hop
    end = min(start + n_fft * 2, len(y))
    frame = y[start:end]
    
    if len(frame) < n_fft:
        return onset['drum_type']
    
    # 计算该onset附近的频谱
    freqs = np.fft.rfftfreq(n_fft, 1.0/sr)
    mag = np.abs(np.fft.rfft(frame[:n_fft] * np.hanning(n_fft)))
    
    # 找峰值频率
    peak_freq = freqs[np.argmax(mag)]
    
    if onset['drum_type'] == 'snare':
        # 军鼓 vs 通鼓：按峰值频率细分
        if peak_freq < 150:
            return 'tom_low'
        elif peak_freq < 250:
            return 'tom_mid'
        elif peak_freq < 400:
            return 'tom_high'
        else:
            return 'snare'
    elif onset['drum_type'] == 'cymbal':
        # 镲片细分：用时域能量衰减区分 hihat/ride/crash
        # 计算onset后音频包络的衰减速率
        onset_sample = int(onset['time'] * sr / 1000)
        tail_start = min(onset_sample + n_fft // 2, len(y))
        tail_end = min(onset_sample + n_fft * 3, len(y))
        tail = y[tail_start:tail_end]
        if len(tail) < 512:
            return 'hihat'
        
        # 分帧计算包络能量
        frame_len = 512
        n_env_frames = len(tail) // frame_len
        if n_env_frames < 3:
            return 'hihat'
        env_energy = np.array([np.sqrt(np.mean(tail[i*frame_len:(i+1)*frame_len]**2)) for i in range(n_env_frames)])
        
        if env_energy[0] == 0:
            return 'hihat'
        
        # 衰减比：第3帧能量 / 第1帧能量
        decay_ratio = env_energy[min(3, n_env_frames-1)] / env_energy[0]
        
        # 宽频能量比（crash频谱更宽）
        full_energy = np.sum(mag)
        high_energy = np.sum(mag[freqs > 8000]) if np.any(freqs > 8000) else 0
        high_ratio = high_energy / full_energy if full_energy > 0 else 0
        
        if decay_ratio > 0.6 and high_ratio > 0.15:
            return 'crash'   # 衰减慢+高频多 → 吊镲
        elif decay_ratio > 0.3 or high_ratio > 0.08:
            return 'ride'    # 中等衰减 → 叮叮镲
        else:
            return 'hihat'   # 衰减快 → 踩镲
    return onset['drum_type']

def estimate_bpm_from_onsets(onsets_ms):
    """从onset间隔估计BPM（中位数法）"""
    if len(onsets_ms) < 3:
        return 120.0
    intervals = np.diff(onsets_ms)
    valid = intervals[(intervals > 200) & (intervals < 2000)]
    if len(valid) == 0:
        return 120.0
    median = np.median(valid)
    bpm = 60000.0 / median
    while bpm < 80: bpm *= 2
    while bpm > 200: bpm /= 2
    return round(bpm, 1)

def weighted_dp_beat_tracking(onset_env, sr, hop, initial_bpm, drum_labels=None):
    """加权DP beat tracking：底鼓2x、军鼓1.5x、镲片0.5x"""
    n = len(onset_env)
    if n < 10:
        return []

    beat_interval = int(round(60.0 * sr / (initial_bpm * hop)))
    if beat_interval < 2:
        beat_interval = 2

    # 构建权重数组（按帧）
    weights = np.ones(n)
    if drum_labels:
        for i in range(min(n, len(drum_labels))):
            dt = drum_labels[i]
            if dt == 'kick':
                weights[i] = 2.0
            elif dt == 'snare':
                weights[i] = 1.5
            elif dt in ('hihat', 'ride', 'crash'):
                weights[i] = 0.5

    score = np.full(n, -np.inf)
    prev = np.full(n, -1, dtype=int)
    score[0] = 0

    min_gap = max(2, int(beat_interval * 0.8))
    max_gap = min(n - 1, int(beat_interval * 1.2) + 1)

    for i in range(1, n):
        if score[i-1] > score[i]:
            score[i] = score[i-1]
            prev[i] = prev[i-1] if prev[i-1] >= 0 else i-1
        for gap in range(min_gap, max_gap + 1):
            j = i - gap
            if j < 0:
                continue
            transition_score = score[j] + onset_env[i] * 2.0 * weights[i]
            gap_penalty = abs(gap - beat_interval) * 0.1
            transition_score -= gap_penalty
            if transition_score > score[i]:
                score[i] = transition_score
                prev[i] = j

    beats = []
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

def grid_align(onsets_ms, bpm, tolerance_ms=30, drum_labels=None):
    """鼓组分档容差网格对齐：核心鼓±10ms，装饰鼓±20ms，Hihat±30ms"""
    if bpm <= 0 or len(onsets_ms) == 0:
        return onsets_ms

    beat_interval = 60000.0 / bpm
    first = onsets_ms[0]
    grid_start = round(first / beat_interval) * beat_interval

    aligned = []
    for idx, t in enumerate(onsets_ms):
        # 获取鼓类型
        drum = 'unknown'
        if drum_labels and idx < len(drum_labels):
            drum = drum_labels[idx]
        
        # 分档容差
        if drum in ('kick', 'snare'):
            tol = 10.0   # 核心鼓：±10ms
        elif drum in ('tom_low', 'tom_mid', 'tom_high', 'ride'):
            tol = 20.0   # 装饰鼓：±20ms
        else:
            tol = 30.0   # Hihat/未知：±30ms
        
        grid_idx = round((t - grid_start) / beat_interval)
        grid_t = grid_start + grid_idx * beat_interval
        
        if abs(t - grid_t) <= tol:
            aligned.append(round(grid_t))
        elif drum in ('kick', 'snare'):
            # 核心鼓超出容差，判定为误检，直接删除
            continue
        else:
            # 非核心鼓，强制拉到最近网格点
            aligned.append(round(grid_t))
    
    return aligned

# ========== 鼓点 Flow 模板系统 V2 ==========
# 模拟真实鼓手的联动套路，包含：
# - Kick/Snare 总是和 Hiat 联动
# - 密集的 Hiat 16分音符
# - Tom fill 快速三连击
# - 双押（Kick+Snare, Snare+Hiat）

drum_templates = {
    'rock_basic': {
        'name': 'Rock基本beat',
        'bpm_range': (100, 160),
        'pattern': [
            # 位置, 鼓类型列表（同时触发的鼓）
            (0,   ['kick', 'hihat']),        # 第1拍：底鼓+踩镲
            (0.5, ['hihat']),                 # 1拍后半：踩镲
            (1,   ['snare', 'hihat']),       # 第2拍：军鼓+踩镲
            (1.5, ['hihat']),                 # 2拍后半：踩镲
            (2,   ['kick', 'hihat']),        # 第3拍：底鼓+踩镲
            (2.5, ['hihat']),                 # 3拍后半：踩镲
            (3,   ['snare', 'hihat']),       # 第4拍：军鼓+踩镲
            (3.5, ['hihat']),                 # 4拍后半：踩镲
        ],
        'fill': {
            'bar': 4,  # 每4小节一次fill
            'pattern': [
                (0,   ['tom_high']),
                (0.3, ['tom_high']),
                (0.6, ['tom_mid']),
                (1.0, ['tom_mid']),
                (1.3, ['tom_low']),
                (1.6, ['tom_low']),
                (2.0, ['tom_low']),
                (2.5, ['tom_low']),
                (3.0, ['kick', 'crash']),  # fill结束：底鼓+强音镲
            ]
        }
    },
    'rock_heavy': {
        'name': 'Rock强力beat',
        'bpm_range': (80, 120),
        'pattern': [
            (0,   ['kick', 'hihat']),
            (0.5, ['hihat']),
            (1,   ['snare', 'hihat']),
            (1.5, ['hihat']),
            (2,   ['kick', 'hihat']),
            (2.5, ['kick', 'hihat']),  # 双底鼓
            (3,   ['snare', 'hihat']),
            (3.5, ['hihat']),
        ],
        'fill': {
            'bar': 4,
            'pattern': [
                (0,   ['tom_high']),
                (0.2, ['tom_high']),
                (0.4, ['tom_mid']),
                (0.6, ['tom_mid']),
                (0.8, ['tom_low']),
                (1.0, ['tom_low']),
                (1.2, ['tom_low']),
                (2.0, ['snare']),
                (3.0, ['kick', 'crash']),
            ]
        }
    },
    'pop_basic': {
        'name': 'Pop基本beat',
        'bpm_range': (90, 130),
        'pattern': [
            (0,   ['kick', 'hihat']),
            (0.5, ['hihat']),
            (1,   ['snare', 'hihat']),
            (1.5, ['hihat']),
            (2,   ['kick', 'hihat']),
            (2.5, ['hihat']),
            (3,   ['snare', 'hihat']),
            (3.5, ['hihat']),
        ],
        'fill': {
            'bar': 4,
            'pattern': [
                (0,   ['tom_high']),
                (0.5, ['tom_mid']),
                (1.0, ['tom_low']),
                (2.0, ['snare']),
                (3.0, ['kick', 'crash']),
            ]
        }
    },
    'funk_basic': {
        'name': 'Funk基本beat',
        'bpm_range': (90, 120),
        'pattern': [
            (0,   ['kick', 'hihat']),
            (0.25, ['hihat']),  # 16分音符
            (0.5, ['hihat']),
            (0.75, ['hihat']),
            (1,   ['snare', 'hihat']),
            (1.25, ['hihat']),
            (1.5, ['hihat']),
            (1.75, ['hihat']),
            (2,   ['kick', 'hihat']),
            (2.25, ['hihat']),
            (2.5, ['hihat']),
            (2.75, ['hihat']),
            (3,   ['snare', 'hihat']),
            (3.25, ['hihat']),
            (3.5, ['hihat']),
            (3.75, ['hihat']),
        ],
        'fill': {
            'bar': 2,
            'pattern': [
                (0,   ['tom_high']),
                (0.3, ['tom_mid']),
                (0.6, ['tom_low']),
                (1.0, ['tom_high']),
                (1.3, ['tom_mid']),
                (1.6, ['tom_low']),
                (2.0, ['kick', 'crash']),
            ]
        }
    },
    'ballad_basic': {
        'name': 'Ballad基本beat',
        'bpm_range': (60, 90),
        'pattern': [
            (0,   ['kick']),
            (1,   ['hihat']),
            (2,   ['snare']),
            (3,   ['hihat']),
        ],
        'fill': {
            'bar': 8,
            'pattern': [
                (0,   ['tom_high']),
                (1.0, ['tom_mid']),
                (2.0, ['tom_low']),
                (3.0, ['kick', 'crash']),
            ]
        }
    },
    'hiphop_basic': {
        'name': 'Hip-Hop基本beat',
        'bpm_range': (80, 110),
        'pattern': [
            (0,   ['kick', 'hihat']),
            (0.5, ['hihat']),
            (1,   ['snare']),
            (1.5, ['kick']),
            (2,   ['hihat']),
            (2.5, ['hihat']),
            (3,   ['snare']),
            (3.5, ['hihat']),
        ],
        'fill': {
            'bar': 4,
            'pattern': [
                (0,   ['tom_high']),
                (0.5, ['tom_mid']),
                (1.0, ['tom_low']),
                (2.0, ['kick', 'crash']),
            ]
        }
    },
}

def select_template(bpm, energy_mean):
    """根据BPM和能量选择最匹配的flow模板"""
    candidates = []
    for key, tmpl in drum_templates.items():
        lo, hi = tmpl['bpm_range']
        if lo <= bpm <= hi:
            candidates.append((key, tmpl))
    
    if not candidates:
        return 'rock_basic'
    
    if energy_mean > 0.3 and 'rock_heavy' in [c[0] for c in candidates]:
        return 'rock_heavy'
    
    return candidates[0][0]

def generate_template_chart(bpm, duration_sec, template_key):
    """根据模板生成完整谱面骨架（支持双押和联动）"""
    tmpl = drum_templates[template_key]
    beat_interval = 60000.0 / bpm
    bar_duration = beat_interval * 4
    
    total_bars = int(duration_sec * 1000 / bar_duration) + 1
    chart = []
    
    for bar in range(total_bars):
        bar_start = bar * bar_duration + 2000  # 跳过开头2秒
        
        # 检查是否是fill小节
        fill = tmpl.get('fill')
        is_fill_bar = (fill and (bar + 1) % fill['bar'] == 0)
        
        if is_fill_bar and fill:
            # Fill小节：使用fill pattern
            for pos, drums in fill['pattern']:
                t = bar_start + pos * beat_interval
                if t >= 2000:
                    for drum in drums:
                        chart.append({'time': int(t), 'drum': drum})
        else:
            # 普通小节：使用主pattern
            for pos, drums in tmpl['pattern']:
                t = bar_start + pos * beat_interval
                if t >= 2000:
                    for drum in drums:
                        chart.append({'time': int(t), 'drum': drum})
    
    return chart

def merge_template_and_detection(template_chart, detected_onsets, tolerance_ms=50):
    """合并模板骨架和实际检测结果：检测到的优先，模板补充缺失，Tom fill强制插入"""
    result = []
    detected_times = set()
    
    # 1. 先把检测到的onset放进去（优先级最高）
    for o in detected_onsets:
        result.append({'time': int(o['time']), 'drum': o['drum_type']})
        detected_times.add(int(o['time']))
    
    # 2. 模板补充：Tom fill和双押强制插入，其他按常规合并
    for t_note in template_chart:
        t = t_note['time']
        drum = t_note['drum']
        
        # Tom fill和双押强制插入
        if drum in ('tom_high', 'tom_mid', 'tom_low', 'crash'):
            result.append(t_note)
            continue
        
        # 其他鼓按常规合并
        has_nearby = any(abs(t - dt) < tolerance_ms for dt in detected_times)
        if not has_nearby:
            result.append(t_note)
    
    # 按时间排序
    result.sort(key=lambda x: x['time'])
    return result

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

        # 3. 分频段频谱通量（底鼓/军鼓/镲片 三路并行检测
        flux_kick, flux_snare, flux_cymbal, sr = compute_spectral_flux_bands(y, sr, hop=512, n_fft=2048)
        
        # 4. 三个频段分别检测onset
        onsets_kick = detect_onsets_band(flux_kick, hop=512, sr=44100, min_interval_ms=120, drum_type='kick')
        onsets_snare = detect_onsets_band(flux_snare, hop=512, sr=44100, min_interval_ms=80, drum_type='snare')
        onsets_cymbal = detect_onsets_band(flux_cymbal, hop=512, sr=44100, min_interval_ms=60, drum_type='cymbal')

        # 合并所有onset
        all_onsets = onsets_kick + onsets_snare + onsets_cymbal
        
        # 5. 进一步细分鼓组类型
        for onset in all_onsets:
            onset['drum_type'] = classify_drum_type(onset, y, sr)

        # 6. 按时间排序
        all_onsets.sort(key=lambda x: x['time'])

        # 7. BPM估计：底鼓优先交叉校验
        kick_times = [o['time'] for o in all_onsets if o['drum_type'] == 'kick']
        all_times_raw = [o['time'] for o in all_onsets]
        
        bpm_general = estimate_bpm_from_onsets(all_times_raw) if len(all_times_raw) >= 3 else 120.0
        bpm_kick = estimate_bpm_from_onsets(kick_times) if len(kick_times) >= 3 else bpm_general
        
        # 交叉校验：误差<5%用底鼓BPM，>5%也用底鼓BPM（更稳）
        if bpm_general > 0 and bpm_kick > 0:
            diff_ratio = abs(bpm_kick - bpm_general) / bpm_general
            if diff_ratio < 0.05:
                bpm = bpm_kick
            else:
                bpm = bpm_kick  # 底鼓永远最稳
        elif bpm_kick > 0:
            bpm = bpm_kick
        else:
            bpm = bpm_general

        # 8. 选择鼓点flow模板
        energy_mean = np.mean([o['energy'] for o in all_onsets]) if all_onsets else 0
        template_key = select_template(bpm, energy_mean)
        print(f"[beat_detect] BPM={bpm}, template={template_key}", file=sys.stderr)
        
        # 9. 用模板生成骨架谱面
        template_chart = generate_template_chart(bpm, len(y)/sr, template_key)
        
        # 10. 合并模板骨架和实际检测结果
        merged_chart = merge_template_and_detection(template_chart, all_onsets)
        
        # 11. 网格对齐每个独立音符（不去重，允许多鼓同时）
        beat_interval = 60000.0 / bpm
        grid_start = round(merged_chart[0]['time'] / beat_interval) * beat_interval if merged_chart else 0
        
        aligned_notes = []
        for note in merged_chart:
            grid_idx = round((note['time'] - grid_start) / beat_interval)
            aligned_t = int(grid_start + grid_idx * beat_interval)
            if aligned_t >= 2000:
                aligned_notes.append({'time': aligned_t, 'drum': note['drum']})
        
        # 12. 去重：同一时间+同一鼓类型 只保留一个
        seen = set()
        deduped = []
        aligned_notes.sort(key=lambda x: (x['time'], x['drum']))
        for n in aligned_notes:
            key = (n['time'], n['drum'])
            if key not in seen:
                seen.add(key)
                deduped.append(n)
        
        # 13. 按时间排序后过滤太密（同一时间的不同鼓不筛）
        deduped.sort(key=lambda x: x['time'])
        last_time = -1000
        filtered = []
        for n in deduped:
            if n['time'] != last_time and n['time'] - last_time < 60:
                continue  # 跳过太密的
            filtered.append(n)
            last_time = n['time']
        
        beat_times_with_drums = filtered

        all_times = sorted(set(n['time'] for n in filtered))

        result = {
            "success": True,
            "beats": beat_times_with_drums,
            "beat_times_ms": all_times,
            "bpm": float(bpm),
            "total_notes": len(all_times),
            "drum_counts": {
                "kick": sum(1 for o in all_onsets if o['drum_type'] == 'kick'),
                "snare": sum(1 for o in all_onsets if o['drum_type'] == 'snare'),
                "hihat": sum(1 for o in all_onsets if o['drum_type'] == 'hihat'),
                "tom_low": sum(1 for o in all_onsets if o['drum_type'] == 'tom_low'),
                "tom_mid": sum(1 for o in all_onsets if o['drum_type'] == 'tom_mid'),
                "tom_high": sum(1 for o in all_onsets if o['drum_type'] == 'tom_high'),
                "ride": sum(1 for o in all_onsets if o['drum_type'] == 'ride'),
                "crash": sum(1 for o in all_onsets if o['drum_type'] == 'crash')
            },
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
