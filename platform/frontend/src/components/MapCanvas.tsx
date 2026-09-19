"use client";

// 地图预览 / 动画播放共用的 Canvas 组件（Canvas 2D 绘制，支持高 DPI 自适应缩放）

import { useEffect, useRef } from "react";
import type { Cargo, Frame } from "@/lib/types";

interface MapCanvasProps {
  width: number;
  height: number;
  grid: number[][];
  cargos?: Cargo[];
  /** 当前帧；为 null 时仅绘制静态地图 */
  frame?: Frame | null;
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
  frame = null,
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

      // 3. 绘制货物（被 AGV 搬运中的货物隐藏）
      const carried = new Set(
        frame?.states.filter((s) => s.carrying >= 0).map((s) => s.carrying) ??
          []
      );
      if (cargos && cargos.length > 0) {
        for (const c of cargos) {
          if (carried.has(c.id)) continue;
          drawCargo(ctx, px(c.x), py(c.y), cell);
        }
      } else {
        for (let y = 0; y < height; y++) {
          for (let x = 0; x < width; x++) {
            if (grid[y]?.[x] !== 2) continue;
            drawCargo(ctx, px(x), py(y), cell);
          }
        }
      }

      // 4. 绘制 AGV（携带货物时用紫色并加角标）
      for (const s of frame?.states ?? []) {
        const r = cell * 0.36;
        ctx.beginPath();
        ctx.arc(px(s.x), py(s.y), r, 0, Math.PI * 2);
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
          ctx.fillText(String(s.agv), px(s.x), py(s.y));
        }

        // 携带货物角标：右上角小圆点
        if (s.carrying >= 0) {
          ctx.beginPath();
          ctx.arc(px(s.x) + r * 0.85, py(s.y) - r * 0.85, cell * 0.14, 0, Math.PI * 2);
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
  }, [width, height, grid, cargos, frame]);

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
  cell: number
) {
  const r = cell * 0.28;
  ctx.beginPath();
  ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.fillStyle = COLORS.cargo;
  ctx.fill();
  ctx.strokeStyle = "#ffffff";
  ctx.lineWidth = 1.5;
  ctx.stroke();
}
