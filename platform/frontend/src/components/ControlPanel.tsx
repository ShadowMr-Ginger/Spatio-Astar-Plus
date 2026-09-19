"use client";

// 左侧控制面板：地图生成参数、生成/上传/运行按钮、错误展示

import { useRef, useState } from "react";
import { useI18n } from "@/lib/i18n";
import type { EstimateResult, LoadedMapInfo, MapGenParams } from "@/lib/types";

/** 性能区块展示的数据（运行算法后填入，未运行时各项显示 "—"） */
export interface PerfSummary {
  elapsedMs?: number;
  makespan?: number;
  totalMoves?: number;
  deliveries?: number;
  totalCargos?: number;
  agvCount?: number;
}

interface ControlPanelProps {
  params: MapGenParams;
  onParamsChange: (params: MapGenParams) => void;
  generating: boolean;
  onGenerate: () => void;
  /** 当前已载入地图（上传或生成），null 表示未载入 */
  map: LoadedMapInfo | null;
  onFileChange: (file: File | null) => void;
  running: boolean;
  onRun: () => void;
  /** 本次/上次运行计时（ms），null 表示尚未运行；运行中每 100ms 刷新，结束后保留最终值 */
  elapsedMs: number | null;
  /** 当前地图的求解耗时预估（ms）；接口失败为 null，静默降级不显示预估 */
  estimate: EstimateResult | null;
  /** 预估上限超过 40s 时显示超时警告条 */
  timeoutWarning: boolean;
  /** 算法运行结果的性能数据，null 表示尚未运行 */
  perf: PerfSummary | null;
  /** 下载当前已载入地图为 map.csv */
  onDownloadMap: () => void;
  errors: string[];
  /** 一次性提示（如"地图已生成并载入"） */
  notice: string | null;
  onLoadDemo: () => void;
}

function NumberField({
  label,
  value,
  min,
  max,
  step,
  onChange,
}: {
  label: string;
  value: number;
  min?: number;
  max?: number;
  step?: number;
  onChange: (v: number) => void;
}) {
  return (
    <label className="block">
      <span className="mb-1 block text-xs font-medium text-slate-500">
        {label}
      </span>
      <input
        type="number"
        value={value}
        min={min}
        max={max}
        step={step}
        onChange={(e) => {
          const v = Number(e.target.value);
          if (!Number.isNaN(v)) onChange(v);
        }}
        className="h-9 w-full rounded-lg border border-slate-200 px-3 text-sm focus:outline-none focus:ring-2 focus:ring-indigo-300"
      />
    </label>
  );
}

/** 秒数格式化：向上取整到 0.1s 精度，保留 1 位小数 */
function formatSeconds(ms: number): string {
  return (Math.ceil(ms / 100) / 10).toFixed(1);
}

function PerfItem({ label, value }: { label: string; value: string | number }) {
  return (
    <div className="flex items-baseline justify-between gap-2">
      <dt className="text-xs text-slate-400">{label}</dt>
      <dd className="font-mono text-sm font-semibold text-slate-700">
        {value}
      </dd>
    </div>
  );
}

