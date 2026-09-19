using SpatioAstar.Core;
using SpatioAstar.Core.Models;
using Xunit;

namespace SpatioAstar.Core.Tests;

public sealed class SchedulerTests
{
    /// <summary>单 AGV 单货物单港口：最简场景，makespan 应等于曼哈顿最优。</summary>
    [Fact]
    public void Schedule_SingleAgvSingleCargo_OptimalMakespan()
    {
        const string csv = """
            0,0,0,0,0
            0,3,0,2,0
            0,0,0,0,0
            0,4,0,0,0
            0,0,0,0,0
            """;
        var result = Scheduler.Schedule(MapParser.Parse(csv));

        ScheduleAssert.Valid(result);
        // AGV (1,1) → 货物 (3,1) 距离 2，货物 → 港口 (1,3) 距离 4，港口 → 起点返场距离 2
        Assert.Equal(8, result.Stats.Makespan);
    }

    /// <summary>狭窄通道内双 AGV 相向而行：必须借助等待/绕行避免互换对撞。</summary>
    [Fact]
    public void Schedule_HeadOnCorridor_NoSwapCollision()
    {
        const string csv = """
            1,0,0,0,1
            1,3,2,3,1
            1,4,0,4,1
            """;
        var result = Scheduler.Schedule(MapParser.Parse(csv));

        ScheduleAssert.Valid(result);
        Assert.Equal(2, result.Agvs.Count);
        Assert.True(result.Cargos.Count >= 1);
    }

    /// <summary>多 AGV 多货物：贪心分配 + 串行时空 A*，全部送达且无冲突。</summary>
    [Fact]
    public void Schedule_MultiAgvMultiCargo_AllDeliveredConflictFree()
    {
        const string csv = """
            0,0,0,0,0,0,0,0
            0,3,0,2,0,2,0,3
            0,0,0,0,0,0,0,0
            0,2,0,1,1,0,2,0
            0,0,0,1,1,0,0,0
            0,2,0,0,0,0,2,0
            0,0,0,0,0,0,0,0
            0,4,0,2,0,2,0,4
            """;
        var result = Scheduler.Schedule(MapParser.Parse(csv));

        ScheduleAssert.Valid(result);
        Assert.Equal(2, result.Agvs.Count);
        Assert.Equal(8, result.Cargos.Count);
        Assert.Equal(2, result.Ports.Count);
    }

    /// <summary>带随机障碍的生成地图：调度和生成器组合，规模接近上限仍须秒级且全部送达。</summary>
    [Fact]
    public void Schedule_GeneratedLargeMap_AllDeliveredConflictFree()
    {
        var map = MapGenerator.Generate(new MapGenerationOptions(30, 20, 4, 40, 2, 0.25));
        var result = Scheduler.Schedule(map);

        ScheduleAssert.Valid(result);
        Assert.Equal(4, result.Agvs.Count);
        Assert.Equal(40, result.Cargos.Count);
    }

    /// <summary>单行通道被墙挡住、只有远端缺口可绕行：验证时空 A* 会绕开障碍而非穿墙。</summary>
    [Fact]
    public void Schedule_DetourAroundWall_AllDeliveredConflictFree()
    {
        // y=2 的墙把 AGV(1,1) 与下方港口 (1,3) 隔开，只有 x=4 的竖缝可以绕行
        const string csv = """
            1,1,1,1,1,1,1
            1,3,0,0,0,2,1
            1,1,1,1,0,1,1
            1,4,0,0,0,0,1
            1,1,1,1,1,1,1
            """;
        var result = Scheduler.Schedule(MapParser.Parse(csv));

        ScheduleAssert.Valid(result);
        Assert.Single(result.Agvs);
        Assert.Single(result.Cargos);
    }

    /// <summary>单个 AGV 连续送 5 件货物：验证取货→卸货→再取货的串任务时间线衔接正确。</summary>
    [Fact]
    public void Schedule_SingleAgvManyCargos_AllDeliveredConflictFree()
    {
        const string csv = """
            0,0,0,0,0,0
            0,3,2,2,2,0
            0,0,0,0,0,0
            0,2,2,4,0,0
            0,0,0,0,0,0
            """;
        var result = Scheduler.Schedule(MapParser.Parse(csv));

        ScheduleAssert.Valid(result);
        Assert.Single(result.Agvs);
        Assert.Equal(5, result.Cargos.Count);
        Assert.Equal(5, result.Stats.Pickups);
        Assert.Equal(5, result.Stats.Deliveries);
    }

