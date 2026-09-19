using SpatioAstar.Core.Models;

namespace SpatioAstar.Core;

/// <summary>
/// 调度主流程：任务分配 + 逐 AGV 串行时空 A* 规划 + 帧序列汇总。
///
/// 任务分配采用贪心循环：每当有 AGV 空闲且还有未分配货物，
/// 取当前时刻最小、优先级最高的 AGV（时刻与优先级都相同再取 id 小者，保证确定性），
/// 用 BFS 距离为它选最近的未分配可达货物（距离相同取货物 id 小者），
/// 卸货港口选距离货物最近且可达的港口（距离相同取港口 id 小者）。
/// 一个 AGV 同一时刻最多携带 1 件货物：取货后必须先送达港口卸货，才能被分配下一件。
///
/// 规划按 AGV 顺序串行进行（先生成的路径先在预留表占坑，后加入者避让），
/// 因此最终帧序列天然无顶点冲突与边冲突。全部货物送达即结束，makespan = 最后一帧 t。
///
/// 串行优先规划可能出现"先发 AGV 堵住后发 AGV"的死锁。两重缓解：
/// 1. 一次分配失败（取货或送货任一段无冲突路径不可行）时回滚预留表，改试下一候选（其他货物/其他 AGV）；
/// 2. 全部候选均失败时，换用不同的 AGV 优先级顺序整体重规划（固定种子的确定性重试）。
/// </summary>
public static class Scheduler
{
    private const int TimeLimitMultiplier = 4;
    private const int TimeLimitExtra = 64;

    /// <summary>整体重规划的最大尝试次数（第 1 次为自然顺序，其余为确定性随机顺序）。</summary>
    private const int MaxPlanAttempts = 8;

    /// <summary>重试时洗牌使用的固定种子，保证同一地图的重试序列可复现。</summary>
    private const int RetrySeedBase = 20260101;

    /// <summary>单个 AGV 的调度状态：当前位置、当前时刻、已生成的帧序列、优先级与是否已激活。</summary>
    private sealed class AgvState
    {
        public required int Id { get; init; }
        public required int StartX { get; init; }
        public required int StartY { get; init; }
        public int Priority { get; init; }
        public int X { get; set; }
        public int Y { get; set; }
        public int T { get; set; }
        public bool Active { get; set; }
        public bool Retired { get; set; }
        public List<FrameState> Frames { get; } = new();
    }

    /// <summary>
    /// 对一张已校验的地图执行完整调度。
    /// </summary>
    /// <exception cref="SchedulerException">所有尝试后仍存在无法送达的货物时抛出。</exception>
    public static ScheduleResult Schedule(GridMap map)
    {
        var cargoCells = FindCells(map, 2);
        var portCells = FindCells(map, 4);
        var agvCells = FindCells(map, 3);

        SchedulerException? lastError = null;
        for (var attempt = 0; attempt < MaxPlanAttempts; attempt++)
        {
            // 各次尝试使用不同的 AGV 优先级顺序：自然顺序优先，失败后换确定性随机顺序
            var priorities = BuildPriorities(agvCells.Count, attempt);
            try
            {
                return Plan(map, agvCells, cargoCells, portCells, priorities);
            }
            catch (SchedulerException ex)
            {
                lastError = ex;
            }
        }

        throw new SchedulerException(
            $"已尝试 {MaxPlanAttempts} 种 AGV 优先顺序仍无法完成调度：{lastError?.Message}");
    }

    /// <summary>生成第 attempt 次尝试的 AGV 优先级表（值越小越优先）。</summary>
    private static int[] BuildPriorities(int agvCount, int attempt)
    {
        var priorities = Enumerable.Range(0, agvCount).ToArray();
        if (attempt > 0)
        {
            var rng = new Random(RetrySeedBase + attempt);
            for (var i = agvCount - 1; i > 0; i--)
            {
                var j = rng.Next(i + 1);
                (priorities[i], priorities[j]) = (priorities[j], priorities[i]);
            }
        }

        return priorities;
    }

