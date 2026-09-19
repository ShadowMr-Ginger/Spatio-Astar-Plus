using System.Diagnostics;
using SpatioAstar.Core.Models;

namespace SpatioAstar.Core;

/// <summary>任务分配策略：决定"哪件货物分给哪台 AGV"。</summary>
public enum AssignmentStrategy
{
    /// <summary>
    /// 动态分配：每轮取"最早空闲"的 AGV，给它分 1 件当前 BFS 最近的未分配货物，
    /// 更新其空闲时间后重新排序，直到货物分完。干得快/路途短的 AGV 会不断领取新货物，
    /// 不存在"完成静态配额后提前退休、旁观其他 AGV 继续搬运"的劳逸不均。
    /// </summary>
    DynamicNearest,

    /// <summary>
    /// 静态配额（旧策略）：按 ceil(剩余货物/剩余 AGV) 一次性给每个 AGV 分一批，
    /// 各 AGV 一口气连续完成自己的配额。实现简单，但路径长短不一导致先完成者闲置。
    /// 保留作为对比基准，与动态分配互相取长补短、按 makespan 取优。
    /// </summary>
    StaticQuota,
}

/// <summary>
/// 调度主流程：任务分配 + 逐 AGV 串行时空 A* 规划 + 帧序列汇总。
///
/// 任务分配提供两种策略（见 <see cref="AssignmentStrategy"/>）：动态分配按"最早空闲优先"
/// 逐个把最近货物分给 AGV，天然负载均衡；静态配额按批一次性分完。
/// 默认 <see cref="Schedule(GridMap)"/> 对两种策略各按多种 AGV 优先级顺序重试，
/// 取 makespan 最小者，保证均衡不牺牲总时长。
/// 货物选择：BFS 最近距离优先（距离相同取货物 id 小者），
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

    /// <summary>每种分配策略下整体重规划的最大尝试次数（第 1 次为自然顺序，其余为确定性随机顺序）。</summary>
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
    /// 对一张已校验的地图执行完整调度：两种分配策略 × 多种 AGV 优先顺序全部尝试，
    /// 取 makespan 最小者；统计中记录求解耗时 <see cref="ScheduleStats.ElapsedMs"/>。
    /// </summary>
    /// <exception cref="SchedulerException">所有尝试后仍存在无法送达的货物时抛出。</exception>
    public static ScheduleResult Schedule(GridMap map)
    {
        var sw = Stopwatch.StartNew();
        var best = SolveBest(map, Enum.GetValues<AssignmentStrategy>(), MaxPlanAttempts, sw);
        sw.Stop();

        var s = best.Stats;
        return best with
        {
            Stats = new ScheduleStats(s.Makespan, s.TotalMoves, s.Pickups, s.Deliveries, sw.ElapsedMilliseconds),
        };
    }

    /// <summary>
    /// 测试与对比用：以指定的一种分配策略求解（不统计 elapsedMs）。
    /// </summary>
    internal static ScheduleResult Schedule(GridMap map, AssignmentStrategy strategy, int maxAttempts = MaxPlanAttempts)
    {
        var sw = Stopwatch.StartNew();
        return SolveBest(map, new[] { strategy }, maxAttempts, sw);
    }

    /// <summary>依次执行给定策略 × 各优先顺序的重试，返回 makespan 最小的成功结果；全部失败抛异常。</summary>
    /// <summary>
    /// 软时间预算：已求得可行解后，继续用剩余顺序改进 makespan，直到总耗时超过该值。
    /// anytime 策略：先用自然顺序快速拿保底解，再花改进预算择优。
    /// </summary>
    private static readonly TimeSpan SoftTimeBudget = TimeSpan.FromSeconds(15);

    /// <summary>
    /// 硬时间预算：连一个可行解都还没求出且总耗时超过该值时放弃并报失败（400）。
    /// 设为 50 秒量级以保证 HTTP 请求一分钟内必有响应；此前 20 秒会把"贴着预算求解成功"
    /// 的地图（如 30x30/100 货物约 19 秒）在系统负载稍高时误杀。
    /// </summary>
    private static readonly TimeSpan HardTimeBudget = TimeSpan.FromSeconds(50);

    private static ScheduleResult SolveBest(
        GridMap map,
        IReadOnlyList<AssignmentStrategy> strategies,
        int maxAttempts,
        Stopwatch sw)
    {
        var cargoCells = FindCells(map, 2);
        var portCells = FindCells(map, 4);
        var agvCells = FindCells(map, 3);

        ScheduleResult? best = null;
        SchedulerException? lastError = null;
        foreach (var strategy in strategies)
        {
            for (var attempt = 0; attempt < maxAttempts; attempt++)
            {
                // 已有可行解且超过软预算：停止启动新尝试，直接返回当前最优
                if (best is not null && sw.Elapsed > SoftTimeBudget)
                    return best;

                // 始终无解且超过硬预算：放弃（正常地图不会走到这里）
                if (best is null && sw.Elapsed > HardTimeBudget)
                    break;

                // 各次尝试使用不同的 AGV 优先级顺序：自然顺序优先，失败后换确定性随机顺序
                var priorities = BuildPriorities(agvCells.Count, attempt);
                try
                {
                    // 尝试内熔断：已有可行解后用软预算（保住解、尽快返回当前最优）；
                    // 尚无可行解时用硬预算（给病态地图一个确定性的时间出口，避免单条尝试跑数分钟）。
                    // 绝不因熔断牺牲"求出第一个可行解"的机会——求不出就是 400 + 中文错误。
                    var budget = best is not null ? SoftTimeBudget : HardTimeBudget;
                    var result = Plan(map, agvCells, cargoCells, portCells, priorities, strategy, () => sw.Elapsed > budget);
                    if (best is null || result.Stats.Makespan < best.Stats.Makespan)
                        best = result;
                }
                catch (SchedulerException ex)
                {
                    lastError = ex;
                }
            }
        }

        if (best is null)
        {
            // 汇总最后的失败类别：让用户能区分"地图无解"（冲突不可行/静态不可达）
            // 与"求解预算不足"（预算熔断），后者明确提示可重试。
            var kind = lastError?.Kind ?? ScheduleFailureKind.ConflictInfeasible;
            throw new SchedulerException(
                $"已尝试 {strategies.Count * maxAttempts} 种组合仍无法完成调度（{PathFinder.KindText(kind)}）：{lastError?.Message}",
                kind);
        }

        return best;
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
        int[] priorities,
        AssignmentStrategy strategy,
        Func<bool> abort)
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

        // 分配阶段会从大量不同的源格反复求 BFS 距离（每次选货都算一遍），
        // 按源格缓存整个 Plan 共享——高密度地图下这是显著的重复劳动。
        var bfsCache = new Dictionary<(int X, int Y), int[,]>();
        int[,] DistFrom(int x, int y)
        {
            if (!bfsCache.TryGetValue((x, y), out var d))
            {
                d = MapValidator.BfsDistances(map, new List<(int X, int Y)> { (x, y) });
                bfsCache[(x, y)] = d;
            }

            return d;
        }

        // 本次尝试中出现过的失败类别：用于让"存在无法送达的货物"的文案能区分
        // 真不可行与预算熔断（TryAssignTask 内部搜索被预算砍掉时只是"没试出来"）。
        var failureKinds = new HashSet<ScheduleFailureKind>();

        // 每个货物按 BFS 距离升序的可达港口列表（距离相同取港口 id 小者），一次性预计算
        var portsByCargo = PrecomputePortsByCargo(map, cargoCells, portCells, DistFrom);
        var timeLimit = TimeLimitMultiplier * map.Width * map.Height + TimeLimitExtra;

        // 任务分配：两种策略共用同一套"取货+送货两段路径、失败回滚换候选"机制。
        // 收尾（返场 + 终态静止预留）在分配完成后统一进行；静态配额策略为保持旧行为
        // 仍在每个 AGV 完成配额时增量收尾（见 AssignByQuota）。
        // abort 是软预算熔断（仅在有可行解后激活）：超预算时让本次尝试快速失败，把时间让给外层收尾。
        if (strategy == AssignmentStrategy.StaticQuota)
            AssignByQuota(map, table, states, pending, ports, portsByCargo, events, timeLimit, abort, DistFrom, failureKinds);
        else
            AssignDynamically(map, table, states, pending, ports, portsByCargo, events, timeLimit, abort, DistFrom, failureKinds);

        if (pending.Count > 0)
        {
            var detail = string.Join("、", pending.Select(c => $"({c.X},{c.Y})"));
            // 失败类别向上汇总：底层失败只要包含预算熔断，本错误即归为 BudgetExhausted
            // （"没试出来"而非"不可行"），让上层文案能明确提示可重试。
            var hasBudget = failureKinds.Contains(ScheduleFailureKind.BudgetExhausted);
            var hint = hasBudget
                ? "（注意：失败原因包含求解预算熔断——搜索达到扩展/时间上限，不代表这些货物不可达，可稍后重试或降低密度）"
                : string.Empty;
            throw new SchedulerException(
                $"存在无法送达的货物：{detail}{hint}",
                hasBudget ? ScheduleFailureKind.BudgetExhausted : ScheduleFailureKind.ConflictInfeasible);
        }

        var result = BuildResult(map, states, ports, events);

        // 终检（见方法注释）
        VerifyNoRestingConflict(result, states);
        return result;
    }

    /// <summary>
    /// 收尾：为每个已激活的 AGV 规划回起点（返场）路径并登记终态静止预留。
    /// 起点格从一开始就受"待定起点"保护、任何其他 AGV 的路径都不会进入，
    /// 因此停靠起点在构造上消除了"先规划者撞后规划者停靠格"的隐性冲突。
    /// 返场失败（极罕见）时退而求其次停靠在最后一个港口，交由终检兜底。
    /// 已登记的 tentative 静止预留会被终态记录覆盖（ReserveResting 每 AGV 只保留一条）。
    /// </summary>
    private static void FinishAndRest(
        GridMap map,
        ReservationTable table,
        List<AgvState> states,
        int timeLimit,
        Func<int, int, int[,]> distFrom)
    {
        foreach (var agv in states.OrderBy(s => s.Priority).ThenBy(s => s.Id))
        {
            if (agv.Active)
                TryReturnHome(map, table, agv, timeLimit, distFrom);

            // 登记终态静止预留：对其他 AGV 而言其终点格从 agv.T 起永久占用
            table.ReserveResting(agv.Id, agv.X, agv.Y, agv.T);
        }
    }

    /// <summary>
    /// 动态任务分配：每轮按"最早空闲优先"（当前时刻 T 最小，并列取优先级/id 小者）
    /// 挑一台 AGV，为它分配 1 件当前 BFS 最近的未分配货物（取货+送货两段路径原子提交），
    /// 更新其空闲时间后重新排序进入下一轮，直到货物分完。
    /// 干得快/路途短的 AGV 会不断领取新货物，完成时间自然趋于一致，
    /// 不存在"完成静态配额后提前退休、旁观其他 AGV 继续搬运"的劳逸不均。
    /// 一轮内所有 AGV 都分不出去时停止，剩余货物交回上层换序重试。
    /// </summary>
    private static void AssignDynamically(
        GridMap map,
        ReservationTable table,
        List<AgvState> states,
        List<Cargo> pending,
        List<Port> ports,
        List<int>[] portsByCargo,
        List<ScheduleEvent> events,
        int timeLimit,
        Func<bool> abort,
        Func<int, int, int[,]> distFrom,
        HashSet<ScheduleFailureKind> failureKinds)
    {
        while (pending.Count > 0)
        {
            if (abort())
                break; // 软预算熔断：快速失败，把剩余时间让给外层其他尝试

            var assignedThisRound = false;
            foreach (var agv in states.OrderBy(s => s.T).ThenBy(s => s.Priority).ThenBy(s => s.Id))
            {
                if (abort())
                    break;
                var dist = distFrom(agv.X, agv.Y);
                var candidates = pending
                    .Where(c => dist[c.Y, c.X] >= 0)
                    .OrderBy(c => dist[c.Y, c.X])
                    .ThenBy(c => c.Id)
                    .ToList();

                var assigned = false;
                foreach (var cargo in candidates)
                {
                    if (abort())
                        break; // 软预算熔断

                    foreach (var portId in portsByCargo[cargo.Id])
                    {
                        // 动态策略专属保护：AGV 一旦激活，其起点（=最终停靠格）立即对他人屏蔽。
                        // 依据：返场时刻 T_home 必然 ≥ 当前 T（AGV 时间只增不减），因此从当前 T
                        // 起把起点格视为他人禁入是安全的——任何时刻规划出的路径都不会在
                        // T_home 之后占用它，终检不会再出现"撞停靠格"冲突。 TryAssignTask
                        // 失败时连同本条保护一起回滚。
                        var cp = table.Checkpoint();
                        if (!agv.Active)
                            table.ReserveResting(agv.Id, agv.StartX, agv.StartY, agv.T);
                        if (TryAssignTask(map, table, agv, cargo, ports[portId], events, timeLimit, distFrom, failureKinds))
                        {
                            pending.Remove(cargo);
                            assigned = true;
                            // 任务完成，把起点保护推进到当前时刻（等价语义，滚动收紧）
                            table.ReserveResting(agv.Id, agv.StartX, agv.StartY, agv.T);
                            break;
                        }

                        table.Rollback(cp);
                    }

                    if (assigned)
                        break;
                }

                if (assigned)
                {
                    // 成功一次即重排：该 AGV 的空闲时间已变，下一轮重新选"最早空闲"
                    assignedThisRound = true;
                    break;
                }
            }

            if (!assignedThisRound)
                break; // 所有 AGV 对剩余货物均无可行路径，交回上层换序重试
        }

        FinishAndRest(map, table, states, timeLimit, distFrom);
    }

    /// <summary>
    /// 静态配额分配（旧策略）：按优先级依次让每个 AGV 一口气连续完成分配给它的货物
    /// （取货→送货→立即从港口出发取下一件，卸货帧即下一段起点帧，中间不产生停车占坑）。
    /// 负载均衡：每个 AGV 的任务配额 = ceil(剩余货物数 / 剩余 AGV 数)。
    /// 该 AGV 已无可行任务时提前结束本轮，剩余货物留给后续 AGV。
    /// </summary>
    private static void AssignByQuota(
        GridMap map,
        ReservationTable table,
        List<AgvState> states,
        List<Cargo> pending,
        List<Port> ports,
        List<int>[] portsByCargo,
        List<ScheduleEvent> events,
        int timeLimit,
        Func<bool> abort,
        Func<int, int, int[,]> distFrom,
        HashSet<ScheduleFailureKind> failureKinds)
    {
        var remaining = states.Count;
        foreach (var agv in states.OrderBy(s => s.Priority).ThenBy(s => s.Id))
        {
            if (abort())
                break; // 软预算熔断

            remaining--;
            var quota = (pending.Count + remaining) / (remaining + 1); // ceil(pending / (remaining+1))
            var delivered = 0;
            while (delivered < quota && pending.Count > 0)
            {
                if (abort())
                    break;
                var dist = distFrom(agv.X, agv.Y);
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
                        if (TryAssignTask(map, table, agv, cargo, ports[portId], events, timeLimit, distFrom, failureKinds))
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

            // 增量收尾（时机即旧实现）：该 AGV 的货物一做完立刻返场并登记终态静止预留，
            // 后续 AGV 的规划会主动避让它的停靠格，避免终点被撞、减少整体重试。
            if (agv.Active)
                TryReturnHome(map, table, agv, timeLimit, distFrom);
            table.ReserveResting(agv.Id, agv.X, agv.Y, agv.T);
        }
    }

    /// <summary>
    /// 规划从当前位置返回起点（返场）的收尾路径并提交。起点格对 AGV 自身永远空闲，
    /// 返回 true；任何失败（路径不可行）都返回 false 且不留副作用（预留表回滚）。
    /// </summary>
    private static bool TryReturnHome(
        GridMap map,
        ReservationTable table,
        AgvState agv,
        int timeLimit,
        Func<int, int, int[,]> distFrom)
    {
        var checkpoint = table.Checkpoint();
        try
        {
            var homePath = PathFinder.Find(
                map, table, agv.Id, agv.X, agv.Y, agv.T, agv.StartX, agv.StartY, agv.T + timeLimit,
                distFrom(agv.StartX, agv.StartY));
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
        List<(int X, int Y)> portCells,
        Func<int, int, int[,]> distFrom)
    {
        return cargoCells.Select(c =>
        {
            var cargoDist = distFrom(c.X, c.Y);
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
    /// 失败类别记录到 <paramref name="failureKinds"/>，供上层错误文案区分"不可行"与"预算熔断"。
    /// </summary>
    private static bool TryAssignTask(
        GridMap map,
        ReservationTable table,
        AgvState agv,
        Cargo cargo,
        Port port,
        List<ScheduleEvent> events,
        int timeLimit,
        Func<int, int, int[,]> distFrom,
        HashSet<ScheduleFailureKind> failureKinds)
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

            // 第一段：当前位置 → 货物格（时空 A*）。启发值用货物格的 BFS 距离场（见 Find 注释）。
            var pickupPath = PathFinder.Find(
                map, table, agv.Id, agv.X, agv.Y, agv.T, cargo.X, cargo.Y, agv.T + timeLimit,
                distFrom(cargo.X, cargo.Y));
            table.ReservePath(pickupPath);

            // 第二段：货物格 → 港口，与第一段共享取货帧
            var pickupT = pickupPath.EndT;
            var dropPath = PathFinder.Find(
                map, table, agv.Id, cargo.X, cargo.Y, pickupT, port.X, port.Y, pickupT + timeLimit,
                distFrom(port.X, port.Y));
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
        catch (SchedulerException ex)
        {
            // 回滚预留表；帧与事件在成功前不会写入，无需回滚
            table.Rollback(checkpoint);
            agv.Active = wasActive;
            failureKinds.Add(ex.Kind);
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

        // ElapsedMs 由外层 Schedule 统一回填（多策略择优后才确定最终结果），此处先置 0
        var stats = new ScheduleStats(makespan, totalMoves, events.Count(e => e.Type == ScheduleEventType.Pickup), events.Count(e => e.Type == ScheduleEventType.Drop), 0L);
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
