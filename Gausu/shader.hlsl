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

float4 PS_ORIGIN(VS_OUTPUT input) : SV_Target
{
    return gtexture.Sample(gDefaultSampler, input.TextureUV);
}