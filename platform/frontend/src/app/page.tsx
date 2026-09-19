"use client";

// 主页面：左侧控制面板 + 右侧地图动画 / 播放器 / 统计 / 导出
// 播放时钟按子步推进：子步索引 = t*3 + k（k ∈ {0,1,2}），逻辑帧时长基准 250ms

import Link from "next/link";
import { useCallback, useEffect, useRef, useState } from "react";
import ControlPanel from "@/components/ControlPanel";
import MapCanvas from "@/components/MapCanvas";
import PlayerControls from "@/components/PlayerControls";
import { useI18n } from "@/lib/i18n";
import {
  ApiError,
  downloadBlob,
  fetchEstimate,
  generateMapCsv,
  runSchedule,
  scheduleToCsv,
} from "@/lib/api";
import { getMockSchedule } from "@/lib/mock";
import { csvToFile, gridToCsv, parseMapCsv } from "@/lib/mapCsv";
import { frameOf, maxSubStep, phaseOf, SUBSTEPS_PER_FRAME } from "@/lib/render";
import type {
  EstimateResult,
  LoadedMapInfo,
  MapGenParams,
  ScheduleResult,
} from "@/lib/types";
import type { ParsedMap } from "@/lib/mapCsv";

// 地图生成参数的默认值
const DEFAULT_PARAMS: MapGenParams = {
  width: 24,
  height: 16,
  agvCount: 3,
  cargoCount: 10,
  portCount: 2,
  obstacleRatio: 0.25,
};

// 当前已载入的地图：预览数据 + 用于上传后端的 File + 面板展示信息 + 耗时预估
interface LoadedMap {
  name: string;
  file: File;
  preview: { width: number; height: number; grid: number[][] };
  info: LoadedMapInfo;
  /** 求解耗时预估（ms）；estimate 接口失败或未返回时为 null（静默降级） */
  estimate: EstimateResult | null;
}

// 基础帧率：1x 速度下逻辑帧时长 250ms，拆 3 子步后每子步 ≈ 83ms
const BASE_FRAME_MS = 250;

// 图例（颜色与 MapCanvas 内 COLORS 一致，文案走 i18n）
const LEGEND_ITEMS: Array<{
  key: "path" | "obstacle" | "cargo" | "agv" | "agvCarry" | "port";
  color: string;
  stroke?: boolean;
}> = [
  { key: "path", color: "#ffffff", stroke: true },
  { key: "obstacle", color: "#475569" },
  { key: "cargo", color: "#f59e0b" },
  { key: "agv", color: "#2563eb" },
  { key: "agvCarry", color: "#7c3aed" },
  { key: "port", color: "#10b981" },
];

