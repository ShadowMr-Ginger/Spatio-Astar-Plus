namespace SpatioAstar.Core.Models;

/// <summary>调度事件类型：取货（pickup）或卸货（drop）。</summary>
public enum ScheduleEventType
{
    Pickup,
    Drop,
}

/// <summary>
/// 调度事件。卸货事件带 <see cref="Port"/>（港口 id），取货事件 <see cref="Port"/> 为 null。
/// </summary>
public sealed record ScheduleEvent(
    int T,
    ScheduleEventType Type,
    int Agv,
    int Cargo,
    int? Port);
