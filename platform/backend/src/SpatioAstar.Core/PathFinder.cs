using System.Diagnostics;
using SpatioAstar.Core.Models;

namespace SpatioAstar.Core;

/// <summary>路径中的单步：所在格与该步动作（首步恒为 Wait，由规划方按需改写末步为 Pickup/Drop）。</summary>
public sealed record PathStep(int X, int Y, AgvAction Action);

/// <summary>
/// 一条已规划的路径。<see cref="Steps"/>[j] 表示时刻 <see cref="StartT"/> + j 所在格与动作，
/// 因此路径在时间段内逐帧占用了这些顶点。
/// </summary>
public sealed class PlannedPath
{
    public required int AgvId { get; init; }
    public required int StartT { get; init; }
    public required IReadOnlyList<PathStep> Steps { get; init; }
    public int EndT => StartT + Steps.Count - 1;
    public (int X, int Y) EndCell => (Steps[^1].X, Steps[^1].Y);
}

/// <summary>
/// 调度失败类别：把"真无解"和"求解预算不足"区分开，用户能据此判断是地图问题还是求解器原因。
/// </summary>
public enum ScheduleFailureKind
{
    /// <summary>
    /// 静态不可达：货物/港口与任何 AGV 起点不在同一四连通区域，
    /// 这类问题在 <see cref="MapValidator"/> 层面即可判定，与调度算法无关。
    /// </summary>
    Unreachable,

    /// <summary>
    /// 搜索预算熔断：单条搜索触达扩展次数或墙钟上限而中途放弃。
    /// 这是求解器的资源限制，<b>不代表地图无解</b>；放宽预算或降低节点密度后可能成功。
    /// </summary>
    BudgetExhausted,

    /// <summary>
    /// 冲突不可行：在预留表约束与时间窗内穷举了全部可达的时空状态仍未找到无冲突路径，
    /// 或未规划 AGV 的终点被他人永久堵住。属于"在当前规划顺序下被堵死"，换序重试可能化解。
    /// </summary>
    ConflictInfeasible,
}

/// <summary>调度求解失败时抛出，消息为面向用户的中文描述，并携带失败类别。</summary>
public sealed class SchedulerException : Exception
{
    /// <summary>失败类别，用于区分"地图无解"与"求解超时"。</summary>
    public ScheduleFailureKind Kind { get; }

    public SchedulerException(string message, ScheduleFailureKind kind = ScheduleFailureKind.ConflictInfeasible)
        : base(message)
    {
        Kind = kind;
    }
}

/// <summary>
/// 时空预留表（Reservation Table）：记录已经规划好的 AGV 路径对未来时空资源的占用，
/// 是实现多 AGV 无冲突调度的核心数据结构。按 AGV 顺序串行规划，
/// 后加入的 AGV 通过查询本表避让先规划的路径，因此天然保证两两无冲突。
///
/// 三类预留：
/// 1. 顶点预留：某 AGV 在时刻 t 占据格子 (x,y) —— 防止同点冲突（顶点冲突）；
/// 2. 有向边预留：某 AGV 在 t-1→t 从 a 移动到 b —— 配合反向查询防止边冲突（互换位置对撞）；
/// 3. 静止预留：完成全部任务的 AGV 从其最后一帧起永久占据终点格；
/// 4. 待定起点：尚未开始规划的 AGV 的起点，对其他 AGV 而言任何时刻都视为障碍
///    （该 AGV 可能一直停在那里），直到它被激活（开始规划）时移除。
/// </summary>
public sealed class ReservationTable
{
    // 顶点预留键：t 高 32 位、y 中 16 位、x 低 16 位。
    // 要求地图宽/高 < 65536（MapParser 已限 10000），单 long 键比三元组字典快数倍——
    // 冲突检查是 A* 内层最热的路径，常数因子直接决定稠密地图的求解速度。
    private const int CoordBits = 16;
    private const long CoordMask = 0xFFFF;

    private static long VertexKey(int x, int y, int t) =>
        ((long)t << (CoordBits * 2)) | ((long)(uint)y << CoordBits) | (uint)x;

