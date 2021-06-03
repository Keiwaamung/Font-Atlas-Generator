float4x4 mvp : register(c0);

struct VS_Input
{
    float3 pos : POSITION0;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
    float4 ch  : COLOR1;
};

struct VS_Output
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
    float4 ch  : COLOR1;
};

VS_Output main(VS_Input input)
{
    VS_Output output;
    output.pos = mul(mvp, float4(input.pos.x, input.pos.y, input.pos.z, 1.0f));
    output.uv  = input.uv;
    output.col = input.col;
    output.ch  = input.ch;
    return output;
};
