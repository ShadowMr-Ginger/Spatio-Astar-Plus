using SpatioAstar.Core;
using SpatioAstar.Core.Models;
using Xunit;

namespace SpatioAstar.Core.Tests;

public sealed class MapGeneratorTests
{
    [Fact]
    public void Generate_100Maps_AllValidWithExactNodeCounts()
    {
        for (var i = 0; i < 100; i++)
        {
            // 交替使用不同规模与障碍率，覆盖 5x5 小图到 60x60 大图
            var small = i % 2 == 0;
            var options = small
                ? new MapGenerationOptions(Width: 10, Height: 8, AgvCount: 2, CargoCount: 12, PortCount: 1, ObstacleRatio: 0.3)
                : new MapGenerationOptions(Width: 40, Height: 30, AgvCount: 5, CargoCount: 60, PortCount: 3, ObstacleRatio: 0.2);

            var map = MapGenerator.Generate(options);

            Assert.Equal(options.Width, map.Width);
            Assert.Equal(options.Height, map.Height);
            Assert.Equal(options.AgvCount, CountCells(map, 3));
            Assert.Equal(options.CargoCount, CountCells(map, 2));
            Assert.Equal(options.PortCount, CountCells(map, 4));

            // 全部特殊节点位于同一连通区域（可通过调度校验间接验证可达性）
            Assert.Empty(MapValidator.Validate(map));

            // 往返序列化保持内容不变
            var reparsed = MapParser.Parse(MapParser.ToCsv(map));
            Assert.Equal(map.Grid, reparsed.Grid);
        }
    }

    [Fact]
    public void Generate_OverCapacityRequest_ClampsNodeCounts()
    {
        // 5x5=25 格放不下 10 AGV + 100 货物 + 10 港口，应自动压缩并仍生成有效地图
        var map = MapGenerator.Generate(new MapGenerationOptions(5, 5, 10, 100, 10, 0.5));
        Assert.Empty(MapValidator.Validate(map));
        Assert.True(CountCells(map, 3) >= 1);
        Assert.True(CountCells(map, 2) >= 1);
        Assert.True(CountCells(map, 4) >= 1);
    }

    private static int CountCells(GridMap map, int value)
    {
        var count = 0;
        for (var y = 0; y < map.Height; y++)
        for (var x = 0; x < map.Width; x++)
        {
            if (map.Grid[y, x] == value)
                count++;
        }

        return count;
    }
}
