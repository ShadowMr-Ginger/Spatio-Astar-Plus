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

/// <summary>调度求解失败时抛出，消息为面向用户的中文描述。</summary>
public sealed class SchedulerException : Exception
{
    public SchedulerException(string message) : base(message)
    {
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
    private readonly Dictionary<(int X, int Y, int T), int> _vertices = new();
    private readonly HashSet<(int X1, int Y1, int X2, int Y2, int T)> _edges = new();
    /// <summary>
    /// 静止预留：AGV 完成一次卸货后从其最后一帧起占据该格，直到它被分配下一个任务。
    /// 每个 AGV 至多一条记录，重复调用覆盖旧记录（旧终点格随之释放）。
    /// 这样"停在港口的 AGV"对后续规划者始终是可见的永久障碍，不会暗中产生冲突。
    /// </summary>
    private readonly Dictionary<int, (int X, int Y, int FromT)> _resting = new();
    private readonly HashSet<(int X, int Y)> _pendingStarts = new();

    /// <summary>登记一个尚未规划路径的 AGV 起点：其他 AGV 在任何时刻都不得进入该格。</summary>
    public void AddPendingStart(int x, int y) => _pendingStarts.Add((x, y));

    /// <summary>
    /// 判断 agvId 在时刻 t 能否占据 (x,y)。
    /// 检查顶点预留、静止预留与待定起点；本 AGV 自己的预留不构成冲突。
    /// </summary>
    public bool IsVertexFree(int x, int y, int t, int agvId)
    {
        if (_vertices.TryGetValue((x, y, t), out var owner) && owner != agvId)
            return false;

        foreach (var (restingAgv, cell) in _resting)
        {
            if (restingAgv != agvId && cell.X == x && cell.Y == y && t >= cell.FromT)
                return false;
        }

        // 起点待定说明该 AGV 还没规划，可能一直停在此处；但搜索时不会查到自己（已先移除）
        return !_pendingStarts.Contains((x, y));
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
        _resting[agvId] = (x, y, fromT);
        _undoLog.Add(() =>
        {
            if (hadValue) _resting[agvId] = oldValue;
            else _resting.Remove(agvId);
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
        _pendingStarts.Remove((x, y));
        _undoLog.Add(() => _pendingStarts.Add((x, y)));
    }

    /// <summary>将一条已规划路径写入预留表：逐帧预留顶点，移动步额外预留有向边。支持回滚。</summary>
    public void ReservePath(PlannedPath path)
    {
        for (var j = 0; j < path.Steps.Count; j++)
        {
            var step = path.Steps[j];
            var t = path.StartT + j;
            var key = (step.X, step.Y, t);
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
    public static PlannedPath Find(
        GridMap map,
        ReservationTable table,
        int agvId,
        int startX,
        int startY,
        int startT,
        int goalX,
        int goalY,
        int timeLimit)
    {
        // 快速失败：目标格在整个时间窗内都被占用（如被静止的 AGV 永久堵住），
        // 任何搜索都不可能成功，直接抛出，避免为证明失败而穷举整个时空状态空间
        var goalEverFree = false;
        for (var t = startT; t <= timeLimit; t++)
        {
            if (table.IsVertexFree(goalX, goalY, t, agvId))
            {
                goalEverFree = true;
                break;
            }
        }

        if (!goalEverFree)
            throw new SchedulerException(
                $"AGV {agvId} 路径规划失败：目标 ({goalX},{goalY}) 在时间上限内均被占用");

        // 开放列表：(f = g + h, 入队序号, 状态)。序号保证出队顺序确定。
        var open = new PriorityQueue<(int X, int Y, int T), (int F, int Seq)>();
        var gScore = new Dictionary<(int X, int Y, int T), int>();
        var parent = new Dictionary<(int X, int Y, int T), (int X, int Y, int T)>();
        var seq = 0;
        var expansions = 0;
        var findSw = System.Diagnostics.Stopwatch.StartNew();

        var start = (X: startX, Y: startY, T: startT);
        gScore[start] = startT;
        open.Enqueue(start, (startT + Heuristic(startX, startY, goalX, goalY), seq++));

        while (open.Count > 0)
        {
            var cur = open.Dequeue();
            if (cur.X == goalX && cur.Y == goalY)
            {
                TotalFinds++; TotalExpansions += expansions;
                if (findSw.ElapsedMilliseconds > SlowestFindMs) { SlowestFindMs = findSw.ElapsedMilliseconds; PeakExpansions = expansions; }
                return Reconstruct(cur, parent, agvId, startT);
            }

            var nextT = cur.T + 1;
            if (nextT > timeLimit)
                continue; // 超出时间上限的扩展直接丢弃，队列耗尽时统一报失败

            // 搜索规模保护：为证明"不可行"而穷举超大时空状态空间代价极高，
            // 超过阈值后快速失败，交由上层的候选回退/顺序重试处理
            if (++expansions > MaxExpansions)
                break;

            // 四方向移动
            foreach (var (dx, dy) in Directions)
            {
                var nx = cur.X + dx;
                var ny = cur.Y + dy;
                if (!map.IsWalkable(nx, ny))
                    continue;
                // 顶点冲突：目标格在 nextT 被占用（含静止预留与待定起点）
                if (!table.IsVertexFree(nx, ny, nextT, agvId))
                    continue;
                // 边冲突：与他人 nextT-1→nextT 的移动互换位置
                if (!table.IsEdgeFree(cur.X, cur.Y, nx, ny, nextT, agvId))
                    continue;
                TryEnqueue(open, gScore, parent, ref seq, cur, nx, ny, nextT, goalX, goalY);
            }

            // 原地等待（等待也可能冲突：他人可能移动到当前格）
            if (table.IsVertexFree(cur.X, cur.Y, nextT, agvId))
                TryEnqueue(open, gScore, parent, ref seq, cur, cur.X, cur.Y, nextT, goalX, goalY);
        }

        TotalFinds++; TotalExpansions += expansions;
        if (findSw.ElapsedMilliseconds > SlowestFindMs) { SlowestFindMs = findSw.ElapsedMilliseconds; PeakExpansions = expansions; }
        throw new SchedulerException(
            $"AGV {agvId} 路径规划失败：无法在时间上限内从 ({startX},{startY}) 到达 ({goalX},{goalY})，请检查地图或降低节点密度");
    }

    // 诊断计数（临时）
    public static long TotalFinds;
    public static long TotalExpansions;
    public static long SlowestFindMs;
    public static long PeakExpansions;

    /// <summary>单条路径搜索的最大扩展次数（防卡死的规模保护）。</summary>
    private const int MaxExpansions = 500_000;

    private static void TryEnqueue(
        PriorityQueue<(int X, int Y, int T), (int F, int Seq)> open,
        Dictionary<(int X, int Y, int T), int> gScore,
        Dictionary<(int X, int Y, int T), (int X, int Y, int T)> parent,
        ref int seq,
        (int X, int Y, int T) cur,
        int nx,
        int ny,
        int nextT,
        int goalX,
        int goalY)
    {
        var next = (X: nx, Y: ny, T: nextT);
        if (gScore.TryGetValue(next, out var known) && known <= nextT)
            return; // 已以更早的到达时刻访问过同一时空状态

        gScore[next] = nextT;
        parent[next] = cur;
        open.Enqueue(next, (nextT + Heuristic(nx, ny, goalX, goalY), seq++));
    }

    private static PlannedPath Reconstruct(
        (int X, int Y, int T) goal,
        Dictionary<(int X, int Y, int T), (int X, int Y, int T)> parent,
        int agvId,
        int startT)
    {
        var states = new List<(int X, int Y, int T)>();
        for (var cur = goal; ; cur = parent[cur])
        {
            states.Add(cur);
            if (cur.T == startT)
                break;
        }

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

