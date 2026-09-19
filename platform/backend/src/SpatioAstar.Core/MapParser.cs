using System.Text;
using SpatioAstar.Core.Models;

namespace SpatioAstar.Core;

/// <summary>地图文本解析失败时抛出，消息为面向用户的中文描述。</summary>
public sealed class MapParseException : Exception
{
    public MapParseException(string message) : base(message)
    {
    }
}

/// <summary>
/// CSV 地图解析器。格式：无表头，每行逗号分隔整数，
/// 0=可通行，1=障碍，2=货物，3=AGV 初始位置，4=港口。
/// </summary>
public static class MapParser
{
    /// <summary>
    /// 将 CSV 文本解析为 <see cref="GridMap"/>。
    /// 校验：矩形网格、格值均在 0-4 之间、非空。校验节点数量与可达性的逻辑见 <see cref="MapValidator"/>。
    /// </summary>
    /// <exception cref="MapParseException">文本为空、行宽不一致、含非整数或越界格值。</exception>
    public static GridMap Parse(string csv)
    {
        if (string.IsNullOrWhiteSpace(csv))
            throw new MapParseException("地图内容为空");

        var lines = csv.Replace("\r\n", "\n").Split('\n');
        // 去掉末尾的空行（ tolerate 文件末尾多余的换行），中间的空行仍视为格式错误
        var last = lines.Length - 1;
        while (last >= 0 && lines[last].Length == 0)
            last--;
        var rows = new List<int[]>();
        var width = -1;

        for (var i = 0; i <= last; i++)
        {
            var line = lines[i];
            if (line.Length == 0)
                throw new MapParseException($"第 {i + 1} 行为空行，地图必须是完整的矩形网格");

            var tokens = line.Split(',');
            if (width < 0)
            {
                width = tokens.Length;
            }
            else if (tokens.Length != width)
            {
                throw new MapParseException($"第 {i + 1} 行有 {tokens.Length} 列，与首行 {width} 列不一致，地图必须是矩形网格");
            }

            var row = new int[width];
            for (var j = 0; j < tokens.Length; j++)
            {
                var token = tokens[j].Trim();
                if (!int.TryParse(token, out var value) || value < 0 || value > 4)
                    throw new MapParseException($"第 {i + 1} 行第 {j + 1} 列的格值“{token}”无效，必须是 0-4 的整数");
                row[j] = value;
            }

            rows.Add(row);
        }

        if (rows.Count == 0)
            throw new MapParseException("地图内容为空");

        if (width > 10_000 || rows.Count > 10_000)
            throw new MapParseException($"地图尺寸过大（{width}x{rows.Count}），宽高均不能超过 10000");

        var grid = new int[rows.Count, width];
        for (var y = 0; y < rows.Count; y++)
        for (var x = 0; x < width; x++)
            grid[y, x] = rows[y][x];

        return new GridMap(width, rows.Count, grid);
    }

    /// <summary>将 <see cref="GridMap"/> 序列化为 CSV 文本（无表头，\n 换行）。</summary>
    public static string ToCsv(GridMap map)
    {
        var sb = new StringBuilder(map.Width * map.Height * 2);
        for (var y = 0; y < map.Height; y++)
        {
            for (var x = 0; x < map.Width; x++)
            {
                if (x > 0)
                    sb.Append(',');
                sb.Append(map.Grid[y, x]);
            }

            sb.Append('\n');
        }

        return sb.ToString();
    }
}
