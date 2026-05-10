#!/usr/bin/env python3
"""
Beat Detection Script - 基于aubio专业音频分析库
用法: python3 beat_detect.py <audio_file>
输出: JSON格式的节拍检测结果（BPM、置信度、节拍时间戳、重音标记）
"""
import sys
import json
import subprocess
import tempfile
import os
import numpy as np

def read_audio_via_ffmpeg(audio_path, sr=22050):
    """用ffmpeg解码音频为WAV格式（aubio可以直接读WAV）"""
    tmp = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)
    tmp_path = tmp.name
    tmp.close()
    cmd = [
        'ffmpeg', '-y', '-i', audio_path,
        '-ar', str(sr), '-ac', '1',
        '-acodec', 'pcm_s16le',
        tmp_path
    ]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return tmp_path

def detect_with_aubio(audio_path):
    """用aubio检测节拍、BPM、onset"""
    import aubio

    sr = 22050
    win_s = 1024
    hop_s = 512

    # 1. BPM检测
    tempo_o = aubio.tempo("default", win_s, hop_s, sr)

    # 2. Onset检测
    onset_o = aubio.onset("default", win_s, hop_s, sr)

    # 3. 读取音频
    src = aubio.source(audio_path, sr, hop_s)
    duration = src.duration / sr

    beats = []
    onset_times = []
    onset_energies = []

    total_frames = 0
    while True:
        samples, read = src()
        is_beat = tempo_o(samples)
        is_onset = onset_o(samples)

        if is_onset:
            t_ms = onset_o.get_last_ms()
            onset_times.append(t_ms)
            # 获取onset的强度
            onset_energies.append(float(onset_o.get_descriptor()))

        if is_beat:
            pass  # BPM由tempo对象内部处理

        total_frames += read
        if read < hop_s:
            break

    # 4. 获取检测到的BPM
    bpm = float(tempo_o.get_bpm())
    confidence = 85.0  # aubio不直接给置信度，用默认值

    # 5. 通过onset间隔验证BPM置信度
    if len(onset_times) > 10:
        intervals = np.diff(onset_times)
        median_interval = np.median(intervals)
        expected_interval = 60000.0 / bpm

        # 如果中位间隔和BPM预期间隔接近，置信度高
        ratio = median_interval / expected_interval
        if 0.8 < ratio < 1.2 or 0.4 < ratio < 0.6 or 1.8 < ratio < 2.2:
            confidence = 95.0
        elif 0.6 < ratio < 1.5:
            confidence = 75.0
        else:
            confidence = 50.0

    # 6. 标记重音（能量较大的onset）
    if onset_energies:
        energy_arr = np.array(onset_energies)
        accent_threshold = np.mean(energy_arr) + 1.0 * np.std(energy_arr)
    else:
        accent_threshold = 0

    # 7. 去重：合并太近的onset（<150ms）
    filtered = []
    for i, t in enumerate(onset_times):
        if t < 1000:  # 跳过开头1秒
            continue
        if filtered and (t - filtered[-1]['timeMs']) < 150:
            # 保留能量更大的
            if onset_energies[i] > filtered[-1]['energy']:
                filtered[-1] = {
                    'timeMs': round(t),
                    'energy': onset_energies[i],
                    'isAccent': onset_energies[i] > accent_threshold
                }
            continue
        filtered.append({
            'timeMs': round(t),
            'energy': onset_energies[i],
            'isAccent': onset_energies[i] > accent_threshold
        })

    # 8. 输出结果
    result = {
        "bpm": round(bpm, 1),
        "confidence": round(confidence, 1),
        "duration": round(duration, 2),
        "beats": []
    }

    for item in filtered:
        result["beats"].append({
            "timeMs": int(item['timeMs']),
            "isAccent": bool(item['isAccent'])
        })

    return result

def main():
    if len(sys.argv) < 2:
        print(json.dumps({"error": "usage: beat_detect.py <audio_file>"}))
        sys.exit(1)

    audio_path = sys.argv[1]

    try:
        # aubio可以直接读大多数音频格式
        # 但有些格式(如AAC/M4A)需要先用ffmpeg转WAV
        tmp_path = None
        try:
            result = detect_with_aubio(audio_path)
        except Exception:
            # aubio读不了的格式，用ffmpeg转WAV再试
            tmp_path = read_audio_via_ffmpeg(audio_path)
            result = detect_with_aubio(tmp_path)

        # 清理临时文件
        if tmp_path and os.path.exists(tmp_path):
            os.unlink(tmp_path)

        print(json.dumps(result))

    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.exit(1)

if __name__ == "__main__":
    main()
