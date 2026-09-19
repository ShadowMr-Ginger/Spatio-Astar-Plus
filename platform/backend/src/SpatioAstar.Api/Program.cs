using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.AspNetCore.Http.Features;
using Microsoft.AspNetCore.Mvc;
using SpatioAstar.Api.Serialization;
using SpatioAstar.Api.Services;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddControllers()
    .AddJsonOptions(options =>
    {
        // 契约要求 camelCase；枚举序列化为 camelCase 字符串（move/wait/pickup/drop）
        options.JsonSerializerOptions.PropertyNamingPolicy = JsonNamingPolicy.CamelCase;
        options.JsonSerializerOptions.Converters.Add(new JsonStringEnumConverter(JsonNamingPolicy.CamelCase));
        // Core 的 GridMap.Grid 是 int[,]，.NET 8 序列化器不支持多维数组，需自定义转换
        options.JsonSerializerOptions.Converters.Add(new IntMatrixJsonConverter());
    })
    .ConfigureApiBehaviorOptions(options =>
    {
        // 契约要求所有失败统一为 { "errors": ["中文错误"] }：
        // multipart 超限等模型绑定失败默认返回英文 ProblemDetails，这里统一改写
        options.InvalidModelStateResponseFactory = context =>
        {
            var messages = context.ModelState.Values
                .SelectMany(v => v.Errors)
                .Select(e => e.ErrorMessage.Contains("Multipart body length limit", StringComparison.OrdinalIgnoreCase)
                    ? "文件大小超过 1MB 限制"
                    : string.IsNullOrWhiteSpace(e.ErrorMessage) ? "请求格式无效" : e.ErrorMessage)
                .ToList();
            return new BadRequestObjectResult(new SpatioAstar.Api.DTOs.ErrorResponse(messages));
        };
    });
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

builder.Services.AddScoped<MapGenerationService>();
builder.Services.AddScoped<ScheduleService>();

// 上传地图大小限制 1MB（Controller 上另有 RequestFormLimits 双保险）
builder.Services.Configure<FormOptions>(options =>
    options.MultipartBodyLengthLimit = ScheduleService.MaxFileBytes);

var app = builder.Build();

if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}

app.UseAuthorization();
app.MapControllers();

app.Run();
