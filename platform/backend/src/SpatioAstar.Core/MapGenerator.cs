using SpatioAstar.Core.Models;

namespace SpatioAstar.Core;

/// <summary>
/// 随机地图生成器。有效性保证：
/// 先按 obstacleRatio 撒障碍，再用 flood fill 找出最大的四连通开放区域，
/// 把所有特殊节点（AGV=3 / 货物=2 / 港口=4）全部放置在该区域内，
/// 因此从任一 AGV 出发必然能到达所有货物和港口。
/// 每次调用使用 <see cref="Random.Shared"/>，生成结果互不相同。
/// </summary>
public static class MapGenerator
{
    private const int MaxAttempts = 200;

    /// <summary>
    /// 生成一张有效地图。各参数需已由调用方 clamp 到合法范围，
    /// 但若特殊节点总数超过地图容量，会自动压缩货物/港口/AGV 数量（每种至少保留 1 个）。
    /// </summary>
    public static GridMap Generate(MapGenerationOptions options)
    {
        var (width, height, agvCount, cargoCount, portCount) = NormalizeCounts(options);
        var total = width * height;
        var special = agvCount + cargoCount + portCount;
        var obstacleTarget = (int)Math.Round((total - special) * options.ObstacleRatio);
        obstacleTarget = Math.Clamp(obstacleTarget, 0, total - special);

        for (var attempt = 0; attempt < MaxAttempts; attempt++)
        {
            var map = TryGenerate(width, height, agvCount, cargoCount, portCount, obstacleTarget);
            if (map is not null)
                return map;
        }

        throw new InvalidOperationException(
            $"无法生成有效地图（{width}x{height}，障碍率 {options.ObstacleRatio:F2}）。请降低障碍率或节点数量。");
    }

    private static (int W, int H, int Agv, int Cargo, int Port) NormalizeCounts(MapGenerationOptions o)
    {
        var capacity = o.Width * o.Height;
        var agv = Math.Min(o.AgvCount, capacity - 2); // 至少给货物和港口各留 1 格
        var port = Math.Min(o.PortCount, capacity - agv - 1);
        var cargo = Math.Min(o.CargoCount, capacity - agv - port);
        return (o.Width, o.Height, Math.Max(agv, 1), Math.Max(cargo, 1), Math.Max(port, 1));
    }

    private static GridMap? TryGenerate(int width, int height, int agvCount, int cargoCount, int portCount, int obstacleTarget)
    {
        var total = width * height;
        var special = agvCount + cargoCount + portCount;

        // 1. 打乱所有格子，前 obstacleTarget 个设为障碍，其余保持开放
        var cells = new List<(int X, int Y)>(total);
        for (var y = 0; y < height; y++)
        for (var x = 0; x < width; x++)
            cells.Add((x, y));
        Shuffle(cells);

        var grid = new int[height, width];
        for (var i = 0; i < obstacleTarget; i++)
            grid[cells[i].Y, cells[i].X] = 1;

        // 2. flood fill 找最大四连通开放区域
        var component = LargestOpenComponent(grid, width, height);
        if (component.Count < special)
            return null; // 开放区域放不下全部特殊节点，重试

        // 3. 在最大连通区域内随机选取 special 个互不重叠的格子放置特殊节点
        Shuffle(component);
        var index = 0;
        for (var i = 0; i < agvCount; i++, index++)
            grid[component[index].Y, component[index].X] = 3;
        for (var i = 0; i < portCount; i++, index++)
            grid[component[index].Y, component[index].X] = 4;
        for (var i = 0; i < cargoCount; i++, index++)
            grid[component[index].Y, component[index].X] = 2;

        return new GridMap(width, height, grid);
    }

    /// <summary>返回最大四连通开放区域的所有格子。</summary>
    private static List<(int X, int Y)> LargestOpenComponent(int[,] grid, int width, int height)
    {
        var visited = new bool[height, width];
        var largest = new List<(int X, int Y)>();

        for (var sy = 0; sy < height; sy++)
        for (var sx = 0; sx < width; sx++)
        {
            if (grid[sy, sx] == 1 || visited[sy, sx])
                continue;

            // BFS 收集当前连通块
            var component = new List<(int X, int Y)>();
            var queue = new Queue<(int X, int Y)>();
            visited[sy, sx] = true;
            queue.Enqueue((sx, sy));
            while (queue.Count > 0)
            {
                var (x, y) = queue.Dequeue();
                component.Add((x, y));
                foreach (var (nx, ny) in new[] { (x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1) })
                {
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                        continue;
                    if (grid[ny, nx] == 1 || visited[ny, nx])
                        continue;
                    visited[ny, nx] = true;
                    queue.Enqueue((nx, ny));
                }
            }

            if (component.Count > largest.Count)
                largest = component;
        }

        return largest;
    }

    private static void Shuffle<T>(IList<T> list)
    {
        for (var i = list.Count - 1; i > 0; i--)
        {
            var j = Random.Shared.Next(i + 1);
            (list[i], list[j]) = (list[j], list[i]);
        }
    }
}