    private readonly Dictionary<long, int> _vertices = new();
    private readonly HashSet<(int X1, int Y1, int X2, int Y2, int T)> _edges = new();
    /// <summary>
    /// 静止预留：AGV 完成一次卸货后从其最后一帧起占据该格，直到它被分配下一个任务。
    /// 每个 AGV 至多一条记录，重复调用覆盖旧记录（旧终点格随之释放）。
    /// 这样"停在港口的 AGV"对后续规划者始终是可见的永久障碍，不会暗中产生冲突。
    /// </summary>
    private readonly Dictionary<int, (int X, int Y, int FromT)> _resting = new();

    /// <summary>格子打包：与 VertexKey 的坐标部分同构，避免元组键的哈希开销。</summary>
    private static long CellKey(int x, int y) => ((long)(uint)y << 32) | (uint)x;

    /// <summary>静止预留的反向索引：格子 → AGV id，把 IsVertexFree 里的线性扫描降为 O(1)。</summary>
    private readonly Dictionary<long, int> _restingByCell = new();
    private readonly HashSet<long> _pendingStarts = new();

    /// <summary>登记一个尚未规划路径的 AGV 起点：其他 AGV 在任何时刻都不得进入该格。</summary>
    public void AddPendingStart(int x, int y) => _pendingStarts.Add(CellKey(x, y));

    /// <summary>该格是否是某个尚未规划 AGV 的待定起点（全时段障碍）。</summary>
    public bool IsPendingStart(int x, int y) => _pendingStarts.Contains(CellKey(x, y));

    /// <summary>
    /// 目标格被他人静止停靠堵死的起始时刻：t ≥ 返回值时该格被永久占用，无需再扫描。
    /// 返回 int.MaxValue 表示无静止占用。用于把快速失败循环从整窗扫描降为短扫描。
    /// </summary>
    public int RestingBlockStart(int x, int y, int agvId)
    {
        if (_restingByCell.TryGetValue(CellKey(x, y), out var restingAgv) && restingAgv != agvId)
            return _resting[restingAgv].FromT;
        return int.MaxValue;
    }

    /// <summary>
    /// 判断 agvId 在时刻 t 能否占据 (x,y)。
    /// 检查顶点预留、静止预留与待定起点；本 AGV 自己的预留不构成冲突。
    /// </summary>
    public bool IsVertexFree(int x, int y, int t, int agvId)
    {
        if (_vertices.TryGetValue(VertexKey(x, y, t), out var owner) && owner != agvId)
            return false;

        // 静止预留 O(1) 查询：该格被他人从 FromT 起常驻占用即冲突
        if (_restingByCell.TryGetValue(CellKey(x, y), out var restingAgv))
        {
            if (restingAgv != agvId && _resting[restingAgv].FromT <= t)
                return false;
        }

        // 起点待定说明该 AGV 还没规划，可能一直停在此处；但搜索时不会查到自己（已先移除）
        return !_pendingStarts.Contains(CellKey(x, y));
    }

    /// <summary>
    /// 判断 agvId 在 t-1→t 从 (x1,y1) 移动到 (x2,y2) 是否存在边冲突。
    /// 边冲突即"互换位置"：他人同刻从 (x2,y2) 移动到 (x1,y1)。原地等待无此问题。
    /// </summary>
    public bool IsEdgeFree(int x1, int y1, int x2, int y2, int t, int agvId)
    {
        // 反向边被占用即构成对撞；正向边被自己重复预留是允许的
        return !_edges.Contains((x2, y2, x1, y1, t));
    }

    /// <summary>
    /// AGV 完成一次卸货后调用：从其最后一帧起占据 (x,y)，直到被分配下一个任务（届时覆盖本记录）。
    /// 对后续规划的所有时刻生效。支持回滚。
    /// </summary>
    public void ReserveResting(int agvId, int x, int y, int fromT)
    {
        var hadValue = _resting.TryGetValue(agvId, out var oldValue);
        var newKey = CellKey(x, y);
        var hadCell = _restingByCell.TryGetValue(newKey, out var priorCellAgv);
        // 覆盖旧记录时旧终点格随之释放：先清掉旧格上的映射，再写新格
        var oldCell = hadValue ? (X: oldValue.X, Y: oldValue.Y) : (X: 0, Y: 0);
        var hadOldCell = hadValue && (oldValue.X != x || oldValue.Y != y);
        var oldCellHadMapping = false;
        var oldCellPriorAgv = -1;
        if (hadOldCell)
            oldCellHadMapping = _restingByCell.TryGetValue(CellKey(oldCell.X, oldCell.Y), out oldCellPriorAgv);
        if (hadOldCell) _restingByCell.Remove(CellKey(oldCell.X, oldCell.Y));
        _resting[agvId] = (x, y, fromT);
        _restingByCell[newKey] = agvId;
        _undoLog.Add(() =>
        {
            if (hadValue) _resting[agvId] = oldValue;
            else _resting.Remove(agvId);
            if (oldCellHadMapping) _restingByCell[CellKey(oldCell.X, oldCell.Y)] = oldCellPriorAgv;
            else if (hadOldCell) _restingByCell.Remove(CellKey(oldCell.X, oldCell.Y));
            if (hadCell) _restingByCell[newKey] = priorCellAgv;
            else _restingByCell.Remove(newKey);
        });
    }



