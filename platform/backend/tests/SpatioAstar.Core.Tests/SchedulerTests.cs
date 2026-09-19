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
}
