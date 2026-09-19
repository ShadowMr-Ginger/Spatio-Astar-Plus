"use client";

// 动画播放控制条：播放/暂停、逐帧步进、可拖动进度条、倍速选择

import { useI18n } from "@/lib/i18n";

interface PlayerControlsProps {
  playing: boolean;
  /** 当前逻辑帧号 t（子步插值后仍按逻辑帧显示） */
  frame: number;
  totalFrames: number;
  speed: number;
  onTogglePlay: () => void;
  onPrev: () => void;
  onNext: () => void;
  onSeek: (frame: number) => void;
  onSpeedChange: (speed: number) => void;
  disabled: boolean;
}

const SPEEDS = [0.5, 1, 2, 4];

export default function PlayerControls({
  playing,
  frame,
  totalFrames,
  speed,
  onTogglePlay,
  onPrev,
  onNext,
  onSeek,
  onSpeedChange,
  disabled,
}: PlayerControlsProps) {
  const { t } = useI18n();
  const maxFrame = Math.max(0, totalFrames - 1);

  return (
    <div className="flex flex-wrap items-center gap-3 rounded-2xl border border-slate-200 bg-white p-4 shadow-sm">
      {/* 播放控制按钮组 */}
      <div className="flex items-center gap-1">
        <button
          type="button"
          onClick={onPrev}
          disabled={disabled || frame <= 0}
          title={t("player.prev")}
          className="h-9 w-9 rounded-lg border border-slate-200 text-slate-600 transition hover:bg-slate-50 disabled:cursor-not-allowed disabled:opacity-40"
        >
          ⏮
        </button>
        <button
          type="button"
          onClick={onTogglePlay}
          disabled={disabled}
          title={playing ? t("player.pause") : t("player.play")}
          className="h-9 w-12 rounded-lg bg-indigo-600 text-white transition hover:bg-indigo-500 disabled:cursor-not-allowed disabled:opacity-40"
        >
          {playing ? "⏸" : "▶"}
        </button>
        <button
          type="button"
          onClick={onNext}
          disabled={disabled || frame >= maxFrame}
          title={t("player.next")}
          className="h-9 w-9 rounded-lg border border-slate-200 text-slate-600 transition hover:bg-slate-50 disabled:cursor-not-allowed disabled:opacity-40"
        >
          ⏭
        </button>
      </div>

      {/* 进度条（以逻辑帧为单位拖动 seek） */}
      <input
        type="range"
        min={0}
        max={maxFrame}
        value={frame}
        disabled={disabled}
        onChange={(e) => onSeek(Number(e.target.value))}
        className="h-2 flex-1 cursor-pointer appearance-none rounded-lg bg-slate-200 accent-indigo-600 disabled:cursor-not-allowed disabled:opacity-40"
      />

      {/* 帧计数（逻辑帧） */}
      <span className="min-w-[110px] text-right font-mono text-sm text-slate-600">
        {disabled ? "-- / --" : `${t("player.frame")} ${frame} / ${maxFrame}`}
      </span>

      {/* 倍速选择 */}
      <select
        value={speed}
        disabled={disabled}
        onChange={(e) => onSpeedChange(Number(e.target.value))}
        className="h-9 rounded-lg border border-slate-200 bg-white px-2 text-sm text-slate-600 focus:outline-none focus:ring-2 focus:ring-indigo-300 disabled:opacity-40"
      >
        {SPEEDS.map((s) => (
          <option key={s} value={s}>
            {s}x
          </option>
        ))}
      </select>
    </div>
  );
}