    // ---- 批量回滚支持：一次任务分配（取货+送货两段路径）要么全部写入预留表，要么全部撤销 ----

    private readonly List<Action> _undoLog = new();

    /// <summary>记录回滚点，返回其句柄。</summary>
    public int Checkpoint() => _undoLog.Count;

    /// <summary>回滚到指定回滚点，撤销之后的全部预留变更。</summary>
    public void Rollback(int checkpoint)
    {
        for (var i = _undoLog.Count - 1; i >= checkpoint; i--)
            _undoLog[i]();
        _undoLog.RemoveRange(checkpoint, _undoLog.Count - checkpoint);
    }

    /// <summary>AGV 开始规划第一条路径时调用：其起点不再对他人构成全时段障碍（改由顶点预留精确描述）。支持回滚。</summary>
    public void RemovePendingStart(int x, int y)
    {
        var key = CellKey(x, y);
        _pendingStarts.Remove(key);
        _undoLog.Add(() => _pendingStarts.Add(key));
    }

    /// <summary>将一条已规划路径写入预留表：逐帧预留顶点，移动步额外预留有向边。支持回滚。</summary>
    public void ReservePath(PlannedPath path)
    {
        for (var j = 0; j < path.Steps.Count; j++)
        {
            var step = path.Steps[j];
            var t = path.StartT + j;
            var key = VertexKey(step.X, step.Y, t);
            var hadValue = _vertices.TryGetValue(key, out var oldValue);
            _vertices[key] = path.AgvId;
            _undoLog.Add(() =>
            {
                if (hadValue) _vertices[key] = oldValue;
                else _vertices.Remove(key);
            });

            if (j > 0)
            {
                var prev = path.Steps[j - 1];
                if (prev.X != step.X || prev.Y != step.Y)
                {
                    var edge = (prev.X, prev.Y, step.X, step.Y, t);
                    _edges.Add(edge);
                    _undoLog.Add(() => _edges.Remove(edge));
                }
            }
        }
    }
}

/// <summary>
/// 带预留表的时空 A*：状态为 (x, y, t) 三维——相比普通 A* 多了时间维，
/// 同一格子在不同时刻是不同的状态，因此 AGV 可以通过"等待"错开占用时刻来化解冲突。
///
/// 每一步扩展四方向移动或原地等待（等待同样消耗 1 帧，且其目标顶点也要做冲突检查，
/// 因为别人可能移动到 AGV 所在格）。启发函数为曼哈顿距离（可采纳且一致）。
/// 冲突判定完全委托给 <see cref="ReservationTable"/>：
/// 候选顶点在 t 被他人占用 → 顶点冲突，转移不可用；
/// 候选移动与他人 t-1→t 的移动构成反向边 → 边冲突（互换位置），转移不可用。
///
/// 按 AGV 串行规划（先规划的路径先占坑），后加入者通过查询预留表避让，
/// 因此最终所有路径两两无冲突，无需事后调解。
/// </summary>
public static class PathFinder
{
    private static readonly (int Dx, int Dy)[] Directions = { (1, 0), (-1, 0), (0, 1), (0, -1) };

