"use client";

// 左侧控制面板：地图生成参数、生成/上传/运行按钮、错误展示

import { useRef, useState } from "react";
import { useI18n } from "@/lib/i18n";
import type { MapGenParams } from "@/lib/types";

interface ControlPanelProps {
  params: MapGenParams;
  onParamsChange: (params: MapGenParams) => void;
  generating: boolean;
  onGenerate: () => void;
  file: File | null;
  onFileChange: (file: File | null) => void;
  running: boolean;
  onRun: () => void;
  errors: string[];
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

export default function ControlPanel({
  params,
  onParamsChange,
  generating,
  onGenerate,
  file,
  onFileChange,
  running,
  onRun,
  errors,
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
            {file
              ? `${t("panel.selectedPrefix")}${file.name}`
              : t("panel.noFile")}
          </span>
        </div>
        <input
          ref={inputRef}
          type="file"
          accept=".csv,text/csv"
          className="hidden"
          onChange={(e) => onFileChange(e.target.files?.[0] ?? null)}
        />
        <button
          type="button"
          onClick={onRun}
          disabled={busy || !file}
          className="mt-4 h-10 w-full rounded-lg bg-blue-600 text-sm font-medium text-white transition hover:bg-blue-500 disabled:cursor-not-allowed disabled:opacity-50"
        >
          {running ? t("panel.running") : t("panel.run")}
        </button>
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
