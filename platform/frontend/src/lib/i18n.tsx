"use client";

// 轻量 i18n：dictionary + React Context；无存储偏好时默认跟随浏览器语言，手动切换持久化到 localStorage
// 语言状态通过 useSyncExternalStore 订阅 localStorage，避免水合不一致与 effect 级联渲染

import { createContext, useCallback, useContext, useEffect } from "react";
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
  "panel.generateHint": "生成的地图将自动载入为当前地图",
  "panel.downloadMap": "下载地图",

  "panel.mapFile": "地图文件",
  "panel.dropHint": "点击选择或拖拽 CSV 地图文件",
  "panel.noFile": "未选择文件",
  "panel.selectedPrefix": "已选择：",
  "panel.run": "运行算法",
  "panel.running": "算法运行中…",
  "panel.loadDemo": "载入示例数据（无需后端）",

  "run.elapsed": "已运行 {v} s",
  "run.estimate": "预估 {min}~{max} s",
  "run.timeoutWarning":
    "该地图求解可能超出响应时间限制（>40s），可能超时失败，建议减少货物数量或增大地图",

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

  "perf.title": "性能",
  "perf.elapsed": "算法求解耗时",
  "perf.agv": "AGV 数量",

  "export.json": "导出调度方案（JSON）",
  "export.csv": "导出 CSV（逐帧动作表）",

  "panel.mapInfoTitle": "当前地图",
  "panel.mapInfo": "尺寸 {w} × {h} · AGV {agv} · 货物 {cargo} · 港口 {port}",
  "panel.mapLoaded": "地图已生成并载入，可直接运行算法",

  "rules.open": "规则介绍",
  "rules.title": "规则介绍",
  "rules.back": "← 返回主页",
  "rules.map.title": "地图规则",
  "rules.map.b1": "地图为矩形网格，尺寸由 width × height 决定",
  "rules.map.b2": "AGV 沿上下左右四连通方向移动，每次移动一格",
  "rules.map.b3": "障碍节点（1）不可通行",
  "rules.csv.title": "CSV 数字含义",
  "rules.csv.value": "数字",
  "rules.csv.meaning": "含义",
  "rules.csv.v0": "可通行路径节点（AGV 可达）",
  "rules.csv.v1": "障碍节点（不可通行）",
  "rules.csv.v2": "货物节点（可通行，需运送到港口）",
  "rules.csv.v3": "AGV 初始位置（可通行）",
  "rules.csv.v4": "港口节点（可通行，货物送达目标）",
  "rules.csv.note": "数字 2 / 3 / 4 都属于可通行路径节点。",
  "rules.task.title": "任务规则",
  "rules.task.b1": "多个 AGV 从数字 3（AGV 初始位置）出发",
  "rules.task.b2": "将地图上所有数字 2（货物）运送到任一数字 4（港口）",
  "rules.task.b3":
    "每个 AGV 一次最多携带 1 件货物：取货后须先送港口卸下，才能取下一件",
  "rules.task.b4":
    "目标是最短完成时间 makespan，即全部货物送达所用的总帧数",
  "rules.task.b5":
    "算法使用时空 A* 与预留表（reservation table），保证 AGV 互不碰撞：不占据同一格、不互换位置",
  "rules.flow.title": "使用流程",
  "rules.flow.s1": "生成随机地图：自动载入为当前地图",
  "rules.flow.s2": "点击「运行算法」上传地图，由后端求解调度方案",
  "rules.flow.s3": "播放动画回放调度过程，可拖动进度条、切换倍速",
  "rules.flow.s4": "导出调度方案 JSON 或逐帧动作 CSV",
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
  "panel.generateHint": "The generated map is loaded automatically",
  "panel.downloadMap": "Download map",

  "panel.mapFile": "Map file",
  "panel.dropHint": "Click to select or drop a CSV map file",
  "panel.noFile": "No file selected",
  "panel.selectedPrefix": "Selected: ",
  "panel.run": "Run algorithm",
  "panel.running": "Running algorithm…",
  "panel.loadDemo": "Load demo data (no backend needed)",

  "run.elapsed": "Elapsed {v} s",
  "run.estimate": "Estimated {min}~{max} s",
  "run.timeoutWarning":
    "This map may exceed the response time limit (>40s) and fail. Consider fewer cargos or a larger map.",

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

  "perf.title": "Performance",
  "perf.elapsed": "Solve time",
  "perf.agv": "AGVs",

  "export.json": "Export schedule (JSON)",
  "export.csv": "Export CSV (per-frame actions)",

  "panel.mapInfoTitle": "Current map",
  "panel.mapInfo":
    "Size {w} × {h} · AGV {agv} · Cargo {cargo} · Port {port}",
  "panel.mapLoaded":
    "Map generated and loaded — you can run the algorithm now",

  "rules.open": "Rules",
  "rules.title": "Rules",
  "rules.back": "← Back to home",
  "rules.map.title": "Map rules",
  "rules.map.b1": "The map is a rectangular grid sized width × height",
  "rules.map.b2":
    "AGVs move one cell at a time in the four cardinal directions",
  "rules.map.b3": "Obstacle cells (1) are impassable",
  "rules.csv.title": "CSV cell values",
  "rules.csv.value": "Value",
  "rules.csv.meaning": "Meaning",
  "rules.csv.v0": "Path node (traversable by AGVs)",
  "rules.csv.v1": "Obstacle node (impassable)",
  "rules.csv.v2": "Cargo node (traversable; must be delivered to a port)",
  "rules.csv.v3": "AGV start position (traversable)",
  "rules.csv.v4": "Port node (traversable; delivery destination)",
  "rules.csv.note": "Values 2 / 3 / 4 are all traversable path nodes.",
  "rules.task.title": "Task rules",
  "rules.task.b1": "Multiple AGVs start from cells marked 3",
  "rules.task.b2": "Deliver every cargo (2) on the map to any port (4)",
  "rules.task.b3":
    "Each AGV carries at most one cargo at a time: after picking up, it must drop the cargo at a port before picking the next one",
  "rules.task.b4":
    "The objective is the shortest makespan — the total number of frames until all cargo is delivered",
  "rules.task.b5":
    "The algorithm uses spatio-temporal A* with a reservation table so AGVs never collide: they never share a cell or swap positions",
  "rules.flow.title": "How to use",
  "rules.flow.s1":
    "Generate a random map: it is loaded as the current map automatically",
  "rules.flow.s2":
    'Click "Run algorithm" to upload the map and let the backend solve the schedule',
  "rules.flow.s3":
    "Play the animation to replay the schedule; seek via the progress bar and change speed",
  "rules.flow.s4": "Export the schedule as JSON or per-frame CSV",
};

