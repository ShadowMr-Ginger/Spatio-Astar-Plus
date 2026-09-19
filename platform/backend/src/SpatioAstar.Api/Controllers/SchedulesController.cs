using Microsoft.AspNetCore.Mvc;
using SpatioAstar.Api.DTOs;
using SpatioAstar.Api.Services;

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
