using System.Text.Json;
using Microsoft.AspNetCore.Mvc;
using SpatioAstar.Api.Controllers;
using SpatioAstar.Api.DTOs;
using SpatioAstar.Core;
using Xunit;

namespace SpatioAstar.Core.Tests;

/// <summary>耗时估算接口：校准点回归、区间不变式、非法参数 400。</summary>
public sealed class EstimateTests
{
    /// <summary>三个实测校准点必须落在模型区间内（数据见 SchedulerEstimator 注释）。</summary>
    [Fact]
    public void Estimate_CalibrationPoints_MatchMeasuredRuntime()
    {
        // 30×30 / 100 货物 / 589 开放格 ≈ 3.8s
        var dense = SchedulerEstimator.Estimate(cargoCount: 100, openCount: 589);
        Assert.InRange(dense.MinMs, 1_800, 2_300);
        Assert.InRange(dense.MaxMs, 9_500, 11_500);

        // 60×60 / 100 货物 / ≈2700 开放格 ≈ 0.85s
        var sparse = SchedulerEstimator.Estimate(cargoCount: 100, openCount: 2_700);
        Assert.InRange(sparse.MinMs, 500, 650);
        Assert.InRange(sparse.MaxMs, 2_800, 3_400);

        // 小地图 ≈180 开放格 / 6 货物 ≈ 0.3s：下限被 200ms 托住
        var tiny = SchedulerEstimator.Estimate(cargoCount: 6, openCount: 180);
        Assert.Equal(200, tiny.MinMs);
        Assert.InRange(tiny.MaxMs, 1_000, 1_300);
    }

    /// <summary>不变式：200 ≤ minMs ≤ maxMs ≤ 55000，极端参数下也成立。</summary>
    [Theory]
    [InlineData(1, 1)]
    [InlineData(6, 180)]
    [InlineData(100, 589)]
    [InlineData(100, 2_700)]
    [InlineData(10_000, 3_600)]
    [InlineData(2_000_000_000, 1)]
    public void Estimate_AnyScale_KeepsIntervalInvariant(int cargo, int open)
    {
        var est = SchedulerEstimator.Estimate(cargo, open);
        Assert.InRange(est.MinMs, 200, SchedulerEstimator.MaxAllowedMs);
        Assert.InRange(est.MaxMs, 200, SchedulerEstimator.MaxAllowedMs);
        Assert.True(est.MinMs <= est.MaxMs);
    }

    [Theory]
    [InlineData(0, 100)]
    [InlineData(-5, 100)]
    [InlineData(10, 0)]
    [InlineData(10, -100)]
    [InlineData(10, 100, 0)]
    public void Estimate_NonPositiveArgs_Throws(int cargo, int open, int agv = 1)
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => SchedulerEstimator.Estimate(cargo, open, agv));
    }

    /// <summary>正常请求：200 + { minMs, maxMs }（camelCase long）。</summary>
    [Fact]
    public void EstimateEndpoint_ValidQuery_ReturnsInterval()
    {
        var controller = new SchedulesController(null!);
        var result = Assert.IsType<OkObjectResult>(controller.Estimate("100", "589", "10"));
        var payload = Assert.IsType<EstimateResponse>(result.Value);

        Assert.InRange(payload.MinMs, 1_800, 2_300);
        Assert.InRange(payload.MaxMs, 9_500, 11_500);

        // 序列化契约：经 Api 的 camelCase 策略序列化后为 minMs/maxMs 两个数字字段
        var options = new JsonSerializerOptions { PropertyNamingPolicy = JsonNamingPolicy.CamelCase };
        var json = JsonSerializer.Serialize(payload, options);
        using var doc = JsonDocument.Parse(json);
        Assert.True(doc.RootElement.TryGetProperty("minMs", out var min));
        Assert.True(doc.RootElement.TryGetProperty("maxMs", out var max));
        Assert.Equal(JsonValueKind.Number, min.ValueKind);
        Assert.Equal(JsonValueKind.Number, max.ValueKind);
    }

    /// <summary>缺失/非整数/非正数参数：400 + { "errors": ["中文"] }，多个错误全部列出。</summary>
    [Fact]
    public void EstimateEndpoint_InvalidQuery_Returns400WithChineseErrors()
    {
        var controller = new SchedulesController(null!);
        var result = Assert.IsType<BadRequestObjectResult>(controller.Estimate(null, "abc", "0"));
        var payload = Assert.IsType<ErrorResponse>(result.Value);

        Assert.Equal(3, payload.Errors.Count);
        Assert.Contains(payload.Errors, e => e.Contains("cargoCount（货物数）必须是正整数"));
        Assert.Contains(payload.Errors, e => e.Contains("openCount（开放格数）必须是正整数"));
        Assert.Contains(payload.Errors, e => e.Contains("agvCount（AGV 数）必须是正整数"));
    }

    /// <summary>agvCount 缺省时默认 1，结果与显式传 1 一致。</summary>
    [Fact]
    public void EstimateEndpoint_DefaultAgvCount_MatchesExplicitOne()
    {
        var controller = new SchedulesController(null!);
        var omitted = Assert.IsType<OkObjectResult>(controller.Estimate("50", "500", null));
        var explicitOne = Assert.IsType<OkObjectResult>(controller.Estimate("50", "500", "1"));
        Assert.Equal(omitted.Value, explicitOne.Value);
    }
}
