// 与后端 API 契约对应的数据类型定义

/** AGV 初始位置 */
export interface AgvStart {
  id: number;
  startX: number;
  startY: number;
}

/** 货物 */
export interface Cargo {
  id: number;
  x: number;
  y: number;
  delivered: boolean;
}

/** 港口 */
export interface Port {
  id: number;
  x: number;
  y: number;
}

export type AgvAction = "move" | "wait" | "pickup" | "drop";

/** 某一帧中单个 AGV 的状态 */
export interface AgvState {
  agv: number;
  x: number;
  y: number;
  action: AgvAction;
  /** 正在搬运的货物 id，-1 表示空载 */
  carrying: number;
}

/** 一帧调度快照，t 从 0 到 makespan */
export interface Frame {
  t: number;
  states: AgvState[];
}

export type ScheduleEventType = "pickup" | "drop";

/** 关键事件（取货 / 卸货） */
export interface ScheduleEvent {
  t: number;
  type: ScheduleEventType;
  agv: number;
  cargo: number;
}

/** 调度统计信息 */
export interface ScheduleStats {
  makespan: number;
  totalMoves: number;
  pickups: number;
  deliveries: number;
  /** 算法求解耗时（ms）；旧版后端可能缺失 */
  elapsedMs?: number;
}

/** POST /api/schedules/run 的完整返回 */
export interface ScheduleResult {
  width: number;
  height: number;
  grid: number[][];
  agvs: AgvStart[];
  cargos: Cargo[];
  ports: Port[];
  frames: Frame[];
  events: ScheduleEvent[];
  stats: ScheduleStats;
}

/** 地图生成参数 */
export interface MapGenParams {
  width: number;
  height: number;
  agvCount: number;
  cargoCount: number;
  portCount: number;
  obstacleRatio: number;
}

/** 当前已载入地图的信息（展示于控制面板） */
export interface LoadedMapInfo {
  name: string;
  width: number;
  height: number;
  agvCount: number;
  cargoCount: number;
  portCount: number;
}

/** GET /api/schedules/estimate 的查询参数 */
export interface EstimateParams {
  cargoCount: number;
  openCount: number;
  agvCount: number;
}

/** GET /api/schedules/estimate 的返回：求解耗时预估区间（ms） */
export interface EstimateResult {
  minMs: number;
  maxMs: number;
}
