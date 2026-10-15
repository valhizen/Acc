struct VSOutput {
    float4 Position : SV_Position;
    float3 Color : COLOR0;
};

static const float2 positions[3] = {float2(0.0, -0.5), float2(0.5, 0.5), float2(-0.5, 0.5)};
static const float3 colors[3] = {float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0)};

[[shader("vertex")]]
VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    output.Position = float4(positions[vertexID], 0.0f, 1.0f);
    output.Color = colors[vertexID];
    return output;
}

[[shader("fragment")]]
float4 FSMain(VSOutput input) : SV_Target {
    return float4(input.Color, 1.0f);
}