    private static ScheduleResult Plan(
        GridMap map,
        List<(int X, int Y)> agvCells,
        List<(int X, int Y)> cargoCells,
        List<(int X, int Y)> portCells,
        int[] priorities)
    {
        var ports = portCells.Select((c, i) => new Port(i, c.X, c.Y)).ToList();
        var states = agvCells.Select((c, i) =>
        {
            var s = new AgvState
            {
                Id = i,
                StartX = c.X,
                StartY = c.Y,
                Priority = priorities[i],
                X = c.X,
                Y = c.Y,
            };
            s.Frames.Add(new FrameState(i, c.X, c.Y, AgvAction.Wait, -1));
            return s;
        }).ToList();

        var table = new ReservationTable();
        foreach (var s in states)
            table.AddPendingStart(s.StartX, s.StartY);

        // 待分配货物（货物 id 按格序分配，保证确定性）
        var pending = cargoCells.Select((c, i) => new Cargo(i, c.X, c.Y, false)).ToList();
        var events = new List<ScheduleEvent>();

        // 每个货物按 BFS 距离升序的可达港口列表（距离相同取港口 id 小者），一次性预计算
        var portsByCargo = PrecomputePortsByCargo(map, cargoCells, portCells);
        var timeLimit = TimeLimitMultiplier * map.Width * map.Height + TimeLimitExtra;

        // 逐 AGV 连续规划：按优先级依次让每个 AGV 一口气连续完成分配给它的货物
        // （取货→送货→立即从港口出发取下一件，卸货帧即下一段起点帧，中间不产生停车占坑）。
        // 负载均衡：每个 AGV 的任务配额 = ceil(剩余货物数 / 剩余 AGV 数)。
        // 一个 AGV 规划完毕立即登记终态静止预留，之后规划的其他 AGV 会主动避让它的终点。
        var remaining = states.Count;
        foreach (var agv in states.OrderBy(s => s.Priority).ThenBy(s => s.Id))
        {
            remaining--;
            var quota = (pending.Count + remaining) / (remaining + 1); // ceil(pending / (remaining+1))
            var delivered = 0;
            while (delivered < quota && pending.Count > 0)
            {
                var dist = MapValidator.BfsDistances(map, new List<(int X, int Y)> { (agv.X, agv.Y) });
                var candidates = pending
                    .Where(c => dist[c.Y, c.X] >= 0)
                    .OrderBy(c => dist[c.Y, c.X])
                    .ThenBy(c => c.Id)
                    .ToList();

                var assigned = false;
                foreach (var cargo in candidates)
                {
                    foreach (var portId in portsByCargo[cargo.Id])
                    {
                        if (TryAssignTask(map, table, agv, cargo, ports[portId], events, timeLimit))
                        {
                            pending.Remove(cargo);
                            delivered++;
                            assigned = true;
                            break;
                        }
                    }

                    if (assigned)
                        break;
                }

                if (!assigned)
                    break; // 该 AGV 已无可行任务，提前结束本轮
            }

            // 任务做完后"返场"：规划一段回到自己起点的收尾路径，最终停靠在起点格。
            // 起点格从一开始就受"待定起点"保护、任何其他 AGV 的路径都不会进入，
            // 因此停靠起点在构造上消除了"先规划者撞后规划者停靠格"的隐性冲突。
            // 返场失败（极罕见）时退而求其次停靠在最后一个港口，交由终检兜底。
            if (agv.Active)
                TryReturnHome(map, table, agv, timeLimit);

            // 登记终态静止预留：对其他 AGV 而言其终点格从 agv.T 起永久占用
            table.ReserveResting(agv.Id, agv.X, agv.Y, agv.T);
        }

        if (pending.Count > 0)
        {
            var detail = string.Join("、", pending.Select(c => $"({c.X},{c.Y})"));
            throw new SchedulerException($"存在无法送达的货物：{detail}");
        }

        var result = BuildResult(map, states, ports, events);

        // 终检（见方法注释）
        VerifyNoRestingConflict(result, states);
        return result;
    }

    /// <summary>
    /// 规划从当前位置返回起点（返场）的收尾路径并提交。起点格对 AGV 自身永远空闲，
    /// 返回 true；任何失败（路径不可行）都返回 false 且不留副作用（预留表回滚）。
    /// </summary>
    private static bool TryReturnHome(
        GridMap map,
        ReservationTable table,
        AgvState agv,
        int timeLimit)
    {
        var checkpoint = table.Checkpoint();
        try
        {
            var homePath = PathFinder.Find(
                map, table, agv.Id, agv.X, agv.Y, agv.T, agv.StartX, agv.StartY, agv.T + timeLimit);
            table.ReservePath(homePath);
            AppendLeg(agv, homePath);
            agv.X = agv.StartX;
            agv.Y = agv.StartY;
            agv.T = homePath.EndT;
            return true;
        }
        catch (SchedulerException)
        {
            table.Rollback(checkpoint);
            return false;
        }
    }