    /// <summary>
    /// 为 agvId 从 (startX,startY,startT) 规划到 (goalX,goalY) 的无冲突时空路径。
    /// </summary>
    /// <param name="timeLimit">搜索的时间上限；超过后抛出 <see cref="SchedulerException"/>。</param>
    /// <param name="distToGoal">
    /// 可选的静态距离启发值：dist[y,x] 表示 (x,y) 到目标的纯行走距离（多源 BFS，不可达为 -1）。
    /// 网格四向对称，以目标为源做 BFS 即得。比曼哈顿距离更紧且仍可采纳（等待只会更慢），
    /// 障碍密集地图可大幅削减扩展数；同时为 -1 的状态可作静态死路直接剪枝。
    /// </param>
    public static PlannedPath Find(
        GridMap map,
        ReservationTable table,
        int agvId,
        int startX,
        int startY,
        int startT,
        int goalX,
        int goalY,
        int timeLimit,
        int[,]? distToGoal = null)
    {
        // 搜索窗收紧：无冲突路径若存在，极少需要超过"全图格数"量级的等待；
        // 失败穷举的状态空间随窗长线性膨胀，收紧后失败成本近减半。
        // 成功路径几乎不会触顶；真被误杀的极端长等待场景由外层换策略/换序重试兜底。
        var tCap = Math.Min(timeLimit, startT + map.Width * map.Height + 256);
        // 静止停靠是永久占用：goal 被他人停靠堵死时只需扫到停靠起点之前，
        // 把 O(窗口) 的快速失败循环降为 O(实际占用帧数)。
        var scanCap = Math.Min(tCap, table.RestingBlockStart(goalX, goalY, agvId) - 1);
        var goalEverFree = false;
        if (scanCap >= startT && !table.IsPendingStart(goalX, goalY))
        {
            for (var t = startT; t <= scanCap; t++)
            {
                if (table.IsVertexFree(goalX, goalY, t, agvId))
                {
                    goalEverFree = true;
                    break;
                }
            }
        }

        if (!goalEverFree)
        {
            throw new SchedulerException(
                $"AGV {agvId} 路径规划失败（{KindText(ScheduleFailureKind.ConflictInfeasible)}）：" +
                $"目标 ({goalX},{goalY}) 在整个时间窗内均被预留占用（可能被已完成 AGV 的静止停靠或待规划 AGV 的起点永久堵住）",
                ScheduleFailureKind.ConflictInfeasible);
        }

        // 时空状态编码为单个 int：id = (t * height + y) * width + x。
        // 比 (x,y,t) 元组字典快数倍——稠密地图的深等待搜索依赖这个常数因子。
        // 每条边的代价恒为 1，因此状态的 g 值恒等于其 t，记录父指针即完成访问标记。
        var width = map.Width;
        var height = map.Height;
        var tStride = width * height;
        int Encode(int x, int y, int t) => t * tStride + y * width + x;
        var open = new PriorityQueue<int, (int F, int H, int Seq)>();
        var parent = new Dictionary<int, int>(1 << 10);
        var seq = 0;
        var expansions = 0;
        var findSw = Stopwatch.StartNew();

        var startId = Encode(startX, startY, startT);
        parent[startId] = -1;
        var startH = distToGoal?[startY, startX] is >= 0 ? distToGoal[startY, startX] : Heuristic(startX, startY, goalX, goalY);
        open.Enqueue(startId, (startT + startH, startH, seq++));

        while (open.Count > 0)
        {
            var curId = open.Dequeue();
            var curX = curId % width;
            var curY = curId / width % height;
            var curT = curId / tStride;
            if (curX == goalX && curY == goalY)
                return Reconstruct(curId, parent, agvId, startT, width, height);

            var nextT = curT + 1;
            if (nextT > tCap)
                continue; // 超出搜索窗的扩展直接丢弃，队列耗尽时统一报失败

            // 搜索规模保护：为证明"不可行"而穷举超大时空状态空间代价极高，超过预算后快速失败。
            // 主上限是扩展次数（确定性、与机器负载无关）；墙钟只作兜底防挂死。
            // 两者都报 BudgetExhausted——这是预算熔断，不代表地图无解。
            if (++expansions > MaxExpansions)
            {
                throw new SchedulerException(
                    $"AGV {agvId} 路径规划失败（{KindText(ScheduleFailureKind.BudgetExhausted)}）：" +
                    $"搜索超过扩展次数上限（{MaxExpansions:N0} 次）而熔断，属于求解预算原因，不代表地图无解；" +
                    $"可降低货物/AGV 密度或稍后重试",
                    ScheduleFailureKind.BudgetExhausted);
            }

            if (findSw.Elapsed > MaxFindTime)
            {
                throw new SchedulerException(
                    $"AGV {agvId} 路径规划失败（{KindText(ScheduleFailureKind.BudgetExhausted)}）：" +
                    $"搜索超过单条墙钟上限（{MaxFindTime.TotalSeconds:N0} 秒）而熔断，属于求解预算原因，不代表地图无解；" +
                    $"可降低货物/AGV 密度或稍后重试",
                    ScheduleFailureKind.BudgetExhausted);
            }

            // 四方向移动
            foreach (var (dx, dy) in Directions)
            {
                var nx = curX + dx;
                var ny = curY + dy;
                if (!map.IsWalkable(nx, ny))
                    continue;
                // 顶点冲突：目标格在 nextT 被占用（含静止预留与待定起点）
                if (!table.IsVertexFree(nx, ny, nextT, agvId))
                    continue;
                // 边冲突：与他人 nextT-1→nextT 的移动互换位置
                if (!table.IsEdgeFree(curX, curY, nx, ny, nextT, agvId))
                    continue;
                TryEnqueue(open, parent, ref seq, curId, nx, ny, nextT, goalX, goalY, width, height, distToGoal);
            }

            // 原地等待（等待也可能冲突：他人可能移动到当前格）
            if (table.IsVertexFree(curX, curY, nextT, agvId))
                TryEnqueue(open, parent, ref seq, curId, curX, curY, nextT, goalX, goalY, width, height, distToGoal);
        }

        throw new SchedulerException(
            $"AGV {agvId} 路径规划失败（{KindText(ScheduleFailureKind.ConflictInfeasible)}）：" +
            $"在时间窗内穷举了全部可达的时空状态仍未找到无冲突路径（被其他 AGV 的预留堵死）",
            ScheduleFailureKind.ConflictInfeasible);
    }

