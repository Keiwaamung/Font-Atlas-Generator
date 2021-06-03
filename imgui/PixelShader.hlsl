sampler tex0 : register(s0);

struct PS_Input
{
    float4 pos : VPOS;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
    float4 ch  : COLOR1;
};

struct PS_Output
{
    float4 col : COLOR;
};

PS_Output main(PS_Input input)
{
    PS_Output output;
    float gray = dot(input.ch, tex2D(tex0, input.uv));
    float4 color = tex2D(tex0, input.uv);
    output.col = input.col;
    output.col.a = output.col.a * gray;
    return output;
};
