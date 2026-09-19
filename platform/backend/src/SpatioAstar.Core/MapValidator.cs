using SpatioAstar.Core.Models;

namespace SpatioAstar.Core;

/// <summary>
/// 地图可调度性校验。解析（<see cref="MapParser"/>）只保证格式合法，
/// 本类保证地图能够被调度算法完整求解。
/// </summary>
public static class MapValidator
{
    /// <summary>
    /// 校验地图是否可调度，返回错误列表（为空表示合法）：
    /// 至少 1 个 AGV / 1 个港口 / 1 个货物；
    /// 每个货物和港口都能从某个 AGV 起点可达。
    /// （可达性即同一四连通分量：货物能从 AGV 到达、港口也能从 AGV 到达，
    /// 则货物必然能送达该港口，调度一定可完整求解。）
    /// </summary>
    public static IReadOnlyList<string> Validate(GridMap map)
    {
        var errors = new List<string>();
        var agvs = FindCells(map, 3);
        var cargos = FindCells(map, 2);
        var ports = FindCells(map, 4);

        if (agvs.Count == 0)
            errors.Add("地图中至少需要 1 个 AGV 起点（格值 3）");
        if (ports.Count == 0)
            errors.Add("地图中至少需要 1 个港口（格值 4）");
        if (cargos.Count == 0)
            errors.Add("地图中至少需要 1 个货物（格值 2）");
        if (errors.Count > 0)
            return errors;

        // 从所有 AGV 起点做多源 BFS，得到"静态可达"区域
        var reachableFromAgvs = BfsDistances(map, agvs);

        foreach (var (x, y) in cargos)
        {
            if (reachableFromAgvs[y, x] < 0)
                errors.Add($"货物 ({x},{y}) 无法从任何 AGV 起点到达");
        }

        foreach (var (x, y) in ports)
        {
            if (reachableFromAgvs[y, x] < 0)
                errors.Add($"港口 ({x},{y}) 无法从任何 AGV 起点到达");
        }

        return errors;
    }

    private static List<(int X, int Y)> FindCells(GridMap map, int value)
    {
        var cells = new List<(int X, int Y)>();
        for (var y = 0; y < map.Height; y++)
        for (var x = 0; x < map.Width; x++)
        {
            if (map.Grid[y, x] == value)
                cells.Add((x, y));
        }

        return cells;
    }

    /// <summary>多源 BFS，返回 dist[y,x]（不可达为 -1）。只把非障碍格视为可通行。</summary>
    public static int[,] BfsDistances(GridMap map, IReadOnlyList<(int X, int Y)> sources)
    {
        var dist = new int[map.Height, map.Width];
        for (var y = 0; y < map.Height; y++)
        for (var x = 0; x < map.Width; x++)
            dist[y, x] = -1;

        var queue = new Queue<(int X, int Y)>();
        foreach (var (sx, sy) in sources)
        {
            if (!map.IsWalkable(sx, sy))
                continue;
            dist[sy, sx] = 0;
            queue.Enqueue((sx, sy));
        }

        while (queue.Count > 0)
        {
            var (x, y) = queue.Dequeue();
            var next = dist[y, x] + 1;
            foreach (var (nx, ny) in Neighbors(map, x, y))
            {
                if (dist[ny, nx] >= 0)
                    continue;
                dist[ny, nx] = next;
                queue.Enqueue((nx, ny));
            }
        }

        return dist;
    }

    private static IEnumerable<(int X, int Y)> Neighbors(GridMap map, int x, int y)
    {
        if (map.IsWalkable(x + 1, y)) yield return (x + 1, y);
        if (map.IsWalkable(x - 1, y)) yield return (x - 1, y);
        if (map.IsWalkable(x, y + 1)) yield return (x, y + 1);
        if (map.IsWalkable(x, y - 1)) yield return (x, y - 1);
    }
}