    /// <summary>失败类别对应的用户可读前缀。</summary>
    internal static string KindText(ScheduleFailureKind kind) => kind switch
    {
        ScheduleFailureKind.Unreachable => "静态不可达",
        ScheduleFailureKind.BudgetExhausted => "求解预算熔断",
        ScheduleFailureKind.ConflictInfeasible => "冲突不可行",
        _ => "未知原因",
    };

    // ---- 预算参数 ----

    /// <summary>单条路径搜索的最大扩展次数（主要预算上限，确定性、与机器负载无关；测试可临时调小）。</summary>
    internal static int MaxExpansions = 2_000_000;

    /// <summary>单条路径搜索的墙钟兜底上限：只防意外挂死，正常搜索远不会触达。</summary>
    internal static TimeSpan MaxFindTime = TimeSpan.FromSeconds(10);

    /// <summary>优先级三元组：f 升序，同 f 时 h 升序（先扩展更靠近目标），再按入队序号 FIFO 打破平局。</summary>
    private static void TryEnqueue(
        PriorityQueue<int, (int F, int H, int Seq)> open,
        Dictionary<int, int> parent,
        ref int seq,
        int curId,
        int nx,
        int ny,
        int nextT,
        int goalX,
        int goalY,
        int width,
        int height,
        int[,]? distToGoal)
    {
        var nextId = nextT * width * height + ny * width + nx;
        int h;
        if (distToGoal is not null)
        {
            var d = distToGoal[ny, nx];
            if (d < 0)
                return; // 静态不可达：该格到不了目标，经此状态的搜索分支无解，直接剪枝
            h = d;
        }
        else
        {
            h = Heuristic(nx, ny, goalX, goalY);
        }

        // 该时空状态已访问则跳过（g 恒等于 t，首次到达即最优）
        if (!parent.TryAdd(nextId, curId))
            return;

        open.Enqueue(nextId, (nextT + h, h, seq++));
    }

    private static PlannedPath Reconstruct(
        int goalId,
        Dictionary<int, int> parent,
        int agvId,
        int startT,
        int width,
        int height)
    {
        var states = new List<(int X, int Y)>();
        for (var cur = goalId; cur != -1; cur = parent[cur])
            states.Add((cur % width, cur / width % height));

        states.Reverse();
        var steps = new List<PathStep>(states.Count);
        for (var j = 0; j < states.Count; j++)
        {
            var moved = j > 0 && (states[j].X != states[j - 1].X || states[j].Y != states[j - 1].Y);
            steps.Add(new PathStep(states[j].X, states[j].Y, moved ? AgvAction.Move : AgvAction.Wait));
        }

        return new PlannedPath { AgvId = agvId, StartT = startT, Steps = steps };
    }

    private static int Heuristic(int x, int y, int goalX, int goalY) => Math.Abs(x - goalX) + Math.Abs(y - goalY);
}

