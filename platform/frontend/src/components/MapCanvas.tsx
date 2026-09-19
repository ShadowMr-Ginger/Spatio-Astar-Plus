"use client";

// 地图预览 / 动画播放共用的 Canvas 组件（Canvas 2D 绘制，支持高 DPI 自适应缩放）
// 动画模式下 AGV 按 1/3 子步在相邻两帧间线性插值，被搬运货物跟随 AGV

import { useEffect, useRef } from "react";
import { lerpPos, resolveCargos } from "@/lib/render";
import type { Cargo, Frame, ScheduleEvent } from "@/lib/types";

interface MapCanvasProps {
  width: number;
  height: number;
  grid: number[][];
  cargos?: Cargo[];
  /** 调度事件（pickup/drop），用于判定货物是否已送达隐藏 */
  events?: ScheduleEvent[];
  /** 当前逻辑帧 t；为 null 时仅绘制静态地图 */
  frame?: Frame | null;
  /** 下一逻辑帧 t+1，用于插值；末帧传 null */
  nextFrame?: Frame | null;
  /** 帧内子步相位 k ∈ {0,1,2}，插值比例 k/3 */
  phase?: number;
}

// 图例配色（与页面底部图例一致）
const COLORS = {
  cell: "#ffffff",
  cellStroke: "#e2e8f0",
  obstacle: "#475569",
  cargo: "#f59e0b",
  port: "#10b981",
  agv: "#2563eb",
  agvCarry: "#7c3aed",
};

export default function MapCanvas({
  width,
  height,
  grid,
  cargos,
  events,
  frame = null,
  nextFrame = null,
  phase = 0,
}: MapCanvasProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const wrapRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const wrap = wrapRef.current;
    if (!canvas || !wrap) return;

    const draw = () => {
      const ctx = canvas.getContext("2d");
      if (!ctx) return;

      // 按 devicePixelRatio 缩放，保证高 DPI 屏下画面清晰
      const dpr = window.devicePixelRatio || 1;
      const cw = wrap.clientWidth;
      const ch = wrap.clientHeight;
      canvas.width = Math.round(cw * dpr);
      canvas.height = Math.round(ch * dpr);
      canvas.style.width = `${cw}px`;
      canvas.style.height = `${ch}px`;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, cw, ch);

      // 保持格子为正方形，并在画布内居中
      const pad = 4;
      const cell = Math.max(
        2,
        Math.min((cw - pad * 2) / width, (ch - pad * 2) / height)
      );
      const ox = (cw - cell * width) / 2;
      const oy = (ch - cell * height) / 2;

      // 1. 绘制基础格子与障碍
      for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
          const v = grid[y]?.[x] ?? 0;
          ctx.fillStyle = v === 1 ? COLORS.obstacle : COLORS.cell;
          ctx.fillRect(ox + x * cell, oy + y * cell, cell, cell);
          ctx.strokeStyle = COLORS.cellStroke;
          ctx.lineWidth = 1;
          ctx.strokeRect(ox + x * cell, oy + y * cell, cell, cell);
        }
      }

      const px = (x: number) => ox + x * cell + cell / 2;
      const py = (y: number) => oy + y * cell + cell / 2;

      // 2. 绘制港口
      for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
          if (grid[y]?.[x] !== 4) continue;
          ctx.fillStyle = COLORS.port;
          const r = cell * 0.32;
          ctx.fillRect(px(x) - r, py(y) - r, r * 2, r * 2);
        }
      }

      // 3. 计算每个 AGV 的插值后位置（帧 t 坐标 + (t+1 - t) × k/3）
      const k = phase / 3;
      const agvPos = new Map<number, { x: number; y: number }>();
      for (const s of frame?.states ?? []) {
        const next = nextFrame?.states.find((n) => n.agv === s.agv);
        agvPos.set(s.agv, lerpPos(s, next, k));
      }

      // 4. 绘制货物：静止原位 / 跟随 AGV / 已送达隐藏
      if (cargos && cargos.length > 0) {
        for (const item of resolveCargos(cargos, frame, events)) {
          if (item.kind === "at") {
            drawCargo(ctx, px(item.pos.x), py(item.pos.y), cell * 0.28);
          } else if (item.kind === "follow") {
            if (item.pin) {
              // 卸货帧：货物固定在卸货位置，不随 AGV 插值离开
              const carrier = frame?.states.find((st) => st.agv === item.agv);
              if (carrier) {
                drawCargo(ctx, px(carrier.x), py(carrier.y) - cell * 0.42, cell * 0.2);
              }
            } else {
              const pos = agvPos.get(item.agv);
              if (pos) {
                // 跟随货物悬浮在 AGV 上方，随插值坐标移动
                drawCargo(ctx, px(pos.x), py(pos.y) - cell * 0.42, cell * 0.2);
              }
            }
          }
        }
      } else {
        // 无货物清单时（静态预览）直接按网格值绘制
        for (let y = 0; y < height; y++) {
          for (let x = 0; x < width; x++) {
            if (grid[y]?.[x] !== 2) continue;
            drawCargo(ctx, px(x), py(y), cell * 0.28);
          }
        }
      }

      // 5. 绘制 AGV（携带货物时用紫色并加角标）
      for (const s of frame?.states ?? []) {
        const pos = agvPos.get(s.agv);
        if (!pos) continue;
        const cx = px(pos.x);
        const cy = py(pos.y);
        const r = cell * 0.36;
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.fillStyle = s.carrying >= 0 ? COLORS.agvCarry : COLORS.agv;
        ctx.fill();
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = 2;
        ctx.stroke();

        // AGV 编号
        if (cell >= 12) {
          ctx.fillStyle = "#ffffff";
          ctx.font = `bold ${Math.max(8, cell * 0.32)}px sans-serif`;
          ctx.textAlign = "center";
          ctx.textBaseline = "middle";
          ctx.fillText(String(s.agv), cx, cy);
        }

        // 携带货物角标：右上角小圆点
        if (s.carrying >= 0) {
          ctx.beginPath();
          ctx.arc(cx + r * 0.85, cy - r * 0.85, cell * 0.14, 0, Math.PI * 2);
          ctx.fillStyle = COLORS.cargo;
          ctx.fill();
          ctx.strokeStyle = "#ffffff";
          ctx.lineWidth = 1.5;
          ctx.stroke();
        }
      }
    };

    draw();
    // 监听容器尺寸变化，自动重绘
    const observer = new ResizeObserver(draw);
    observer.observe(wrap);
    return () => observer.disconnect();
  }, [width, height, grid, cargos, events, frame, nextFrame, phase]);

  return (
    <div ref={wrapRef} className="relative w-full h-[480px]">
      <canvas ref={canvasRef} className="block" />
    </div>
  );
}

function drawCargo(
  ctx: CanvasRenderingContext2D,
  cx: number,
  cy: number,
  r: number
) {
  ctx.beginPath();
  ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.fillStyle = COLORS.cargo;
  ctx.fill();
  ctx.strokeStyle = "#ffffff";
  ctx.lineWidth = 1.5;
  ctx.stroke();
}
