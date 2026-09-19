"use client";

// 主页面：左侧控制面板 + 右侧地图动画 / 播放器 / 统计 / 导出

import { useCallback, useEffect, useState } from "react";
import ControlPanel from "@/components/ControlPanel";
import MapCanvas from "@/components/MapCanvas";
import PlayerControls from "@/components/PlayerControls";
import {
  ApiError,
  downloadBlob,
  generateMapCsv,
  runSchedule,
  scheduleToCsv,
} from "@/lib/api";
import { getMockSchedule } from "@/lib/mock";
import type { MapGenParams, ScheduleResult } from "@/lib/types";

// 地图生成参数的默认值
const DEFAULT_PARAMS: MapGenParams = {
  width: 24,
  height: 16,
  agvCount: 3,
  cargoCount: 10,
  portCount: 2,
  obstacleRatio: 0.25,
};

// 上传后、运行前，用于静态预览的地图数据
interface MapPreview {
  width: number;
  height: number;
  grid: number[][];
}

// 基础帧率：1x 速度下每 250ms 推进一帧
const BASE_FRAME_MS = 250;

const LEGEND = [
  { color: "#ffffff", stroke: true, label: "路径" },
  { color: "#475569", label: "障碍" },
  { color: "#f59e0b", label: "货物" },
  { color: "#2563eb", label: "AGV" },
  { color: "#7c3aed", label: "AGV（载货）" },
  { color: "#10b981", label: "港口" },
];

