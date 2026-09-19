"use client";

// 轻量 i18n：dictionary + React Context，默认中文，选择持久化到 localStorage
// 语言状态通过 useSyncExternalStore 订阅 localStorage，避免水合不一致与 effect 级联渲染

import { createContext, useCallback, useContext } from "react";
import { useSyncExternalStore } from "react";
import type { ReactNode } from "react";

export type Lang = "zh" | "en";
export type TParams = Record<string, string | number>;

const zh = {
  "app.title": "Spatio A* 多 AGV 调度演示平台",
  "app.subtitle": "时空 A* 路径规划 · 多 AGV 货物调度 · 逐帧动画回放",
  "lang.zh": "中文",
  "lang.en": "EN",

  "panel.mapParams": "地图生成参数",
  "panel.width": "宽度 width",
  "panel.height": "高度 height",
  "panel.agvCount": "AGV 数量",
  "panel.cargoCount": "货物数量",
  "panel.portCount": "港口数量",
  "panel.obstacleRatio": "障碍比例",
  "panel.generate": "生成随机地图",
  "panel.generating": "正在生成地图…",
  "panel.generateHint": "生成的地图将以 map.csv 自动下载",

  "panel.mapFile": "地图文件",
  "panel.dropHint": "点击选择或拖拽 CSV 地图文件",
  "panel.noFile": "未选择文件",
  "panel.selectedPrefix": "已选择：",
  "panel.run": "运行算法",
  "panel.running": "算法运行中…",
  "panel.loadDemo": "载入示例数据（无需后端）",

  "errors.title": "出错了（{n}）",

  "canvas.animating": "调度动画",
  "canvas.preview": "地图预览",
  "canvas.empty": "请先生成地图或上传 CSV 文件，也可以载入示例数据",

  "legend.path": "路径",
  "legend.obstacle": "障碍",
  "legend.cargo": "货物",
  "legend.agv": "AGV",
  "legend.agvCarry": "AGV（载货）",
  "legend.port": "港口",

  "player.play": "播放",
  "player.pause": "暂停",
  "player.prev": "上一帧",
  "player.next": "下一帧",
  "player.frame": "帧",

  "stats.makespan": "总帧数（makespan）",
  "stats.moves": "总移动步数",
  "stats.delivered": "已送达货物",

  "export.json": "导出调度方案（JSON）",
  "export.csv": "导出 CSV（逐帧动作表）",
} as const;

export type TKey = keyof typeof zh;

const en: Record<TKey, string> = {
  "app.title": "Spatio A* Multi-AGV Scheduling Demo",
  "app.subtitle":
    "Spatio-temporal A* path planning · multi-AGV cargo scheduling · frame-by-frame playback",
  "lang.zh": "中文",
  "lang.en": "EN",

  "panel.mapParams": "Map parameters",
  "panel.width": "Width",
  "panel.height": "Height",
  "panel.agvCount": "AGV count",
  "panel.cargoCount": "Cargo count",
  "panel.portCount": "Port count",
  "panel.obstacleRatio": "Obstacle ratio",
  "panel.generate": "Generate random map",
  "panel.generating": "Generating map…",
  "panel.generateHint": "The map will be downloaded automatically as map.csv",

  "panel.mapFile": "Map file",
  "panel.dropHint": "Click to select or drop a CSV map file",
  "panel.noFile": "No file selected",
  "panel.selectedPrefix": "Selected: ",
  "panel.run": "Run algorithm",
  "panel.running": "Running algorithm…",
  "panel.loadDemo": "Load demo data (no backend needed)",

  "errors.title": "Errors ({n})",

  "canvas.animating": "Scheduling animation",
  "canvas.preview": "Map preview",
  "canvas.empty":
    "Generate a map or upload a CSV file first, or load the demo data",

  "legend.path": "Path",
  "legend.obstacle": "Obstacle",
  "legend.cargo": "Cargo",
  "legend.agv": "AGV",
  "legend.agvCarry": "AGV (carrying)",
  "legend.port": "Port",

  "player.play": "Play",
  "player.pause": "Pause",
  "player.prev": "Previous frame",
  "player.next": "Next frame",
  "player.frame": "Frame",

  "stats.makespan": "Frames (makespan)",
  "stats.moves": "Total moves",
  "stats.delivered": "Cargo delivered",

  "export.json": "Export schedule (JSON)",
  "export.csv": "Export CSV (per-frame actions)",
};

const STORAGE_KEY = "spa-lang";
const LANG_EVENT = "spa-lang-change";

/** 从 localStorage 读取语言偏好（默认中文） */
function readLang(): Lang {
  if (typeof window === "undefined") return "zh";
  return window.localStorage.getItem(STORAGE_KEY) === "en" ? "en" : "zh";
}

function subscribeLang(callback: () => void): () => void {
  window.addEventListener("storage", callback);
  window.addEventListener(LANG_EVENT, callback);
  return () => {
    window.removeEventListener("storage", callback);
    window.removeEventListener(LANG_EVENT, callback);
  };
}

interface I18nValue {
  lang: Lang;
  setLang: (lang: Lang) => void;
  t: (key: TKey, params?: TParams) => string;
}

const I18nContext = createContext<I18nValue | null>(null);

export function I18nProvider({ children }: { children: ReactNode }) {
  // SSR/水合兜底返回 zh；客户端快照不一致时 React 会自动客户端重渲染，不报错
  const lang = useSyncExternalStore<Lang>(subscribeLang, readLang, () => "zh");

  const setLang = useCallback((l: Lang) => {
    window.localStorage.setItem(STORAGE_KEY, l);
    window.dispatchEvent(new Event(LANG_EVENT));
  }, []);

  const t = useCallback(
    (key: TKey, params?: TParams) => {
      let s: string = (lang === "zh" ? zh : en)[key] ?? key;
      if (params) {
        for (const [k, v] of Object.entries(params)) {
          s = s.replaceAll(`{${k}}`, String(v));
        }
      }
      return s;
    },
    [lang]
  );

  return (
    <I18nContext.Provider value={{ lang, setLang, t }}>
      {children}
    </I18nContext.Provider>
  );
}

export function useI18n(): I18nValue {
  const ctx = useContext(I18nContext);
  if (!ctx) throw new Error("useI18n must be used within I18nProvider");
  return ctx;
}
