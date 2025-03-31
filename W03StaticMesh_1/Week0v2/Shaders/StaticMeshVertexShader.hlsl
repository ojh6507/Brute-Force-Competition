// MatrixBuffer: 변환 행렬 관리
cbuffer MatrixConstants : register(b0)
{
    row_major float4x4 Model;
    float4 UUID;
  
};
cbuffer CameraConstants : register(b6)
{
    row_major float4x4 VP;
  
};

struct VS_INPUT
{
    half3 Position   : POSITION;   // 버텍스 위치
    half2 texcoord : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // 변환된 화면 좌표
    float2 texcoord : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // 위치 변환

    float4 pos = float4(input.Position, 1.0h);
    output.position = mul(mul(pos, Model), VP);
    //output.position = mul(mul(pos, Model), VP);


    output.texcoord = input.texcoord;
    
    return output;
}