export default function Home() {
  const [params, setParams] = useState<MapGenParams>(DEFAULT_PARAMS);
  const [file, setFile] = useState<File | null>(null);
  const [preview, setPreview] = useState<MapPreview | null>(null);
  const [result, setResult] = useState<ScheduleResult | null>(null);
  const [frameIdx, setFrameIdx] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(1);
  const [generating, setGenerating] = useState(false);
  const [running, setRunning] = useState(false);
  const [errors, setErrors] = useState<string[]>([]);

  const totalFrames = result?.frames.length ?? 0;

  /** 选择文件后本地解析 CSV，用于运行前的静态地图预览 */
  const handleFileChange = useCallback(async (f: File | null) => {
    setFile(f);
    setErrors([]);
    if (!f) {
      setPreview(null);
      return;
    }
    try {
      const text = await f.text();
      const grid = text
        .trim()
        .split(/\r?\n/)
        .map((line) => line.split(",").map(Number));
      const valid =
        grid.length > 0 &&
        grid.every(
          (row) =>
            row.length === grid[0].length &&
            row.every((v) => Number.isInteger(v) && v >= 0 && v <= 4)
        );
      if (!valid) {
        setPreview(null);
        setErrors(["地图文件格式不正确：应为逗号分隔的 0-4 数字网格"]);
        return;
      }
      setPreview({ width: grid[0].length, height: grid.length, grid });
    } catch {
      setPreview(null);
      setErrors(["读取地图文件失败"]);
    }
  }, []);

  /** 生成随机地图并触发下载 */
  const handleGenerate = useCallback(async () => {
    setGenerating(true);
    setErrors([]);
    try {
      await generateMapCsv(params);
    } catch (e) {
      setErrors(e instanceof ApiError ? e.errors : ["生成地图失败，请检查后端服务"]);
    } finally {
      setGenerating(false);
    }
  }, [params]);

  /** 上传当前文件并运行调度算法 */
  const handleRun = useCallback(async () => {
    if (!file) return;
    setRunning(true);
    setPlaying(false);
    setErrors([]);
    try {
      const data = await runSchedule(file);
      setResult(data);
      setFrameIdx(0);
    } catch (e) {
      setErrors(e instanceof ApiError ? e.errors : ["算法运行失败，请检查后端服务"]);
    } finally {
      setRunning(false);
    }
  }, [file]);

  /** 载入本地示例数据（后端未启动时预览界面效果） */
  const handleLoadDemo = useCallback(() => {
    const demo = getMockSchedule();
    setResult(demo);
    setPreview({ width: demo.width, height: demo.height, grid: demo.grid });
    setFrameIdx(0);
    setPlaying(false);
    setErrors([]);
  }, []);

  // 播放驱动：按速度换算间隔推进帧，播完自动停止
  useEffect(() => {
    if (!playing || !result) return;
    const timer = window.setInterval(() => {
      setFrameIdx((f) => {
        if (f >= result.frames.length - 1) {
          setPlaying(false);
          return f;
        }
        return f + 1;
      });
    }, BASE_FRAME_MS / speed);
    return () => window.clearInterval(timer);
  }, [playing, speed, result]);

  const handleSeek = useCallback(
    (f: number) => {
      if (!result) return;
      setFrameIdx(Math.min(Math.max(0, f), result.frames.length - 1));
    },
    [result]
  );

  // Canvas 数据源：运行结果优先，否则用上传文件的静态预览
  const mapData = result
    ? { width: result.width, height: result.height, grid: result.grid }
    : preview;
  const currentFrame = result?.frames[frameIdx] ?? null;

  /** 导出调度方案 JSON */
  const handleExportJson = useCallback(() => {
    if (!result) return;
    downloadBlob(
      new Blob([JSON.stringify(result, null, 2)], {
        type: "application/json",
      }),
      "schedule.json"
    );
  }, [result]);

  /** 导出逐帧动作表 CSV */
  const handleExportCsv = useCallback(() => {
    if (!result) return;
    downloadBlob(
      new Blob([scheduleToCsv(result)], { type: "text/csv" }),
      "schedule.csv"
    );
  }, [result]);

  return (
    <div className="flex min-h-screen flex-col">
      {/* 顶部标题栏 */}
      <header className="border-b border-slate-200 bg-white shadow-sm">
        <div className="mx-auto max-w-7xl px-6 py-4">
          <h1 className="text-xl font-bold text-slate-800">
            Spatio A* 多 AGV 调度演示平台
          </h1>
          <p className="mt-0.5 text-sm text-slate-500">
            时空 A* 路径规划 · 多 AGV 货物调度 · 逐帧动画回放
          </p>
        </div>
      </header>

      <main className="mx-auto flex w-full max-w-7xl flex-1 flex-col gap-6 px-6 py-6 lg:flex-row">
        {/* 左侧控制面板 */}
        <aside className="w-full shrink-0 lg:w-80">
          <ControlPanel
            params={params}
            onParamsChange={setParams}
            generating={generating}
            onGenerate={handleGenerate}
            file={file}
            onFileChange={handleFileChange}
            running={running}
            onRun={handleRun}
            errors={errors}
            onLoadDemo={handleLoadDemo}
          />
        </aside>

        {/* 右侧主区 */}
        <section className="flex min-w-0 flex-1 flex-col gap-4">
          {/* 地图画布卡片 */}
          <div className="rounded-2xl border border-slate-200 bg-white p-4 shadow-sm">
            <div className="mb-2 flex items-center justify-between">
              <h2 className="text-sm font-semibold text-slate-700">
                {result ? "调度动画" : "地图预览"}
              </h2>
              {mapData && (
                <span className="font-mono text-xs text-slate-400">
                  {mapData.width} × {mapData.height}
                </span>
              )}
            </div>
            {mapData ? (
              <MapCanvas
                width={mapData.width}
                height={mapData.height}
                grid={mapData.grid}
                cargos={result?.cargos}
                frame={currentFrame}
              />
            ) : (
              <div className="flex h-[480px] flex-col items-center justify-center gap-2 text-slate-400">
                <span className="text-4xl">🗺️</span>
                <p className="text-sm">
                  请先生成地图或上传 CSV 文件，也可以载入示例数据
                </p>
              </div>
            )}
            {/* 图例 */}
            <div className="mt-2 flex flex-wrap items-center gap-4 border-t border-slate-100 pt-3">
              {LEGEND.map((item) => (
                <span
                  key={item.label}
                  className="flex items-center gap-1.5 text-xs text-slate-500"
                >
                  <span
                    className={`h-3.5 w-3.5 rounded ${
                      item.stroke ? "border border-slate-300" : ""
                    }`}
                    style={{ backgroundColor: item.color }}
                  />
                  {item.label}
                </span>
              ))}
            </div>
          </div>

          {/* 动画播放器 */}
          <PlayerControls
            playing={playing}
            frame={frameIdx}
            totalFrames={totalFrames}
            speed={speed}
            onTogglePlay={() => setPlaying((p) => !p)}
            onPrev={() => handleSeek(frameIdx - 1)}
            onNext={() => handleSeek(frameIdx + 1)}
            onSeek={handleSeek}
            onSpeedChange={setSpeed}
            disabled={!result}
          />

          {/* 统计卡片 */}
          <div className="grid grid-cols-3 gap-4">
            <div className="rounded-2xl border border-slate-200 bg-white p-4 text-center shadow-sm">
              <p className="text-xs text-slate-400">总帧数（makespan）</p>
              <p className="mt-1 text-2xl font-bold text-slate-800">
                {result ? result.stats.makespan : "--"}
              </p>
            </div>
            <div className="rounded-2xl border border-slate-200 bg-white p-4 text-center shadow-sm">
              <p className="text-xs text-slate-400">总移动步数</p>
              <p className="mt-1 text-2xl font-bold text-slate-800">
                {result ? result.stats.totalMoves : "--"}
              </p>
            </div>
            <div className="rounded-2xl border border-slate-200 bg-white p-4 text-center shadow-sm">
              <p className="text-xs text-slate-400">已送达货物</p>
              <p className="mt-1 text-2xl font-bold text-slate-800">
                {result ? `${result.stats.deliveries} / ${result.cargos.length}` : "--"}
              </p>
            </div>
          </div>

          {/* 导出按钮 */}
          <div className="flex flex-wrap gap-3">
            <button
              type="button"
              onClick={handleExportJson}
              disabled={!result}
              className="h-10 rounded-lg bg-indigo-600 px-5 text-sm font-medium text-white transition hover:bg-indigo-500 disabled:cursor-not-allowed disabled:opacity-40"
            >
              导出调度方案（JSON）
            </button>
            <button
              type="button"
              onClick={handleExportCsv}
              disabled={!result}
              className="h-10 rounded-lg border border-indigo-200 px-5 text-sm font-medium text-indigo-600 transition hover:bg-indigo-50 disabled:cursor-not-allowed disabled:opacity-40"
            >
              导出 CSV（逐帧动作表）
            </button>
          </div>
        </section>
      </main>
    </div>
  );
}
