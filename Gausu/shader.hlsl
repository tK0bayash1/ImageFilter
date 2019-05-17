// 定数バッファ(CPU側からの値受け取り場)
cbuffer global
{
    matrix gWVP;
    Texture2D gtexture; // 画像の受け取り
    SamplerState gDefaultSampler; // サンプラ
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
            result += gtexture.Sample(gDefaultSampler, input.TextureUV + float2(u - gausuSize.x / 2, v - gausuSize.y / 2) / imageSize);
        }
    }

    result /= gausuSize.x * gausuSize.y;
    return result;
}

float4 PS_SOBEL(VS_OUTPUT input) : SV_Target
{
    float2 imageSize = float2(256, 256);

    float4 result = 0;
    float4 ghs = 0;
    float4 gus = 0;
    
    int3x3 ghsMat = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };

    for (int u = 0; u < 3; u++)
    {
        for (int v = 0; v < 3; v++)
        {
            ghs += gtexture.Sample(gDefaultSampler, input.TextureUV + float2(u - 1, v - 1) / imageSize) * ghsMat[u][v];
            gus += gtexture.Sample(gDefaultSampler, input.TextureUV + float2(u - 1, v - 1) / imageSize) * ghsMat[v][u];
        }
    }
    
    result = (ghs + gus) / 2;

    return result;
}

float4 PS_ORIGIN(VS_OUTPUT input) : SV_Target
{
    return gtexture.Sample(gDefaultSampler, input.TextureUV);
}