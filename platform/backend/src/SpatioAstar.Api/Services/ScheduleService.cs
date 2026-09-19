using SpatioAstar.Core;
using SpatioAstar.Core.Models;

namespace SpatioAstar.Api.Services;

/// <summary>
/// 调度执行服务：读取上传的 CSV 地图（1MB 上限），解析、校验后运行时空 A* 调度。
/// 任何面向用户的失败都以中文消息聚合抛出，由 Controller 统一转 400。
/// </summary>
public sealed class ScheduleService
{
    public const long MaxFileBytes = 1_048_576; // 1MB

    /// <summary>
    /// 执行调度。成功返回 <see cref="ScheduleResult"/>；
    /// 失败抛出 <see cref="ScheduleServiceException"/>（携带中文错误列表）。
    /// </summary>
    public async Task<ScheduleResult> RunAsync(IFormFile file)
    {
        if (file.Length == 0)
            throw new ScheduleServiceException("上传的文件为空");

        if (file.Length > MaxFileBytes)
            throw new ScheduleServiceException("文件大小超过 1MB 限制");

        string csv;
        await using (var stream = file.OpenReadStream())
        using (var reader = new StreamReader(stream))
        {
            csv = await reader.ReadToEndAsync();
        }

        GridMap map;
        try
        {
            map = MapParser.Parse(csv);
        }
        catch (MapParseException ex)
        {
            throw new ScheduleServiceException(ex.Message);
        }

        var errors = MapValidator.Validate(map);
        if (errors.Count > 0)
            throw new ScheduleServiceException(errors);

        try
        {
            return Scheduler.Schedule(map);
        }
        catch (SchedulerException ex)
        {
            // 调度求解失败也属于用户输入（地图）层面的失败，统一转为 400 + errors
            throw new ScheduleServiceException(ex.Message);
        }
    }
}

/// <summary>调度服务失败：携带面向用户的中文错误列表，Controller 据此返回 400 + { errors }。</summary>
public sealed class ScheduleServiceException : Exception
{
    public IReadOnlyList<string> Errors { get; }

    public ScheduleServiceException(string error) : base(error) => Errors = new List<string> { error };

    public ScheduleServiceException(IReadOnlyList<string> errors)
        : base(string.Join("；", errors)) => Errors = errors;
}
