#!/usr/bin/env python3
"""
Beat Detection Script - 基于频谱onset检测
用法: python3 beat_detect.py <audio_file>
输出: JSON格式的节拍检测结果
"""
import sys
import json
import subprocess
import struct
import tempfile
import os
import numpy as np

def read_pcm_from_ffmpeg(audio_path, sr=22050):
    """用ffmpeg解码音频为PCM"""
    tmp = tempfile.NamedTemporaryFile(suffix='.pcm', delete=False)
    tmp_path = tmp.name
    tmp.close()
    cmd = [
        'ffmpeg', '-y', '-i', audio_path,
        '-f', 's16le', '-acodec', 'pcm_s16le',
        '-ac', '1', '-ar', str(sr),
        tmp_path
    ]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    with open(tmp_path, 'rb') as f:
        data = f.read()
    os.unlink(tmp_path)
    
    if len(data) < 2:
        raise ValueError('ffmpeg解码失败')
    
    samples = np.frombuffer(data, dtype=np.int16).astype(np.float64) / 32768.0
    return samples, sr

def compute_stft(samples, win_size=1024, hop=512):
    """计算短时傅里叶变换"""
    num_frames = (len(samples) - win_size) // hop
    window = np.hanning(win_size)
    stft = np.zeros((num_frames, win_size // 2 + 1))
    
    for i in range(num_frames):
        start = i * hop
        frame = samples[start:start + win_size] * window
        spectrum = np.abs(np.fft.rfft(frame))
        stft[i] = spectrum
    
    return stft

def compute_spectral_flux(stft):
    """计算频谱通量（半波整流）"""
    flux = np.zeros(stft.shape[0])
    for i in range(1, stft.shape[0]):
        diff = stft[i] - stft[i-1]
        flux[i] = np.sum(np.maximum(0, diff))  # 半波整流
    return flux

def detect_onsets(flux, hop, sr, min_interval_ms=150):
    """自适应阈值检测onset"""
    num_frames = len(flux)
    local_win = 16
    
    # 计算局部均值和标准差
    local_mean = np.zeros(num_frames)
    local_std = np.zeros(num_frames)
    for i in range(num_frames):
        start = max(0, i - local_win)
        window = flux[start:i+1]
        local_mean[i] = np.mean(window)
        local_std[i] = np.std(window)
    
    # 全局统计
    global_mean = np.mean(flux)
    global_std = np.std(flux)
    
    # 自适应阈值
    threshold = np.maximum(
        local_mean + 0.5 * local_std,
        global_mean + 0.2 * global_std
    )
    
    # 检测峰值
    min_interval_frames = int(min_interval_ms * sr / (1000 * hop))
    onsets = []
    
    for i in range(2, num_frames - 1):
        if flux[i] > threshold[i] and flux[i] > flux[i-1] and flux[i] >= flux[i+1]:
            time_ms = i * hop * 1000.0 / sr
            
            # 最小间隔检查
            if onsets and (time_ms - onsets[-1]) < min_interval_ms:
                if flux[i] > flux[i-1]:
                    onsets[-1] = time_ms
                continue
            
            # 跳过开头1秒
            if time_ms < 1000:
                continue
            
            onsets.append(time_ms)
    
    # 节拍太少则降低阈值
    if len(onsets) < 15:
        threshold2 = np.maximum(
            local_mean + 0.3 * local_std,
            global_mean + 0.1 * global_std
        )
        onsets = []
        for i in range(2, num_frames - 1):
            if flux[i] > threshold2[i] and flux[i] > flux[i-1] and flux[i] >= flux[i+1]:
                time_ms = i * hop * 1000.0 / sr
                if onsets and (time_ms - onsets[-1]) < min_interval_ms:
                    continue
                if time_ms < 1000:
                    continue
                onsets.append(time_ms)
    
    # 重音标记
    accent_threshold = global_mean + 1.5 * global_std
    accents = []
    for t in onsets:
        frame_idx = int(t * sr / (1000 * hop))
        if frame_idx < len(flux) and flux[frame_idx] > accent_threshold:
            accents.append(True)
        else:
            accents.append(False)
    
    return onsets, accents

def detect_bpm(flux, sr, hop):
    """用自相关检测BPM"""
    # 只用前30秒
    max_frames = min(len(flux), int(30 * sr / hop))
    signal = flux[:max_frames]
    
    # 归一化
    signal = signal - np.mean(signal)
    if np.std(signal) > 0:
        signal = signal / np.std(signal)
    
    # 自相关
    autocorr = np.correlate(signal, signal, mode='full')
    autocorr = autocorr[len(autocorr)//2:]
    
    # BPM范围: 60-200
    min_lag = int(60 * sr / (hop * 200))
    max_lag = int(60 * sr / (hop * 60))
    
    if max_lag >= len(autocorr):
        max_lag = len(autocorr) - 1
    
    # 找峰值
    search = autocorr[min_lag:max_lag]
    if len(search) == 0:
        return 120.0, 50.0
    
    peak_idx = np.argmax(search) + min_lag
    bpm = 60.0 * sr / (hop * peak_idx)
    
    # 计算置信度
    peak_val = autocorr[peak_idx]
    mean_val = np.mean(autocorr[min_lag:max_lag])
    confidence = min(100, max(0, (peak_val - mean_val) / (np.max(autocorr[1:min_lag]) - mean_val + 1e-10) * 100))
    
    return bpm, confidence

def main():
    if len(sys.argv) < 2:
        print(json.dumps({"error": "usage: beat_detect.py <audio_file>"}))
        sys.exit(1)
    
    audio_path = sys.argv[1]
    
    try:
        # 1. 读取音频
        samples, sr = read_pcm_from_ffmpeg(audio_path)
        duration = len(samples) / sr
        
        # 2. 计算STFT
        stft = compute_stft(samples)
        
        # 3. 计算频谱通量
        flux = compute_spectral_flux(stft)
        
        # 4. 检测BPM
        bpm, confidence = detect_bpm(flux, sr, 512)
        
        # 5. 检测onset
        onsets, accents = detect_onsets(flux, 512, sr)
        
        # 6. 输出JSON
        result = {
            "bpm": round(bpm, 1),
            "confidence": round(confidence, 1),
            "duration": round(duration, 2),
            "beats": []
        }
        
        for t, accent in zip(onsets, accents):
            result["beats"].append({
                "timeMs": round(t),
                "isAccent": accent
            })
        
        print(json.dumps(result))
        
    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.exit(1)

if __name__ == "__main__":
    main()
