namespace SpatioAstar.Core.Models;

/// <summary>调度统计信息。</summary>
/// <param name="Makespan">完成全部任务的最后时刻（最后一帧的 t）。</param>
/// <param name="TotalMoves">所有 AGV 的 move 动作总次数。</param>
/// <param name="Pickups">取货次数。</param>
/// <param name="Deliveries">送达次数。</param>
/// <param name="ElapsedMs">调度求解耗时（毫秒），自 <see cref="Scheduler.Schedule"/> 入口至结果就绪。</param>
public sealed record ScheduleStats(int Makespan, int TotalMoves, int Pickups, int Deliveries, long ElapsedMs);

/// <summary>一次完整调度的结果：地图快照、逐帧状态序列、事件流与统计。</summary>
public sealed record ScheduleResult(
    int Width,
    int Height,
    int[,] Grid,
    IReadOnlyList<Agv> Agvs,
    IReadOnlyList<Cargo> Cargos,
    IReadOnlyList<Port> Ports,
    IReadOnlyList<Frame> Frames,
    IReadOnlyList<ScheduleEvent> Events,
    ScheduleStats Stats);
