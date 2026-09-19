using SpatioAstar.Core;
using SpatioAstar.Core.Models;
using Xunit;

namespace SpatioAstar.Core.Tests;

public sealed class MapParserTests
{
    [Fact]
    public void Parse_ValidCsv_ReturnsExpectedGrid()
    {
        const string csv = "0,1,2\n3,0,4\n";
        var map = MapParser.Parse(csv);

        Assert.Equal(3, map.Width);
        Assert.Equal(2, map.Height);
        Assert.Equal(0, map.Grid[0, 0]);
        Assert.Equal(1, map.Grid[0, 1]);
        Assert.Equal(2, map.Grid[0, 2]);
        Assert.Equal(3, map.Grid[1, 0]);
        Assert.Equal(4, map.Grid[1, 2]);
    }

    [Fact]
    public void Parse_CrlfAndTrailingNewLine_AreTolerated()
    {
        const string csv = "0,1\r\n2,3\r\n\r\n";
        var map = MapParser.Parse(csv);
        Assert.Equal(2, map.Width);
        Assert.Equal(2, map.Height);
    }

    [Theory]
    [InlineData("")]                       // 空内容
    [InlineData("0,1\n0")]                 // 非矩形：行宽不一致
    [InlineData("0,a\n0,1")]               // 非整数格值
    [InlineData("0,5\n0,1")]               // 格值越界（>4）
    [InlineData("0,-1\n0,1")]              // 格值越界（<0）
    [InlineData("0,1\n\n0,1")]             // 中间空行
    public void Parse_InvalidCsv_ThrowsMapParseException(string csv)
    {
        Assert.Throws<MapParseException>(() => MapParser.Parse(csv));
    }

    [Fact]
    public void ToCsv_RoundTrips()
    {
        const string csv = "0,1,2\n3,0,4\n";
        var map = MapParser.Parse(csv);
        var reparsed = MapParser.Parse(MapParser.ToCsv(map));
        Assert.Equal(map.Width, reparsed.Width);
        Assert.Equal(map.Height, reparsed.Height);
        Assert.Equal(map.Grid, reparsed.Grid);
    }
}
