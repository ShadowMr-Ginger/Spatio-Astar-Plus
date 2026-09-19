// 最小可用的演示调度数据，用于后端未启动时验证前端渲染与播放逻辑
// 场景：8x6 地图，1 台 AGV 从 (5,4) 取货后运到港口 (0,4)

import type { ScheduleResult } from "./types";

export function getMockSchedule(): ScheduleResult {
  const grid: number[][] = [
    [0, 0, 0, 0, 0, 0, 0, 0],
    [0, 1, 1, 0, 0, 1, 1, 0],
    [0, 0, 0, 0, 0, 0, 0, 0],
    [0, 1, 0, 0, 0, 0, 1, 0],
    [4, 0, 0, 2, 0, 3, 0, 0],
    [0, 0, 0, 0, 0, 0, 2, 0],
  ];

  // AGV 路径：(5,4) → (4,4) → (3,4) 取货 → (2,4) → (1,4) → (0,4) 卸货
  const path: Array<[number, number, "move" | "wait" | "pickup" | "drop", number]> = [
    [5, 4, "wait", -1],
    [4, 4, "move", -1],
    [3, 4, "pickup", 0],
    [2, 4, "move", 0],
    [1, 4, "move", 0],
    [0, 4, "drop", -1],
    [0, 4, "wait", -1],
    [0, 4, "wait", -1],
  ];

  return {
    width: 8,
    height: 6,
    grid,
    agvs: [{ id: 0, startX: 5, startY: 4 }],
    cargos: [
      { id: 0, x: 3, y: 4, delivered: true },
      { id: 1, x: 6, y: 5, delivered: false },
    ],
    ports: [{ id: 0, x: 0, y: 4 }],
    frames: path.map(([x, y, action, carrying], t) => ({
      t,
      states: [{ agv: 0, x, y, action, carrying }],
    })),
    events: [
      { t: 2, type: "pickup", agv: 0, cargo: 0 },
      { t: 5, type: "drop", agv: 0, cargo: 0 },
    ],
    stats: { makespan: 7, totalMoves: 4, pickups: 1, deliveries: 1 },
  };
}
