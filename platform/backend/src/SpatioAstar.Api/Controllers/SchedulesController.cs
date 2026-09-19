using Microsoft.AspNetCore.Mvc;
using SpatioAstar.Api.DTOs;
using SpatioAstar.Api.Services;
using SpatioAstar.Core;

namespace SpatioAstar.Api.Controllers;

/// <summary>调度执行接口。</summary>
[ApiController]
[Route("api/schedules")]
public sealed class SchedulesController : ControllerBase
{
    private readonly ScheduleService _scheduleService;

    public SchedulesController(ScheduleService scheduleService)
    {
        _scheduleService = scheduleService;
    }

    /// <summary>
    /// 估算给定规模地图的调度耗时区间（毫秒），无需上传地图。
    /// cargoCount（货物数）与 openCount（开放格数，即非障碍格数）为必填正整数，
    /// agvCount 可选、默认 1；缺失或非法返回 400 + { "errors": ["..."] }（中文）。
    /// 模型与校准数据见 <see cref="SchedulerEstimator"/>。
    /// </summary>
    [HttpGet("estimate")]
    public IActionResult Estimate(
        [FromQuery] string? cargoCount,
        [FromQuery] string? openCount,
        [FromQuery] string? agvCount)
    {
        var errors = new List<string>();
        var cargo = ParsePositive(cargoCount, "cargoCount（货物数）", errors);
        var open = ParsePositive(openCount, "openCount（开放格数）", errors);
        var agv = agvCount is null ? 1 : ParsePositive(agvCount, "agvCount（AGV 数）", errors);
        if (errors.Count > 0)
            return BadRequest(new ErrorResponse(errors));

        var est = SchedulerEstimator.Estimate(cargo, open, agv);
        return Ok(new EstimateResponse(est.MinMs, est.MaxMs));
    }

    /// <summary>解析正整数查询参数；缺失、非整数或非正数均记入 errors（失败时返回 0，调用方据 errors 判断）。</summary>
    private static int ParsePositive(string? raw, string displayName, List<string> errors)
    {
        if (string.IsNullOrWhiteSpace(raw) || !int.TryParse(raw, out var value) || value <= 0)
        {
            errors.Add($"{displayName}必须是正整数");
            return 0;
        }

        return value;
    }

    /// <summary>
    /// 上传 CSV 地图（multipart/form-data，字段名 file，1MB 上限），
    /// 运行多 AGV 时空 A* 调度，返回逐帧状态序列、事件流与统计。
    /// 校验失败返回 400 + { "errors": ["..."] }（中文）。
    /// </summary>
    [HttpPost("run")]
    [RequestFormLimits(MultipartBodyLengthLimit = ScheduleService.MaxFileBytes)]
    [Consumes("multipart/form-data")]
    public async Task<IActionResult> Run(IFormFile? file)
    {
        if (file is null || file.Length == 0)
            return BadRequest(new ErrorResponse(new List<string> { "缺少上传文件：请使用 multipart/form-data，字段名 file" }));

        try
        {
            var result = await _scheduleService.RunAsync(file);
            return Ok(result);
        }
        catch (ScheduleServiceException ex)
        {
            return BadRequest(new ErrorResponse(ex.Errors));
        }
    }
}