    /// <summary>
    /// 校验任一 AGV 的最终停靠格不会被其他 AGV 在其停靠后进入。
    /// AGV 的最终停靠格 = 规划结束时的位置（返场成功时为自己的起点，否则为最后一个港口），
    /// 从 agv.T（其最后一帧）起被静止预留保护。逐 AGV 规划中后规划者知道先规划者的停靠格，
    /// 但先规划者的路径可能穿过后来者最终停靠的格子（当时后者的静止预留尚不存在）——
    /// 帧序列若存在此类"静止被撞"冲突，说明当前优先顺序不可行，交由上层换序重试。
    /// </summary>
    private static void VerifyNoRestingConflict(
        ScheduleResult result,
        IReadOnlyList<AgvState> states)
    {
        var restFrom = new Dictionary<int, int>();
        var restCell = new Dictionary<int, (int X, int Y)>();
        foreach (var s in states.Where(s => s.Active))
        {
            restFrom[s.Id] = s.T;
            restCell[s.Id] = (s.X, s.Y);
        }

        for (var t = 0; t <= result.Stats.Makespan; t++)
        {
            foreach (var (agvId, fromT) in restFrom)
            {
                if (t <= fromT)
                    continue;
                var cell = restCell[agvId];
                var intruder = result.Frames[t].States.FirstOrDefault(
                    s => s.Agv != agvId && s.X == cell.X && s.Y == cell.Y);
                if (intruder is not null)
                    throw new SchedulerException($"AGV {intruder.Agv} 在 t={t} 进入 AGV {agvId} 的最终停靠格");
            }
        }
    }

    /// <summary>预计算每个货物按 BFS 距离升序（相同取 id 小者）的可达港口 id 列表。</summary>
    private static List<int>[] PrecomputePortsByCargo(
        GridMap map,
        List<(int X, int Y)> cargoCells,
        List<(int X, int Y)> portCells)
    {
        return cargoCells.Select(c =>
        {
            var cargoDist = MapValidator.BfsDistances(map, new List<(int X, int Y)> { (c.X, c.Y) });
            return portCells
                .Select((p, id) => (Id: id, Dist: cargoDist[p.Y, p.X]))
                .Where(p => p.Dist >= 0)
                .OrderBy(p => p.Dist)
                .ThenBy(p => p.Id)
                .Select(p => p.Id)
                .ToList();
        }).ToArray();
    }

    /// <summary>
    /// 尝试为某个 AGV 规划"取货 + 送货"两段路径。
    /// 两段都成功才提交（写预留表、追加帧、记录事件）；任一失败则回滚预留表并返回 false。
    /// 第二段路径与第一段共享取货帧：取货动作发生在第一段到达货物格的同一帧。
    /// </summary>
    private static bool TryAssignTask(
        GridMap map,
        ReservationTable table,
        AgvState agv,
        Cargo cargo,
        Port port,
        List<ScheduleEvent> events,
        int timeLimit)
    {
        var wasActive = agv.Active;
        var checkpoint = table.Checkpoint();
        try
        {
            // 首次被分配任务：起点从"待定起点"转为正常顶点预留
            if (!agv.Active)
            {
                table.RemovePendingStart(agv.StartX, agv.StartY);
                agv.Active = true;
            }

            // 第一段：当前位置 → 货物格（时空 A*）
            var pickupPath = PathFinder.Find(map, table, agv.Id, agv.X, agv.Y, agv.T, cargo.X, cargo.Y, agv.T + timeLimit);
            table.ReservePath(pickupPath);

            // 第二段：货物格 → 港口，与第一段共享取货帧
            var pickupT = pickupPath.EndT;
            var dropPath = PathFinder.Find(map, table, agv.Id, cargo.X, cargo.Y, pickupT, port.X, port.Y, pickupT + timeLimit);
            table.ReservePath(dropPath);

            // 两段均可行，正式提交预留、帧与事件
            AppendLeg(agv, pickupPath);
            var dropT = dropPath.EndT;
            AppendLeg(agv, dropPath);
            SetAction(agv, pickupT, AgvAction.Pickup);
            SetAction(agv, dropT, AgvAction.Drop);
            SetCarrying(agv, pickupT, dropT, cargo.Id);
            events.Add(new ScheduleEvent(pickupT, ScheduleEventType.Pickup, agv.Id, cargo.Id, null));
            events.Add(new ScheduleEvent(dropT, ScheduleEventType.Drop, agv.Id, cargo.Id, port.Id));

            // 卸货后该 AGV 空载。下一段任务在本 AGV 的本轮规划内紧接出发，
            // 对其他 AGV 的位置保护由其本轮结束时的终态静止预留提供。
            agv.X = port.X;
            agv.Y = port.Y;
            agv.T = dropT;
            return true;
        }
        catch (SchedulerException)
        {
            // 回滚预留表；帧与事件在成功前不会写入，无需回滚
            table.Rollback(checkpoint);
            agv.Active = wasActive;
            return false;
        }
    }

