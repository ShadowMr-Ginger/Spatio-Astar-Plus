using SpatioAstar.Core;
using SpatioAstar.Core.Models;
using Xunit;

namespace SpatioAstar.Core.Tests;

public sealed class MapValidatorTests
{
    private static GridMap Parse(string csv) => MapParser.Parse(csv);

    [Fact]
    public void Validate_MissingAgv_ReturnsError()
    {
        // 只有货物和港口，没有 AGV 起点（3）
        var map = Parse("2,0,4\n0,0,0\n");
        var errors = MapValidator.Validate(map);
        Assert.Contains(errors, e => e.Contains("AGV"));
    }

    [Fact]
    public void Validate_MissingPort_ReturnsError()
    {
        var map = Parse("2,0,3\n0,0,0\n");
        var errors = MapValidator.Validate(map);
        Assert.Contains(errors, e => e.Contains("港口"));
    }

    [Fact]
    public void Validate_MissingCargo_ReturnsError()
    {
        var map = Parse("3,0,4\n0,0,0\n");
        var errors = MapValidator.Validate(map);
        Assert.Contains(errors, e => e.Contains("货物"));
    }

    [Fact]
    public void Validate_UnreachableCargo_ReturnsError()
    {
        // 货物被障碍完全包围，任何 AGV 都无法到达
        const string csv = """
            3,0,1,1,1
            0,0,1,2,1
            1,1,1,1,1
            0,0,0,0,4
            0,0,0,0,0
            """;
        var map = Parse(csv);
        var errors = MapValidator.Validate(map);
        Assert.Contains(errors, e => e.Contains("无法从任何 AGV 起点到达"));
    }

    [Fact]
    public void Validate_UnreachablePort_ReturnsError()
    {
        // 港口被障碍隔离，任何 AGV 都无法到达
        const string csv = """
            3,0,2
            0,1,1
            0,1,4
            """;
        var map = Parse(csv);
        var errors = MapValidator.Validate(map);
        Assert.Contains(errors, e => e.Contains("无法从任何 AGV 起点到达"));
    }

    [Fact]
    public void Validate_LegalMap_ReturnsNoErrors()
    {
        const string csv = """
            0,0,0
            3,2,4
            0,0,0
            """;
        var map = Parse(csv);
        Assert.Empty(MapValidator.Validate(map));
    }
}
