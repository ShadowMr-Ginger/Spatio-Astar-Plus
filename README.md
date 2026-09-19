# Spatio A* Plus — 多 AGV 时空 A* 调度算法与演示平台

基于**时空 A\***（Spatio-Temporal A\*）的多 AGV 路径规划与货物调度算法，以及一个可在线运行的全栈演示平台。算法原型为 C++ 竞赛代码（华为软件精英挑战赛港口调度赛道），本仓库将其简化、移植为 C# 调度引擎，并配套 Next.js 前端实现地图生成 → 上传 → 调度求解 → 动画回放 → 方案导出的完整流程。

**Demo（即将上线）：https://jianqiaoxu.xyz/spatio-astar**

---

## 功能特性

- **一键生成随机地图**：按密度参数生成保证连通、合法的任务地图（CSV），自动载入预览，可手动下载
- **在线运行算法**：上传 CSV 地图，后端求解多 AGV 协同调度方案
- **调度动画播放器**：Canvas 渲染逐帧方案，1/3 子步插值丝滑移动，支持播放/暂停/逐帧/进度拖动/0.5x–4x 倍速
- **相机交互**：滚轮以鼠标为焦点缩放（0.5x–8x）、拖拽平移、双击复位
- **性能展示**：求解预估耗时（X~Y 秒）、实时计时器、求解结果统计（makespan/步数/送达率/求解毫秒数）
- **方案导出**：调度方案 JSON、逐帧动作 CSV
- **中英双语**：默认跟随浏览器语言，可手动切换
- **规则介绍页**：地图格式、CSV 各数字含义、任务规则说明

## 地图格式（CSV）

矩形网格，无表头，逗号分隔：

| 数值 | 含义 | AGV 可达 |
|------|------|----------|
| 0 | 路径节点 | ✔ |
| 1 | 障碍节点 | ✘ |
| 2 | 货物节点 | ✔ |
| 3 | AGV 初始位置 | ✔ |
| 4 | 港口节点 | ✔ |

**任务规则**：多个 AGV 从各自的 3 出发，协同将全部 2 货物运送至任一 4 港口；每个 AGV 同时最多携带 1 件货物（取货后须先送港口卸下才能取下一个）；优化目标为最短完成时间（makespan）。

## 技术栈

| 层 | 技术 |
|----|------|
| 算法引擎 | C# (.NET 8)，时空 A\* + 预留表冲突避免，动态任务分配 |
| 后端 | ASP.NET Core 8 Web API，Controllers / Services / Core 分层 |
| 前端 | Next.js 16 + React 19 + Tailwind CSS v4 + Canvas 2D |
| 测试 | xUnit（27+ 用例） |

## 快速开始

前置：.NET SDK 8、Node.js 18+

```bash
# 1. 启动后端（端口 5080）
cd platform/backend/src/SpatioAstar.Api
dotnet run --urls http://localhost:5080

# 2. 启动前端（端口 3000，/api 已配置代理到 5080）
cd platform/frontend
npm install
npm run dev
```

浏览器打开 http://localhost:3000 即可体验完整流程。

## API 一览

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/maps/generate?width=&height=&agvCount=&cargoCount=&portCount=&obstacleRatio=` | 生成随机合法地图，返回 CSV 附件 |
| POST | `/api/schedules/run`（multipart，字段 `file`） | 上传地图执行调度，返回逐帧方案 JSON |
| GET | `/api/schedules/estimate?cargoCount=&openCount=&agvCount=` | 预估求解耗时区间 `{minMs, maxMs}` |

错误统一返回 400 + `{ "errors": ["..."] }`，并区分静态不可达 / 求解预算熔断 / 冲突不可行三类原因。

## 算法说明

### 时空 A\* 与冲突避免

- 搜索状态为 `(x, y, t)` 三维，支持四向移动与原地等待，启发式采用多源 BFS 距离场（可采纳且带静态死路剪枝）
- **预留表（Reservation Table）**：已规划路径按时间占用顶点与有向边；后加入规划的 AGV 自动避让前者，杜绝同点冲突与同帧对向互换
- AGV 起点格自激活起对他人屏蔽，构造性消除返场停靠冲突

### 动态任务分配

- 双策略组合择优（共 16 种组合取 makespan 最小）：
  - **DynamicNearest**：每轮将货物分配给"最早空闲"的 AGV，干得快者持续领新任务，负载天然均衡
  - **StaticQuota**：静态配额基准策略，作为兜底
- 实测 DynamicNearest 使 makespan 较静态分配下降 16%–43%
- 求解预算护栏：200 万次扩展为主上限（与系统负载无关），50s 硬熔断，病态地图确定性退出

### 求解性能

| 场景 | 端到端耗时 |
|------|-----------|
| 30×30 / 100 货物 / 10 AGV（高密度） | ≈ 3.8s |
| 60×60 / 100 货物 / 10 AGV | ≈ 0.85s |

## 目录结构

```
├── main.cpp                  # 原始 C++ 竞赛算法（最终版）
├── 往期版本/                  # 历届演进版本存档
└── platform/
    ├── backend/
    │   ├── src/SpatioAstar.Core/    # 算法引擎（地图解析/生成、时空 A*、调度器）
    │   ├── src/SpatioAstar.Api/     # ASP.NET Core Web API
    │   └── tests/SpatioAstar.Core.Tests/
    └── frontend/
        ├── src/app/                 # 主页 + /rules 规则页
        ├── src/components/          # 画布、播放器、控制面板
        └── src/lib/                 # API 封装、i18n、地图 CSV、渲染工具
```

## 测试

```bash
cd platform/backend && dotnet test
```

覆盖：地图解析/校验、生成器有效性（连通性与节点计数）、调度正确性（全送达、无顶点/互换冲突、帧序列完整、携带状态一致）、失败语义、性能预算。

## 历史

本项目算法原型为 2024 年华为软件精英挑战赛港口智能调度赛题代码（单文件 C++，含货物价值/时效与轮船泊位调度）。演示平台去掉了价值/时效/船舶，回归纯路径规划问题，核心时空 A\* 冲突避免机制完整保留并移植至 C#。