const STORAGE_KEY = "spa-lang";
const LANG_EVENT = "spa-lang-change";

// 无存储偏好时使用的语言快照。初始固定为 "zh"，与服务端快照一致，
// 保证水合首帧不报错；mount 后由 applyInitialLang 检测浏览器语言再更新。
let browserFallbackLang: Lang = "zh";

/** 检测浏览器语言偏好：navigator.language 以 zh 开头 → 中文，否则英文 */
function detectBrowserLang(): Lang {
  if (typeof window === "undefined") return "zh";
  return navigator.language.toLowerCase().startsWith("zh") ? "zh" : "en";
}

/** 读取语言：localStorage 手动偏好优先；无偏好时用浏览器语言快照 */
function readLang(): Lang {
  if (typeof window === "undefined") return "zh";
  const stored = window.localStorage.getItem(STORAGE_KEY);
  if (stored === "en" || stored === "zh") return stored;
  return browserFallbackLang;
}

/**
 * mount 后调用一次：无存储偏好时读取浏览器语言并应用。
 * 手动切换写入 localStorage，始终优先于此处检测值。
 */
function applyInitialLang(): void {
  if (window.localStorage.getItem(STORAGE_KEY) === null) {
    browserFallbackLang = detectBrowserLang();
  }
  window.dispatchEvent(new Event(LANG_EVENT));
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
  // 服务端快照固定 "zh"，与客户端首帧（browserFallbackLang 初始值）一致；
  // mount 后 applyInitialLang 按浏览器语言更新并触发重渲染
  const lang = useSyncExternalStore<Lang>(subscribeLang, readLang, () => "zh");

  useEffect(() => {
    applyInitialLang();
  }, []);

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
