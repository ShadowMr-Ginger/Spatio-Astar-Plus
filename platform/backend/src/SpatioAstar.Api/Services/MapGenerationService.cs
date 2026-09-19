using SpatioAstar.Core;
using SpatioAstar.Core.Models;

namespace SpatioAstar.Api.Services;

/// <summary>
/// 地图生成服务：负责参数 clamp（非法/缺省值回落到默认值再收进合法区间）与 CSV 序列化。
/// </summary>
public sealed class MapGenerationService
{
    public const int DefaultWidth = 24;
    public const int DefaultHeight = 16;
    public const int DefaultAgvCount = 3;
    public const int DefaultCargoCount = 10;
    public const int DefaultPortCount = 2;
    public const double DefaultObstacleRatio = 0.25;

    /// <summary>生成随机地图并输出 CSV 文本。所有参数为可选查询字符串，非法值按契约 clamp。</summary>
    public string GenerateCsv(
        string? width,
        string? height,
        string? agvCount,
        string? cargoCount,
        string? portCount,
        string? obstacleRatio)
    {
        var options = new MapGenerationOptions(
            Width: ClampInt(width, 5, 60, DefaultWidth),
            Height: ClampInt(height, 5, 60, DefaultHeight),
            AgvCount: ClampInt(agvCount, 1, 10, DefaultAgvCount),
            CargoCount: ClampInt(cargoCount, 1, 100, DefaultCargoCount),
            PortCount: ClampInt(portCount, 1, 10, DefaultPortCount),
            ObstacleRatio: ClampDouble(obstacleRatio, 0.05, 0.5, DefaultObstacleRatio));

        var map = MapGenerator.Generate(options);
        return MapParser.ToCsv(map);
    }

    /// <summary>解析失败或越界时先回落默认值，再收进 [min, max]。</summary>
    private static int ClampInt(string? raw, int min, int max, int defaultValue)
    {
        var value = int.TryParse(raw, out var parsed) ? parsed : defaultValue;
        return Math.Clamp(value, min, max);
    }

    private static double ClampDouble(string? raw, double min, double max, double defaultValue)
    {
        var value = double.TryParse(raw, out var parsed) ? parsed : defaultValue;
        return Math.Clamp(value, min, max);
    }
}
