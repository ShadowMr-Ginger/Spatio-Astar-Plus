namespace SpatioAstar.Core.Models;

/// <summary>随机地图生成参数。各字段的有效范围由 API 层 clamp 后传入。</summary>
public sealed record MapGenerationOptions(
    int Width,
    int Height,
    int AgvCount,
    int CargoCount,
    int PortCount,
    double ObstacleRatio);
