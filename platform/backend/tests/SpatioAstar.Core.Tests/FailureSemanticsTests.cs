using SpatioAstar.Core;
using SpatioAstar.Core.Models;
using Xunit;

namespace SpatioAstar.Core.Tests;

/// <summary>
/// 错误语义拆分：400 错误必须区分 (a) 静态不可达（validator，见 MapValidatorTests）、
/// (b) 求解预算熔断（明确"不代表无解，可重试"）、(c) 冲突不可行（穷举无果/被预留堵死）。
/// </summary>
public sealed class FailureSemanticsTests
{
    private const string Open5x5 = """
        0,0,0,0,0
        0,3,0,2,0
        0,0,0,0,0
        0,0,0,4,0
        0,0,0,0,0
        """;

    [Fact]
    public void Find_GoalBlockedByRestingAgv_ConflictInfeasible()
    {
        var map = MapParser.Parse(Open5x5);
        var table = new ReservationTable();
        // 另一 AGV 从 t=0 起永久停靠在目标格：快速失败应判"冲突不可行"而非预算/不可达
        table.ReserveResting(agvId: 2, x: 3, y: 3, fromT: 0);

        var ex = Assert.Throws<SchedulerException>(() =>
            PathFinder.Find(map, table, agvId: 1, startX: 1, startY: 1, startT: 0, goalX: 3, goalY: 3, timeLimit: 50));

        Assert.Equal(ScheduleFailureKind.ConflictInfeasible, ex.Kind);
        Assert.Contains("冲突不可行", ex.Message);
    }

    [Fact]
    public void Find_GoalIsPendingStart_ConflictInfeasible()
    {
        var map = MapParser.Parse(Open5x5);
        var table = new ReservationTable();
        // 目标是某个尚未规划 AGV 的待定起点：全时段障碍，同样应判"冲突不可行"
        table.AddPendingStart(3, 3);

        var ex = Assert.Throws<SchedulerException>(() =>
            PathFinder.Find(map, table, agvId: 1, startX: 1, startY: 1, startT: 0, goalX: 3, goalY: 3, timeLimit: 50));

        Assert.Equal(ScheduleFailureKind.ConflictInfeasible, ex.Kind);
        Assert.Contains("冲突不可行", ex.Message);
    }

    [Fact]
    public void Schedule_TinyExpansionLimit_BudgetExhaustedThenRecovers()
    {
        var map = MapParser.Parse(Open5x5);
        var saved = PathFinder.MaxExpansions;
        try
        {
            // 把主预算上限压到 1 次扩展：任何搜索都必然熔断
            PathFinder.MaxExpansions = 1;
            var ex = Assert.Throws<SchedulerException>(() => Scheduler.Schedule(map));
            Assert.Equal(ScheduleFailureKind.BudgetExhausted, ex.Kind);
            Assert.Contains("预算", ex.Message);
            // 熔断文案必须明确"不代表无解、可重试"（与静态不可达/冲突不可行的本质区别）
            Assert.Contains("不代表", ex.Message);
            Assert.Contains("重试", ex.Message);
        }
        finally
        {
            PathFinder.MaxExpansions = saved;
        }

        // 预算恢复后同一地图立即正常求解：证明熔断只是预算问题，不是地图无解
        var result = Scheduler.Schedule(map);
        ScheduleAssert.Valid(result);
    }
}
