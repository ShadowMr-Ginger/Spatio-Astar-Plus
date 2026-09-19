using System.Text.Json;
using System.Text.Json.Serialization;

namespace SpatioAstar.Api.Serialization;

/// <summary>
/// .NET 8 的 System.Text.Json 不支持多维数组（int[,]）序列化，
/// 本转换器把按 [y,x] 索引的格值矩阵输出为 jagged 二维数组，
/// 即契约要求的 "grid":[[0,..],[..]] 结构。
/// </summary>
public sealed class IntMatrixJsonConverter : JsonConverter<int[,]>
{
    public override int[,] Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        => throw new NotSupportedException("grid 字段仅用于响应输出，不支持反序列化");

    public override void Write(Utf8JsonWriter writer, int[,] value, JsonSerializerOptions options)
    {
        writer.WriteStartArray();
        for (var y = 0; y < value.GetLength(0); y++)
        {
            writer.WriteStartArray();
            for (var x = 0; x < value.GetLength(1); x++)
                writer.WriteNumberValue(value[y, x]);
            writer.WriteEndArray();
        }

        writer.WriteEndArray();
    }
}