export default function Home() {
  const { lang, setLang, t } = useI18n();
  const [params, setParams] = useState<MapGenParams>(DEFAULT_PARAMS);
  /** 当前已载入地图（上传 / 生成 / 示例数据共用），null 表示未载入 */
  const [map, setMap] = useState<LoadedMap | null>(null);
  const [result, setResult] = useState<ScheduleResult | null>(null);
  // 播放时钟：子步索引（t*3 + k），进度条/帧计数仍以逻辑帧为单位
  const [subIdx, setSubIdx] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(1);
  const [generating, setGenerating] = useState(false);
  const [running, setRunning] = useState(false);
  const [errors, setErrors] = useState<string[]>([]);
  const [notice, setNotice] = useState<string | null>(null);
  /** 运行计时（ms）：点击运行后开始计时，响应返回后停止并保留最终值 */
  const [elapsedMs, setElapsedMs] = useState<number | null>(null);
  const elapsedTimerRef = useRef<number | null>(null);

  const totalFrames = result?.frames.length ?? 0;
  const maxSub = maxSubStep(totalFrames);
  const curT = result ? Math.min(frameOf(subIdx), totalFrames - 1) : 0;
  const phase = result ? phaseOf(subIdx) : 0;

  const invalidFileMsg =
    lang === "zh"
      ? "地图文件格式不正确：应为逗号分隔的 0-4 数字网格"
      : "Invalid map file: expected a comma-separated grid of digits 0-4";

  const stopElapsedTimer = useCallback(() => {
    if (elapsedTimerRef.current != null) {
      window.clearInterval(elapsedTimerRef.current);
      elapsedTimerRef.current = null;
    }
  }, []);

  /** 开始运行计时：每 100ms 刷新一次，直到 stopElapsedTimer */
  const startElapsedTimer = useCallback(() => {
    stopElapsedTimer();
    const t0 = Date.now();
    setElapsedMs(0);
    elapsedTimerRef.current = window.setInterval(
      () => setElapsedMs(Date.now() - t0),
      100
    );
  }, [stopElapsedTimer]);

  // 组件卸载时清理计时器
  useEffect(() => stopElapsedTimer, [stopElapsedTimer]);

  /**
   * 按地图统计信息请求求解耗时预估，并写入对应地图状态。
   * estimate 接口失败（400/网络）时静默降级为 null（不显示预估与警告）。
   */
  const applyEstimate = useCallback(
    (target: LoadedMap, parsed: ParsedMap) => {
      fetchEstimate({
        cargoCount: parsed.cargoCount,
        openCount: parsed.openCount,
        agvCount: parsed.agvCount,
      })
        .then((estimate) => {
          setMap((prev) =>
            prev && prev.file === target.file ? { ...prev, estimate } : prev
          );
        })
        .catch(() => {
          /* 静默降级：不显示预估与超时警告 */
        });
    },
    []
  );

  /** 选择文件后解析 CSV 并载入为当前地图 */
  const handleFileChange = useCallback(
    async (f: File | null) => {
      setErrors([]);
      setNotice(null);
      setResult(null);
      setElapsedMs(null);
      stopElapsedTimer();
      if (!f) {
        setMap(null);
        return;
      }
      try {
        const parsed = parseMapCsv(await f.text());
        const loaded: LoadedMap = {
          name: f.name,
          file: f,
          preview: {
            width: parsed.width,
            height: parsed.height,
            grid: parsed.grid,
          },
          info: {
            name: f.name,
            width: parsed.width,
            height: parsed.height,
            agvCount: parsed.agvCount,
            cargoCount: parsed.cargoCount,
            portCount: parsed.portCount,
          },
          estimate: null,
        };
        setMap(loaded);
        applyEstimate(loaded, parsed);
      } catch {
        setMap(null);
        setErrors([invalidFileMsg]);
      }
    },
    [invalidFileMsg, applyEstimate, stopElapsedTimer]
  );

  /**
   * 生成随机地图：解析返回内容并自动载入为当前地图，
   * 预览立即显示，「运行算法」直接可用（不再自动下载，下载走「下载地图」按钮）
   */
  const handleGenerate = useCallback(async () => {
    setGenerating(true);
    setErrors([]);
    setNotice(null);
    try {
      const csv = await generateMapCsv(params);
      // 解析并自动载入
      const parsed = parseMapCsv(csv);
      const loaded: LoadedMap = {
        name: "map.csv",
        file: csvToFile(csv, "map.csv"),
        preview: {
          width: parsed.width,
          height: parsed.height,
          grid: parsed.grid,
        },
        info: {
          name: "map.csv",
          width: parsed.width,
          height: parsed.height,
          agvCount: parsed.agvCount,
          cargoCount: parsed.cargoCount,
          portCount: parsed.portCount,
        },
        estimate: null,
      };
      setMap(loaded);
      setResult(null);
      setSubIdx(0);
      setPlaying(false);
      setElapsedMs(null);
      stopElapsedTimer();
      setNotice(t("panel.mapLoaded"));
      applyEstimate(loaded, parsed);
    } catch (e) {
      setErrors(
        e instanceof ApiError
          ? e.errors
          : [
              lang === "zh"
                ? "生成地图失败，请检查后端服务"
                : "Failed to generate map; is the backend running?",
            ]
      );
    } finally {
      setGenerating(false);
    }
  }, [params, lang, t, applyEstimate, stopElapsedTimer]);

  /** 上传当前载入的地图并运行调度算法 */
  const handleRun = useCallback(async () => {
    if (!map) return;
    setRunning(true);
    setPlaying(false);
    setErrors([]);
    setNotice(null);
    startElapsedTimer();
    try {
      const data = await runSchedule(map.file);
      setResult(data);
      setSubIdx(0);
    } catch (e) {
      setErrors(
        e instanceof ApiError
          ? e.errors
          : [
              lang === "zh"
                ? "算法运行失败，请检查后端服务"
                : "Algorithm run failed; is the backend running?",
            ]
      );
    } finally {
      stopElapsedTimer();
      setRunning(false);
    }
  }, [map, lang, startElapsedTimer, stopElapsedTimer]);

  /** 下载当前已载入地图为 map.csv（File 由 mapCsv 序列化逻辑生成） */
  const handleDownloadMap = useCallback(() => {
    if (!map) return;
    downloadBlob(map.file, "map.csv");
  }, [map]);

  /** 载入本地示例数据（后端未启动时预览界面效果） */
  const handleLoadDemo = useCallback(() => {
    const demo = getMockSchedule();
    // 由示例网格序列化出真实 File，示例地图也可直接「运行算法」
    const csv = gridToCsv(demo.grid);
    const parsed = parseMapCsv(csv);
    const loaded: LoadedMap = {
      name: "demo.csv",
      file: csvToFile(csv, "demo.csv"),
      preview: { width: demo.width, height: demo.height, grid: demo.grid },
      info: {
        name: "demo.csv",
        width: parsed.width,
        height: parsed.height,
        agvCount: parsed.agvCount,
        cargoCount: parsed.cargoCount,
        portCount: parsed.portCount,
      },
      estimate: null,
    };
    setMap(loaded);
    setResult(demo);
    setSubIdx(0);
    setPlaying(false);
    setElapsedMs(null);
    stopElapsedTimer();
    setErrors([]);
    setNotice(null);
    applyEstimate(loaded, parsed);
  }, [applyEstimate, stopElapsedTimer]);

  // 播放驱动：按子步推进（1x 时子步间隔 ≈ 250/3 ms），播完自动停止
  useEffect(() => {
    if (!playing || !result) return;
    const timer = window.setInterval(() => {
      setSubIdx((s) => {
        if (s >= maxSubStep(result.frames.length)) {
          setPlaying(false);
          return s;
        }
        return s + 1;
      });
    }, BASE_FRAME_MS / SUBSTEPS_PER_FRAME / speed);
    return () => window.clearInterval(timer);
  }, [playing, speed, result]);

  /** 以逻辑帧为单位 seek（子步对齐到帧首） */
  const handleSeek = useCallback(
    (f: number) => {
      if (!result) return;
      const clamped = Math.min(Math.max(0, f), result.frames.length - 1);
      setSubIdx(clamped * SUBSTEPS_PER_FRAME);
    },
    [result]
  );

  /** 播放/暂停；已播到结尾时再按播放则从头开始 */
  const handleTogglePlay = useCallback(() => {
    if (!result) return;
    if (!playing && subIdx >= maxSub) setSubIdx(0);
    setPlaying((p) => !p);
  }, [result, playing, subIdx, maxSub]);

  // Canvas 数据源：运行结果优先，否则用已载入地图的静态预览
  const mapData = result
    ? { width: result.width, height: result.height, grid: result.grid }
    : map?.preview ?? null;
  const currentFrame = result?.frames[curT] ?? null;
  const nextFrame =
    result && curT < totalFrames - 1 ? result.frames[curT + 1] : null;

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
      {/* 顶部标题栏 + 语言切换 + 规则入口 */}
      <header className="border-b border-slate-200 bg-white shadow-sm">
        <div className="mx-auto flex max-w-7xl items-center justify-between gap-4 px-6 py-4">
          <div>
            <h1 className="text-xl font-bold text-slate-800">
              {t("app.title")}
            </h1>
            <p className="mt-0.5 text-sm text-slate-500">{t("app.subtitle")}</p>
          </div>
          <div className="flex items-center gap-2">
            <Link
              href="/rules"
              className="inline-flex h-9 items-center justify-center rounded-lg bg-amber-400 px-3 text-sm font-semibold text-amber-950 shadow-sm transition hover:bg-amber-300 hover:shadow"
            >
              {t("rules.open")}
            </Link>
            <div className="flex items-center gap-1 rounded-lg border border-slate-200 bg-slate-50 p-1">
              {(["zh", "en"] as const).map((l) => (
                <button
                  key={l}
                  type="button"
                  onClick={() => setLang(l)}
                  className={`h-8 rounded-md px-3 text-sm font-medium transition ${
                    lang === l
                      ? "bg-indigo-600 text-white shadow-sm"
                      : "text-slate-500 hover:text-slate-700"
                  }`}
                >
                  {t(l === "zh" ? "lang.zh" : "lang.en")}
                </button>
              ))}
            </div>
          </div>
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
            map={map?.info ?? null}
            onFileChange={handleFileChange}
            running={running}
            onRun={handleRun}
            elapsedMs={elapsedMs}
            estimate={map?.estimate ?? null}
            timeoutWarning={Boolean(
              map?.estimate && map.estimate.maxMs > 40000 && !result
            )}
            perf={
              result
                ? {
                    elapsedMs: result.stats.elapsedMs,
                    makespan: result.stats.makespan,
                    totalMoves: result.stats.totalMoves,
                    deliveries: result.stats.deliveries,
                    totalCargos: result.cargos.length,
                    agvCount: result.agvs.length,
                  }
                : null
            }
            onDownloadMap={handleDownloadMap}
            errors={errors}
            notice={notice}
            onLoadDemo={handleLoadDemo}
          />
        </aside>

        {/* 右侧主区 */}
        <section className="flex min-w-0 flex-1 flex-col gap-4">
          {/* 地图画布卡片 */}
          <div className="rounded-2xl border border-slate-200 bg-white p-4 shadow-sm">
            <div className="mb-2 flex items-center justify-between">
              <h2 className="text-sm font-semibold text-slate-700">
                {result ? t("canvas.animating") : t("canvas.preview")}
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
                events={result?.events}
                frame={currentFrame}
                nextFrame={nextFrame}
                phase={phase}
              />
            ) : (
              <div className="flex h-[480px] flex-col items-center justify-center gap-2 text-slate-400">
                <span className="text-4xl">🗺️</span>
                <p className="text-sm">{t("canvas.empty")}</p>
              </div>
            )}
            {/* 图例 */}
            <div className="mt-2 flex flex-wrap items-center gap-4 border-t border-slate-100 pt-3">
              {LEGEND_ITEMS.map((item) => (
                <span
                  key={item.key}
                  className="flex items-center gap-1.5 text-xs text-slate-500"
                >
                  <span
                    className={`h-3.5 w-3.5 rounded ${
                      item.stroke ? "border border-slate-300" : ""
                    }`}
                    style={{ backgroundColor: item.color }}
                  />
                  {t(`legend.${item.key}`)}
                </span>
              ))}
            </div>
          </div>

          {/* 动画播放器 */}
          <PlayerControls
            playing={playing}
            frame={curT}
            totalFrames={totalFrames}
            speed={speed}
            onTogglePlay={handleTogglePlay}
            onPrev={() => handleSeek(curT - 1)}
            onNext={() => handleSeek(curT + 1)}
            onSeek={handleSeek}
            onSpeedChange={setSpeed}
            disabled={!result}
          />

          {/* 统计卡片 */}
          <div className="grid grid-cols-3 gap-4">
            <div className="rounded-2xl border border-slate-200 bg-white p-4 text-center shadow-sm">
              <p className="text-xs text-slate-400">{t("stats.makespan")}</p>
              <p className="mt-1 text-2xl font-bold text-slate-800">
                {result ? result.stats.makespan : "--"}
              </p>
            </div>
            <div className="rounded-2xl border border-slate-200 bg-white p-4 text-center shadow-sm">
              <p className="text-xs text-slate-400">{t("stats.moves")}</p>
              <p className="mt-1 text-2xl font-bold text-slate-800">
                {result ? result.stats.totalMoves : "--"}
              </p>
            </div>
            <div className="rounded-2xl border border-slate-200 bg-white p-4 text-center shadow-sm">
              <p className="text-xs text-slate-400">{t("stats.delivered")}</p>
              <p className="mt-1 text-2xl font-bold text-slate-800">
                {result
                  ? `${result.stats.deliveries} / ${result.cargos.length}`
                  : "--"}
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
              {t("export.json")}
            </button>
            <button
              type="button"
              onClick={handleExportCsv}
              disabled={!result}
              className="h-10 rounded-lg border border-indigo-200 px-5 text-sm font-medium text-indigo-600 transition hover:bg-indigo-50 disabled:cursor-not-allowed disabled:opacity-40"
            >
              {t("export.csv")}
            </button>
          </div>
        </section>
      </main>
    </div>
  );
}
