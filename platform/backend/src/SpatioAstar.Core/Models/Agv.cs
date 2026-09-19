namespace SpatioAstar.Core.Models;

/// <summary>AGV 实体：从地图上的 3 格出发。</summary>
public sealed record Agv(int Id, int StartX, int StartY);

/// <summary>货物实体：位于地图上的 2 格，需被取货并送达任一港口。</summary>
public sealed record Cargo(int Id, int X, int Y, bool Delivered);

/// <summary>港口实体：位于地图上的 4 格，是货物的卸货点。</summary>
public sealed record Port(int Id, int X, int Y);
