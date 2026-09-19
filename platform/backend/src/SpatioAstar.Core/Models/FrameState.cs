namespace SpatioAstar.Core.Models;

/// <summary>AGV 在单帧内执行的动作。API 层以 camelCase 字符串序列化（move/wait/pickup/drop）。</summary>
public enum AgvAction
{
    Move,
    Wait,
    Pickup,
    Drop,
}

/// <summary>某一帧中单个 AGV 的状态。</summary>
/// <param name="Agv">AGV id。</param>
/// <param name="X">该帧所在格 x 坐标。</param>
/// <param name="Y">该帧所在格 y 坐标。</param>
/// <param name="Action">该帧执行的动作。</param>
/// <param name="Carrying">正在携带的货物 id，未携带时为 -1。取货帧及卸货帧均记为携带该货物。</param>
public sealed record FrameState(int Agv, int X, int Y, AgvAction Action, int Carrying);

/// <summary>一帧的完整状态：时刻 t 与所有 AGV 的状态列表。</summary>
public sealed record Frame(int T, IReadOnlyList<FrameState> States);
