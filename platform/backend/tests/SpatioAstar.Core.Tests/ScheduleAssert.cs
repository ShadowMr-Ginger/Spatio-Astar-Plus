using SpatioAstar.Core;
using SpatioAstar.Core.Models;
using Xunit;

namespace SpatioAstar.Core.Tests;

/// <summary>调度结果的通用不变量校验，供各测试复用。</summary>
public static class ScheduleAssert
{
    public static void Valid(ScheduleResult r)
    {
        // 帧序列：从 t=0 到 t=makespan 连续编号，数量恰为 makespan+1
        Assert.True(r.Stats.Makespan > 0);
        Assert.Equal(r.Stats.Makespan, r.Frames[^1].T);
        Assert.Equal(r.Stats.Makespan + 1, r.Frames.Count);
        for (var t = 0; t <= r.Stats.Makespan; t++)
            Assert.Equal(t, r.Frames[t].T);

        // 统计：取货/送达次数等于货物数，全部货物送达
        Assert.Equal(r.Cargos.Count, r.Stats.Pickups);
        Assert.Equal(r.Cargos.Count, r.Stats.Deliveries);
        Assert.All(r.Cargos, c => Assert.True(c.Delivered));

        // 事件：每个货物恰好一次 pickup 和一次 drop，pickup 早于 drop，drop 带港口
        Assert.Equal(2 * r.Cargos.Count, r.Events.Count);
        foreach (var cargo in r.Cargos)
        {
            var pickup = r.Events.Single(e => e.Type == ScheduleEventType.Pickup && e.Cargo == cargo.Id);
            var drop = r.Events.Single(e => e.Type == ScheduleEventType.Drop && e.Cargo == cargo.Id);
            Assert.True(pickup.T < drop.T);
            Assert.NotNull(drop.Port);
            Assert.True(pickup.T < r.Frames.Count);
            Assert.True(drop.T < r.Frames.Count);
            // pickup 帧位置在货物格，drop 帧位置在某个港口格
            var pickupFrame = r.Frames[pickup.T].States.Single(s => s.Agv == pickup.Agv);
            Assert.Equal((cargo.X, cargo.Y), (pickupFrame.X, pickupFrame.Y));
            Assert.Equal(AgvAction.Pickup, pickupFrame.Action);
            var dropFrame = r.Frames[drop.T].States.Single(s => s.Agv == drop.Agv);
            Assert.Contains(r.Ports, p => (p.X, p.Y) == (dropFrame.X, dropFrame.Y));
            Assert.Equal(AgvAction.Drop, dropFrame.Action);
        }

        // 帧级冲突检查
        for (var t = 0; t <= r.Stats.Makespan; t++)
        {
            var states = r.Frames[t].States;

            // 每帧每个 AGV 恰一条 state
            Assert.Equal(r.Agvs.Count, states.Count);
            Assert.Equal(states.Select(s => s.Agv).OrderBy(id => id), r.Agvs.Select(a => a.Id).OrderBy(id => id));

            // 顶点冲突：同一帧任意两 AGV 不同点
            var cells = states.Select(s => (s.X, s.Y)).ToHashSet();
            Assert.Equal(states.Count, cells.Count);

            // 所有位置可通行且在界内
            foreach (var s in states)
                Assert.True(r.Grid[s.Y, s.X] != 1);

            // 相邻帧间的移动只能是四方向移动或原地等待
            if (t > 0)
            {
                var prev = r.Frames[t - 1].States;
                foreach (var s in states)
                {
                    var p = prev.Single(x => x.Agv == s.Agv);
                    var manhattan = Math.Abs(p.X - s.X) + Math.Abs(p.Y - s.Y);
                    Assert.True(manhattan <= 1, $"AGV {s.Agv} 从 ({p.X},{p.Y}) 跳到 ({s.X},{s.Y})");
                }
            }
        }

        // 边冲突：任意相邻帧，两 AGV 不得互换位置
        for (var t = 0; t < r.Stats.Makespan; t++)
        {
            var cur = r.Frames[t].States;
            var next = r.Frames[t + 1].States;
            foreach (var a in cur)
            {
                var aNext = next.Single(s => s.Agv == a.Agv);
                foreach (var b in cur)
                {
                    if (b.Agv == a.Agv)
                        continue;
                    var bNext = next.Single(s => s.Agv == b.Agv);
                    Assert.False(
                        a.X == bNext.X && a.Y == bNext.Y && b.X == aNext.X && b.Y == aNext.Y,
                        $"AGV {a.Agv} 与 AGV {b.Agv} 在 t={t}→{t + 1} 互换位置对撞");
                }
            }
        }
    }
}