    /// <summary>
    /// 把一段路径追加到 AGV 时间线。每段 Steps[0] 与上一段末帧同刻同格（帧已存在），
    /// 统一从 j=1 追加；携带状态由 <see cref="SetCarrying"/> 在两段都规划完后统一修正。
    /// </summary>
    private static void AppendLeg(AgvState agv, PlannedPath path)
    {
        for (var j = 1; j < path.Steps.Count; j++)
        {
            var step = path.Steps[j];
            agv.Frames.Add(new FrameState(agv.Id, step.X, step.Y, step.Action, -1));
        }
    }

    /// <summary>改写某帧的动作（用于把到达帧标记为 Pickup/Drop）。</summary>
    private static void SetAction(AgvState agv, int t, AgvAction action)
    {
        var f = agv.Frames[t];
        agv.Frames[t] = f with { Action = action };
    }

    /// <summary>设置携带状态：取货帧至卸货帧（含两端）携带 cargoId，卸货帧之后恢复 -1。</summary>
    private static void SetCarrying(AgvState agv, int pickupT, int dropT, int cargoId)
    {
        for (var t = pickupT; t <= dropT && t < agv.Frames.Count; t++)
        {
            var f = agv.Frames[t];
            agv.Frames[t] = f with { Carrying = cargoId };
        }
    }

    /// <summary>汇总结果：按时间对齐所有 AGV 的帧（已结束的 AGV 静止补 wait 帧），生成事件流与统计。</summary>
    private static ScheduleResult BuildResult(
        GridMap map,
        List<AgvState> states,
        List<Port> ports,
        List<ScheduleEvent> events)
    {
        var makespan = states.Max(s => s.T);
        var frames = new List<Frame>(makespan + 1);
        var totalMoves = 0;
        for (var t = 0; t <= makespan; t++)
        {
            var frameStates = new List<FrameState>(states.Count);
            foreach (var s in states)
            {
                if (t < s.Frames.Count)
                {
                    frameStates.Add(s.Frames[t]);
                    if (s.Frames[t].Action == AgvAction.Move)
                        totalMoves++;
                }
                else
                {
                    // 已完成全部任务的 AGV：静止在终点，wait + 空载
                    frameStates.Add(new FrameState(s.Id, s.X, s.Y, AgvAction.Wait, -1));
                }
            }

            frames.Add(new Frame(t, frameStates));
        }

        var orderedEvents = events
            .OrderBy(e => e.T)
            .ThenBy(e => e.Agv)
            .ThenBy(e => e.Type)
            .ToList();

        var stats = new ScheduleStats(makespan, totalMoves, events.Count(e => e.Type == ScheduleEventType.Pickup), events.Count(e => e.Type == ScheduleEventType.Drop));
        var agvList = states.Select(s => new Agv(s.Id, s.StartX, s.StartY)).ToList();
        var deliveredCargos = FindCells(map, 2).Select((c, i) => new Cargo(i, c.X, c.Y, true)).ToList();
        return new ScheduleResult(map.Width, map.Height, map.Grid, agvList, deliveredCargos, ports, frames, orderedEvents, stats);
    }

    private static List<(int X, int Y)> FindCells(GridMap map, int value)
    {
        var cells = new List<(int X, int Y)>();
        for (var y = 0; y < map.Height; y++)
        for (var x = 0; x < map.Width; x++)
        {
            if (map.Grid[y, x] == value)
                cells.Add((x, y));
        }

        return cells;
    }
}
