// 统一封装对后端 API 的调用（通过 next.config 中的 rewrite 代理到 localhost:5080）

import type { MapGenParams, ScheduleResult } from "./types";

/** 后端返回的错误格式：{ "errors": ["..."] } */
export class ApiError extends Error {
  errors: string[];

  constructor(errors: string[]) {
    super(errors.join("；"));
    this.name = "ApiError";
    this.errors = errors;
  }
}

async function parseError(res: Response): Promise<ApiError> {
  try {
    const data = await res.json();
    if (data && Array.isArray(data.errors) && data.errors.length > 0) {
      return new ApiError(data.errors.map(String));
    }
  } catch {
    // 非 JSON 错误响应，走兜底逻辑
  }
  return new ApiError([`请求失败（HTTP ${res.status}）`]);
}

/** 触发浏览器下载 Blob 文件 */
export function downloadBlob(blob: Blob, filename: string): void {
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}

/** GET /api/maps/generate —— 生成随机地图，返回 CSV 文本（调用方负责下载与载入） */
export async function generateMapCsv(params: MapGenParams): Promise<string> {
  const qs = new URLSearchParams({
    width: String(params.width),
    height: String(params.height),
    agvCount: String(params.agvCount),
    cargoCount: String(params.cargoCount),
    portCount: String(params.portCount),
    obstacleRatio: String(params.obstacleRatio),
  });
  const res = await fetch(`/api/maps/generate?${qs.toString()}`);
  if (!res.ok) throw await parseError(res);
  return res.text();
}

/** POST /api/schedules/run —— 上传 csv 地图并运行调度算法 */
export async function runSchedule(file: File): Promise<ScheduleResult> {
  const form = new FormData();
  form.append("file", file); // 后端约定的文件字段名为 file
  const res = await fetch("/api/schedules/run", {
    method: "POST",
    body: form,
  });
  if (!res.ok) throw await parseError(res);
  return (await res.json()) as ScheduleResult;
}

/** 把调度结果展开为逐帧 CSV 文本（列：frame,agv,x,y,action,carrying） */
export function scheduleToCsv(result: ScheduleResult): string {
  const lines: string[] = ["frame,agv,x,y,action,carrying"];
  for (const frame of result.frames) {
    for (const s of frame.states) {
      lines.push(
        [frame.t, s.agv, s.x, s.y, s.action, s.carrying].join(",")
      );
    }
  }
  return lines.join("\n");
}
