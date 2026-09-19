// 地图 CSV 的解析与序列化：文件上传与"生成后自动载入"共用
// 保持纯函数、无浏览器 API（除 csvToFile 外），便于 node 脚本直接验证

/** 解析后的地图数据（含从网格统计的元素计数） */
export interface ParsedMap {
  width: number;
  height: number;
  grid: number[][];
  agvCount: number;
  cargoCount: number;
  portCount: number;
  /** 可通行格子数（值 0/2/3/4，即非障碍） */
  openCount: number;
}

/**
 * 解析 CSV 文本为地图网格。
 * 格式：height 行、每行 width 个逗号分隔的 0-4 数字，无表头。
 * 格式非法时抛出异常（调用方负责本地化错误提示）。
 */
export function parseMapCsv(text: string): ParsedMap {
  const grid = text
    .trim()
    .split(/\r?\n/)
    .map((line) => line.split(",").map(Number));
  const valid =
    grid.length > 0 &&
    grid.every(
      (row) =>
        row.length === grid[0].length &&
        row.every((v) => Number.isInteger(v) && v >= 0 && v <= 4)
    );
  if (!valid) {
    throw new Error("invalid map csv");
  }
  let agvCount = 0;
  let cargoCount = 0;
  let portCount = 0;
  let openCount = 0;
  for (const row of grid) {
    for (const v of row) {
      if (v === 1) continue;
      openCount++;
      if (v === 2) cargoCount++;
      else if (v === 3) agvCount++;
      else if (v === 4) portCount++;
    }
  }
  return { width: grid[0].length, height: grid.length, grid, agvCount, cargoCount, portCount, openCount };
}

/** 网格序列化为 CSV 文本（行尾 \n，无表头） */
export function gridToCsv(grid: number[][]): string {
  return grid.map((row) => row.join(",")).join("\n");
}

/** 由 CSV 文本构造 File 对象（用于"生成后自动载入"时上传后端） */
export function csvToFile(text: string, name: string): File {
  return new File([text], name, { type: "text/csv" });
}
