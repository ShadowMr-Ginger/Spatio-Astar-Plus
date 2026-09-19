namespace SpatioAstar.Api.DTOs;

/// <summary>耗时估算响应。契约：{ "minMs": &lt;long&gt;, "maxMs": &lt;long&gt; }。</summary>
public sealed record EstimateResponse(long MinMs, long MaxMs);