    /// <summary>性能抽查：60x60、10 AGV、100 货物，单次调度须在 10 秒内完成且结果合法。</summary>
    [Fact]
    public void Schedule_MaxScaleMap_CompletesWithinTenSeconds()
    {
        var map = MapGenerator.Generate(new MapGenerationOptions(60, 60, 10, 100, 3, 0.2));
        var sw = System.Diagnostics.Stopwatch.StartNew();
        var result = Scheduler.Schedule(map);
        sw.Stop();

        ScheduleAssert.Valid(result);
        Assert.Equal(10, result.Agvs.Count);
        Assert.Equal(100, result.Cargos.Count);
        Assert.True(
            sw.Elapsed < TimeSpan.FromSeconds(10),
            $"调度耗时 {sw.Elapsed.TotalSeconds:F1} 秒，超过 10 秒上限");
    }

    /// <summary>
    /// 负载均衡场景：货物集中在一角、1 台 AGV 离货物群很远（静态配额下它仍要独自跑长途）。
    /// 动态分配让近处 AGV 多劳、远处 AGV 只并行帮工，makespan 不得劣于静态配额。
    /// </summary>
    [Fact]
    public void Schedule_UnbalancedMap_DynamicNoWorseThanQuota()
    {
        // 港口与 8 件货物挤在左上 5x3 区域；AGV(1,2)、(4,2) 就在旁边，AGV(14,6) 在右下角
        const string csv = """
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,4,0,2,2,2,0,0,0,0,0,0,0,0,0
            0,3,0,2,3,2,0,0,0,0,0,0,0,0,0
            0,0,0,2,2,2,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,3
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            """;
        var map = MapParser.Parse(csv);
        var dynamicResult = Scheduler.Schedule(map, AssignmentStrategy.DynamicNearest);
        var quotaResult = Scheduler.Schedule(map, AssignmentStrategy.StaticQuota);

        ScheduleAssert.Valid(dynamicResult);
        ScheduleAssert.Valid(quotaResult);
        Assert.True(
            dynamicResult.Stats.Makespan <= quotaResult.Stats.Makespan,
            $"动态分配 makespan {dynamicResult.Stats.Makespan} 应不劣于静态配额 {quotaResult.Stats.Makespan}");
    }

    /// <summary>动态分配的行为特征：货物分完之前不存在"AGV 完成任务后长期闲置"——
    /// 每台被用过 AGV 的最后一次作业时间应紧贴 makespan（静态配额则允许近处 AGV 早早退休）。</summary>
    [Fact]
    public void Schedule_UnbalancedMap_DynamicKeepsAgvsBusy()
    {
        const string csv = """
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,4,0,2,2,2,0,0,0,0,0,0,0,0,0
            0,3,0,2,3,2,0,0,0,0,0,0,0,0,0
            0,0,0,2,2,2,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,3
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            """;
        var result = Scheduler.Schedule(MapParser.Parse(csv), AssignmentStrategy.DynamicNearest);
        ScheduleAssert.Valid(result);

        // 每台 AGV 的"退休时间"= 最后一帧非 wait 动作的时刻（返场移动也算作业）。
        // 动态分配下所有 AGV 应在几乎同一时刻收尾，最大退休时差不超过 1 趟近途往返。
        var retireTimes = result.Agvs.Select(a =>
        {
            for (var t = result.Stats.Makespan; t >= 0; t--)
            {
                var s = result.Frames[t].States.Single(x => x.Agv == a.Id);
                if (s.Action != AgvAction.Wait)
                    return t;
            }
            return 0;
        }).ToList();

        var spread = retireTimes.Max() - retireTimes.Min();
        Assert.True(
            spread <= 20,
            $"动态分配下 AGV 退休时间差 {spread} 过大（{string.Join(",", retireTimes)}），存在提前闲置");
    }

    /// <summary>默认组合入口应在两种策略、多种优先顺序的全部结果中择优，且不劣于任一单独策略。</summary>
    [Fact]
    public void Schedule_DefaultPicksBestAcrossStrategies()
    {
        // 与 DynamicNoWorseThanQuota 同一的不均衡地图（确定性）
        const string csv = """
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,4,0,2,2,2,0,0,0,0,0,0,0,0,0
            0,3,0,2,3,2,0,0,0,0,0,0,0,0,0
            0,0,0,2,2,2,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,3
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            """;
        var map = MapParser.Parse(csv);
        var combined = Scheduler.Schedule(map);
        var dynamicResult = Scheduler.Schedule(map, AssignmentStrategy.DynamicNearest);
        var quotaResult = Scheduler.Schedule(map, AssignmentStrategy.StaticQuota);

        ScheduleAssert.Valid(combined);
        Assert.True(combined.Stats.Makespan <= dynamicResult.Stats.Makespan);
        Assert.True(combined.Stats.Makespan <= quotaResult.Stats.Makespan);
        Assert.True(combined.Stats.ElapsedMs >= 0);
    }
}
