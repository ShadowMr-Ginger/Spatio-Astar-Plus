namespace SpatioAstar.Core;

/// <summary>
/// 调度耗时估算（纯函数，无副作用）。
/// 模型用本仓库实测数据校准：
///   - 30×30 / 100 货物 / 589 开放格 ≈ 3.8s
///   - 60×60 / 100 货物 / ≈2700 开放格 ≈ 0.85s
///   - 小地图（≈180–240 开放格 / 6 货物） ≈ 0.3s
/// 观测结论：求解耗时主要随"货物密度" cargoCount²/openCount 增长——
/// 失败搜索的穷举是主要开销（成功路径的搜索代价可忽略），密度越高失败搜索越多。
/// 公式：baseMs = 300（HTTP+框架固定开销） + 224 × cargoCount² / openCount，
/// 即 cargoCount²/openCount 的秒数 × 系数 0.224 × 1000（双精度计算防大数溢出）。
/// 校验：100²/589×0.224≈3.81s ✓；100²/2700×0.224≈0.83s ✓；6²/180×0.224≈0.045s（+300ms 开销）✓。
/// agvCount 当前不参与计算：更多 AGV 理论上摊薄单 AGV 任务量，但校准点均取默认 1，
/// 未校准的加权不如忽略；接口保留该参数，后续可轻微加权。
/// </summary>
public static class SchedulerEstimator
{
    /// <summary>maxMs 的硬上限：与求解硬熔断（50s）和 HTTP 预算（&lt;60s）保持一致余量。</summary>
    public const long MaxAllowedMs = 55_000;

    /// <summary>估算调度耗时区间。cargoCount/openCount/agvCount 必须为正整数，否则抛参数异常（Api 层负责转 400）。</summary>
    public static EstimateResult Estimate(int cargoCount, int openCount, int agvCount = 1)
    {
        if (cargoCount <= 0)
            throw new ArgumentOutOfRangeException(nameof(cargoCount), cargoCount, "货物数必须为正整数");
        if (openCount <= 0)
            throw new ArgumentOutOfRangeException(nameof(openCount), openCount, "开放格数必须为正整数");
        if (agvCount <= 0)
            throw new ArgumentOutOfRangeException(nameof(agvCount), agvCount, "AGV 数必须为正整数");

        var baseMs = 300.0 + 224.0 * cargoCount * cargoCount / openCount;
        // 全程用 double 比较并在转型前钳制：cargoCount 极大时 baseMs 可远超 long 范围，
        // 未检查转型会溢出为 long.MinValue
        var minMs = Math.Max(200.0, baseMs * 0.5);
        var maxMs = Math.Min((double)MaxAllowedMs, baseMs * 2.5 + 300);
        // 极端参数下 min 可能超过 max 被钳制后的值，收拢以保持 minMs ≤ maxMs 不变式
        minMs = Math.Min(minMs, maxMs);
        return new EstimateResult(
            (long)Math.Round(minMs, MidpointRounding.AwayFromZero),
            (long)Math.Round(maxMs, MidpointRounding.AwayFromZero));
    }
}

/// <summary>估算结果。JSON 契约：{ "minMs": &lt;long&gt;, "maxMs": &lt;long&gt; }（camelCase 由序列化负责）。</summary>
public readonly record struct EstimateResult(long MinMs, long MaxMs);
