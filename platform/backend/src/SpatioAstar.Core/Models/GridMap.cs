namespace SpatioAstar.Core.Models;

/// <summary>
/// 矩形网格地图。坐标约定：x 为列（对应 CSV 中一行内的第 x 个值），y 为行。
/// 格值含义：0=可通行，1=障碍，2=货物，3=AGV 初始位置，4=港口。
/// </summary>
public sealed class GridMap
{
    public int Width { get; }
    public int Height { get; }

    /// <summary>按 [y, x] 索引的格值矩阵。</summary>
    public int[,] Grid { get; }

    public GridMap(int width, int height, int[,] grid)
    {
        if (width < 1 || height < 1)
            throw new ArgumentOutOfRangeException(nameof(grid), "地图宽高必须为正数");
        if (grid.GetLength(0) != height || grid.GetLength(1) != width)
            throw new ArgumentException("网格维度与指定的宽高不一致", nameof(grid));
        Width = width;
        Height = height;
        Grid = grid;
    }

    public bool InBounds(int x, int y) => x >= 0 && x < Width && y >= 0 && y < Height;

    public bool IsObstacle(int x, int y) => !InBounds(x, y) || Grid[y, x] == 1;

    /// <summary>AGV 可进入的格子（在界内且非障碍；货物/AGV/港口格均可通行）。</summary>
    public bool IsWalkable(int x, int y) => InBounds(x, y) && Grid[y, x] != 1;

    public int this[int x, int y] => Grid[y, x];
}
