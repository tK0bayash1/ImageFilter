// �萔�o�b�t�@(CPU������̒l�󂯎���)
cbuffer global
{
    matrix gWVP;
    Texture2D gtexture; // �摜�̎󂯎��
    SamplerState gDefaultSampler; // �T���v��
};
 
// ���_�V�F�[�_����o�͂����\����
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TextureUV : TEXCOORD;
};
 
// ���_�V�F�[�_
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