export default function ControlPanel({
  params,
  onParamsChange,
  generating,
  onGenerate,
  map,
  onFileChange,
  running,
  onRun,
  elapsedMs,
  estimate,
  timeoutWarning,
  perf,
  onDownloadMap,
  errors,
  notice,
  onLoadDemo,
}: ControlPanelProps) {
  const { t } = useI18n();
  const inputRef = useRef<HTMLInputElement>(null);
  const [dragOver, setDragOver] = useState(false);

  const busy = generating || running;

  return (
    <div className="flex flex-col gap-5 rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
      {/* 地图生成参数 */}
      <section>
        <h2 className="mb-3 text-sm font-semibold text-slate-700">
          {t("panel.mapParams")}
        </h2>
        <div className="grid grid-cols-2 gap-3">
          <NumberField
            label={t("panel.width")}
            value={params.width}
            min={4}
            max={200}
            onChange={(v) => onParamsChange({ ...params, width: v })}
          />
          <NumberField
            label={t("panel.height")}
            value={params.height}
            min={4}
            max={200}
            onChange={(v) => onParamsChange({ ...params, height: v })}
          />
          <NumberField
            label={t("panel.agvCount")}
            value={params.agvCount}
            min={1}
            max={50}
            onChange={(v) => onParamsChange({ ...params, agvCount: v })}
          />
          <NumberField
            label={t("panel.cargoCount")}
            value={params.cargoCount}
            min={1}
            max={200}
            onChange={(v) => onParamsChange({ ...params, cargoCount: v })}
          />
          <NumberField
            label={t("panel.portCount")}
            value={params.portCount}
            min={1}
            max={20}
            onChange={(v) => onParamsChange({ ...params, portCount: v })}
          />
          <NumberField
            label={t("panel.obstacleRatio")}
            value={params.obstacleRatio}
            min={0}
            max={0.9}
            step={0.05}
            onChange={(v) => onParamsChange({ ...params, obstacleRatio: v })}
          />
        </div>
        <button
          type="button"
          onClick={onGenerate}
          disabled={busy}
          className="mt-4 h-10 w-full rounded-lg bg-indigo-600 text-sm font-medium text-white transition hover:bg-indigo-500 disabled:cursor-not-allowed disabled:opacity-50"
        >
          {generating ? t("panel.generating") : t("panel.generate")}
        </button>
        <button
          type="button"
          onClick={onDownloadMap}
          disabled={!map}
          className="mt-2 h-9 w-full rounded-lg border border-slate-200 text-sm text-slate-500 transition hover:bg-slate-50 disabled:cursor-not-allowed disabled:opacity-50"
        >
          {t("panel.downloadMap")}
        </button>
        <p className="mt-2 text-xs text-slate-400">{t("panel.generateHint")}</p>
      </section>

      <hr className="border-slate-100" />

      {/* 文件上传区（点击或拖拽） */}
      <section>
        <h2 className="mb-3 text-sm font-semibold text-slate-700">
          {t("panel.mapFile")}
        </h2>
        <div
          role="button"
          tabIndex={0}
          onClick={() => inputRef.current?.click()}
          onKeyDown={(e) => e.key === "Enter" && inputRef.current?.click()}
          onDragOver={(e) => {
            e.preventDefault();
            setDragOver(true);
          }}
          onDragLeave={() => setDragOver(false)}
          onDrop={(e) => {
            e.preventDefault();
            setDragOver(false);
            const f = e.dataTransfer.files?.[0];
            if (f) onFileChange(f);
          }}
          className={`flex cursor-pointer flex-col items-center justify-center gap-1 rounded-xl border-2 border-dashed px-4 py-6 text-center transition ${
            dragOver
              ? "border-indigo-400 bg-indigo-50"
              : "border-slate-200 hover:border-indigo-300 hover:bg-slate-50"
          }`}
        >
          <span className="text-2xl">📄</span>
          <span className="text-sm text-slate-500">{t("panel.dropHint")}</span>
          <span className="text-xs text-slate-400">
            {map ? `${t("panel.selectedPrefix")}${map.name}` : t("panel.noFile")}
          </span>
        </div>
        <input
          ref={inputRef}
          type="file"
          accept=".csv,text/csv"
          className="hidden"
          onChange={(e) => onFileChange(e.target.files?.[0] ?? null)}
        />

        {/* 当前已载入地图的信息（尺寸 + 元素计数） */}
        {map && (
          <div className="mt-3 rounded-xl border border-slate-200 bg-slate-50 p-3">
            <p className="text-xs font-semibold text-slate-600">
              {t("panel.mapInfoTitle")}
            </p>
            <p className="mt-1 font-mono text-xs text-slate-500">
              {t("panel.mapInfo", {
                w: map.width,
                h: map.height,
                agv: map.agvCount,
                cargo: map.cargoCount,
                port: map.portCount,
              })}
            </p>
          </div>
        )}

        {/* 一次性提示（如生成后自动载入成功） */}
        {notice && (
          <div className="mt-3 rounded-xl border border-emerald-200 bg-emerald-50 p-3 text-xs text-emerald-600">
            {notice}
          </div>
        )}

        <button
          type="button"
          onClick={onRun}
          disabled={busy || !map}
          className="mt-4 h-10 w-full rounded-lg bg-blue-600 text-sm font-medium text-white transition hover:bg-blue-500 disabled:cursor-not-allowed disabled:opacity-50"
        >
          {running ? t("panel.running") : t("panel.run")}
        </button>

        {/* 预估耗时超阈值警告（>40s） */}
        {timeoutWarning && (
          <div className="mt-3 rounded-xl border border-amber-300 bg-amber-50 p-3 text-xs font-medium text-amber-700">
            {t("run.timeoutWarning")}
          </div>
        )}

        {/* 运行计时 + 预估（运行中显示预估，结束后计时保留最终值） */}
        {(elapsedMs != null || (running && estimate)) && (
          <div className="mt-3 flex items-center justify-between rounded-xl border border-slate-200 bg-slate-50 px-3 py-2 font-mono text-xs text-slate-600">
            <span>
              {elapsedMs != null &&
                t("run.elapsed", { v: formatSeconds(elapsedMs) })}
            </span>
            {running && estimate && (
              <span>
                {t("run.estimate", {
                  min: formatSeconds(estimate.minMs),
                  max: formatSeconds(estimate.maxMs),
                })}
              </span>
            )}
          </div>
        )}

        {/* 性能展示：运行算法后填入实际数据，未运行时显示 "—" */}
        <div className="mt-4 rounded-xl border border-slate-200 bg-slate-50 p-3">
          <p className="text-xs font-semibold text-slate-600">{t("perf.title")}</p>
          <dl className="mt-2 grid grid-cols-2 gap-x-3 gap-y-2">
            <PerfItem
              label={t("perf.elapsed")}
              value={
                perf?.elapsedMs != null ? `${perf.elapsedMs} ms` : "—"
              }
            />
            <PerfItem
              label={t("stats.makespan")}
              value={perf?.makespan ?? "—"}
            />
            <PerfItem
              label={t("stats.moves")}
              value={perf?.totalMoves ?? "—"}
            />
            <PerfItem
              label={t("stats.delivered")}
              value={
                perf &&
                perf.deliveries != null &&
                perf.totalCargos != null
                  ? `${perf.deliveries} / ${perf.totalCargos}`
                  : "—"
              }
            />
            <PerfItem label={t("perf.agv")} value={perf?.agvCount ?? "—"} />
          </dl>
        </div>
        <button
          type="button"
          onClick={onLoadDemo}
          disabled={busy}
          className="mt-2 h-9 w-full rounded-lg border border-slate-200 text-sm text-slate-500 transition hover:bg-slate-50 disabled:opacity-50"
        >
          {t("panel.loadDemo")}
        </button>
      </section>

      {/* 错误信息（后端 errors 原文展示，不做翻译） */}
      {errors.length > 0 && (
        <section className="rounded-xl border border-red-200 bg-red-50 p-3">
          <h3 className="mb-1 text-sm font-semibold text-red-600">
            {t("errors.title", { n: errors.length })}
          </h3>
          <ul className="list-inside list-disc text-xs text-red-500">
            {errors.map((err, i) => (
              <li key={i}>{err}</li>
            ))}
          </ul>
        </section>
      )}
    </div>
  );
}
