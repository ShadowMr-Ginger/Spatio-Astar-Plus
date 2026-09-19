namespace SpatioAstar.Api.DTOs;

/// <summary>错误响应。契约：{ "errors": ["..."] }，错误信息为中文。</summary>
public sealed record ErrorResponse(IReadOnlyList<string> Errors);
