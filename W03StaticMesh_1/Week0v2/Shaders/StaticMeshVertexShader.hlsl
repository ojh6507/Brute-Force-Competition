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
    float4 position : POSITION; // 버텍스 위치
    float2 texcoord : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // 변환된 화면 좌표
    float2 texcoord : TEXCOORD1;
    int materialIndex : MATERIAL_INDEX;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // 위치 변환
    
    output.position = mul(mul(input.position, Model), VP);

    //if (isSelected)
    //    output.color *= 0.5;
    //// 입력 normal 값의 길이 확인
    //float normalThreshold = 0.001;
    //float normalLen = length(input.normal);
    
    //if (normalLen < normalThreshold)
    //{
    //    output.normalFlag = 0.0;
    //}
    //else
    //{
    //       output.normalFlag = 1.0;
    //}
    output.texcoord = input.texcoord;
    
    return output;
}