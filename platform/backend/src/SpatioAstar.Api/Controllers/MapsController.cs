using Microsoft.AspNetCore.Mvc;
using SpatioAstar.Api.Services;

namespace SpatioAstar.Api.Controllers;

/// <summary>随机地图生成接口。</summary>
[ApiController]
[Route("api/maps")]
public sealed class MapsController : ControllerBase
{
    private readonly MapGenerationService _generationService;

    public MapsController(MapGenerationService generationService)
    {
        _generationService = generationService;
    }

    /// <summary>
    /// 生成一张随机有效地图并以 CSV 附件返回。
    /// 参数全部可选且非法值自动 clamp：width/height 5-60（默认 24x16），
    /// agvCount 1-10（默认 3），cargoCount 1-100（默认 10），portCount 1-10（默认 2），
    /// obstacleRatio 0.05-0.5（默认 0.25）。
    /// </summary>
    [HttpGet("generate")]
    [Produces("text/csv")]
    public IActionResult Generate(
        [FromQuery] string? width,
        [FromQuery] string? height,
        [FromQuery] string? agvCount,
        [FromQuery] string? cargoCount,
        [FromQuery] string? portCount,
        [FromQuery] string? obstacleRatio)
    {
        var csv = _generationService.GenerateCsv(width, height, agvCount, cargoCount, portCount, obstacleRatio);
        var bytes = System.Text.Encoding.UTF8.GetBytes(csv);
        return File(bytes, "text/csv", "map.csv");
    }
}
