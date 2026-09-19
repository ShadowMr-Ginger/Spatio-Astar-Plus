// 动画渲染相关的纯函数：子步时钟、位置插值、货物可见性判定
// 保持无 React / 无路径别名依赖，便于用 node 直接运行验证脚本

import type { AgvState, Cargo, Frame, ScheduleEvent } from "./types";

/** 每个逻辑帧拆分的子步数（一格走 3 步：0 → 1/3 → 2/3 → 1） */
export const SUBSTEPS_PER_FRAME = 3;

export interface Pos {
  x: number;
  y: number;
}

/** 线性插值：位置 a → b，k ∈ [0,1]；b 为空时返回 a（最后一帧不插值） */
export function lerpPos(a: Pos, b: Pos | null | undefined, k: number): Pos {
  if (!b) return { x: a.x, y: a.y };
  return { x: a.x + (b.x - a.x) * k, y: a.y + (b.y - a.y) * k };
}

/** N 个逻辑帧对应的最后一个子步索引（t 从 0 到 N-1） */
export function maxSubStep(frameCount: number): number {
  return frameCount > 0 ? (frameCount - 1) * SUBSTEPS_PER_FRAME : 0;
}

/** 子步索引 → 逻辑帧号 t */
export function frameOf(subStep: number): number {
  return Math.floor(subStep / SUBSTEPS_PER_FRAME);
}

/** 子步索引 → 帧内相位 k ∈ {0,1,2} */
export function phaseOf(subStep: number): number {
  return subStep % SUBSTEPS_PER_FRAME;
}

/** 当前帧中“被搬运”的货物：cargoId → 携带它的 AGV 状态 */
export function carriedMap(frame: Frame | null | undefined): Map<number, AgvState> {
  const map = new Map<number, AgvState>();
  for (const s of frame?.states ?? []) {
    if (s.carrying >= 0) map.set(s.carrying, s);
  }
  return map;
}

/**
 * 已送达（应隐藏）的货物 id 集合，仅统计 t <= frameT 的 drop 事件；
 * 无 events 信息时退回使用 cargos[].delivered 标记
 */
export function droppedCargoIds(
  events: ScheduleEvent[] | undefined,
  cargos: Cargo[] | undefined,
  frameT: number | null
): Set<number> {
  const ids = new Set<number>();
  if (events && events.length > 0) {
    for (const e of events) {
      if (e.type === "drop" && (frameT === null || e.t <= frameT)) ids.add(e.cargo);
    }
    return ids;
  }
  // 无事件信息（如静态预览）：用 delivered 标记兜底；frameT 为 null 表示预览，全部显示
  if (frameT !== null) {
    for (const c of cargos ?? []) {
      if (c.delivered) ids.add(c.id);
    }
  }
  return ids;
}

/** 单件货物的渲染判决 */
export type CargoPlacement =
  | { kind: "at"; cargo: Cargo; pos: Pos } // 静止在原位
  | { kind: "follow"; cargo: Cargo; agv: number; pin: boolean } // 跟随 AGV；pin 表示固定在卸货点
  | { kind: "hidden"; cargo: Cargo }; // 不绘制

/**
 * 货物可见性判定（修复点：pickup 后原位隐藏并跟随 AGV，drop 后保持隐藏）
 * - 当前帧有 AGV 携带它 → follow（坐标取 AGV 当前位置，由调用方插值）；
 *   若 AGV 本帧动作是 drop，则 pin=true，货物固定在卸货位置，不随 AGV 离开
 * - 已 drop（t 之前的 drop 事件）/ delivered → hidden
 * - 其余情况 → 静止在原位
 */
export function resolveCargos(
  cargos: Cargo[] | undefined,
  frame: Frame | null | undefined,
  events: ScheduleEvent[] | undefined
): CargoPlacement[] {
  const carried = carriedMap(frame);
  const dropped = droppedCargoIds(events, cargos, frame ? frame.t : null);
  return (cargos ?? []).map((cargo) => {
    const carrier = carried.get(cargo.id);
    if (carrier) {
      return { kind: "follow", cargo, agv: carrier.agv, pin: carrier.action === "drop" };
    }
    if (dropped.has(cargo.id)) return { kind: "hidden", cargo };
    return { kind: "at", cargo, pos: { x: cargo.x, y: cargo.y } };
  });
}
