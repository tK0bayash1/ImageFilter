// 定数バッファ(CPU側からの値受け取り場)
cbuffer global : register( b0 )
{
    matrix gWVP : register( c0 );
    int texNum : register(c4);
};


 
// 頂点シェーダから出力される構造体
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TextureUV : TEXCOORD;
};
 
// 頂点シェーダ
VS_OUTPUT VS(float4 Pos : POSITION, float2 Tex : TEXCOORD)
{
 
    VS_OUTPUT output;
    output.Pos = mul(Pos, gWVP);
    output.TextureUV = Tex;
 
    return output;
}
 
Texture2D gtextureRGB : register(t0);
Texture2D gtexture0 : register(t1);
Texture2D gtexture1 : register(t2);
Texture2D gtexture2 : register(t3);
SamplerState gDefaultSampler; // サンプラ


// ピクセルシェーダ
float4 PS_GAUSS(VS_OUTPUT input) : SV_Target
{
    float2 imageSize = float2(256, 256);
    int2 gausuSize = int2(5, 5);

    float4 result = 0;

    for (int u = 0; u < gausuSize.x; u++)
    {
        for (int v = 0; v < gausuSize.y; v++)
        {
            result += gtexture0.Sample(gDefaultSampler, input.TextureUV + float2(u - gausuSize.x / 2, v - gausuSize.y / 2) / imageSize);
        }
    }

    result /= gausuSize.x * gausuSize.y;
    return result;
}

float4 PS_SPLAT(VS_OUTPUT input) : SV_Target
{
    float4 result;
    float4 map = gtextureRGB.Sample(gDefaultSampler, input.TextureUV);
    result = map.r * gtexture0.Sample(gDefaultSampler, input.TextureUV);
    result += map.g * gtexture1.Sample(gDefaultSampler, input.TextureUV);
    result += map.b * gtexture2.Sample(gDefaultSampler, input.TextureUV);
    return result;
}

float4 PS_ORIGIN(VS_OUTPUT input) : SV_Target
{
    if (texNum == 0)
    {
        return gtextureRGB.Sample(gDefaultSampler, input.TextureUV);
    }
    if (texNum == 1)
    {
        return gtexture0.Sample(gDefaultSampler, input.TextureUV);
    }
    if (texNum == 2)
    {
        return gtexture1.Sample(gDefaultSampler, input.TextureUV);
    }
    if (texNum == 3)
    {
        return gtexture2.Sample(gDefaultSampler, input.TextureUV);
    }
    return gtexture2.Sample(gDefaultSampler, input.TextureUV);
}