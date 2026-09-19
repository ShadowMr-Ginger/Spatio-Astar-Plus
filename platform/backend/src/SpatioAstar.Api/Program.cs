using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.AspNetCore.Http.Features;
using SpatioAstar.Api.Services;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddControllers()
    .AddJsonOptions(options =>
    {
        // 契约要求 camelCase；枚举序列化为 camelCase 字符串（move/wait/pickup/drop）
        options.JsonSerializerOptions.PropertyNamingPolicy = JsonNamingPolicy.CamelCase;
        options.JsonSerializerOptions.Converters.Add(new JsonStringEnumConverter(JsonNamingPolicy.CamelCase));
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
