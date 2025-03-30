struct InstanceData
{
    row_major float4x4 WorldMatrix;
};

// MatrixBuffer: 변환 행렬 관리
cbuffer MatrixConstants : register(b0)
{
    row_major float4x4 MVP;
  
};

// 오프셋 정보를 담는 상수 버퍼 
cbuffer PerBatchData : register(b10)
{
    uint BaseInstanceOffset; // C++의 OffsetData.BaseInstanceOffset 값
    // 패딩 등 다른 데이터
};

struct VS_INPUT
{
    float4 position : POSITION; // 버텍스 위치
    float2 texcoord : TEXCOORD;
    int matrixIndex : MATRIXINDEX; // VBO에서 오는 현재 인스턴스의 상대 인덱스
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // 변환된 화면 좌표
    float2 texcoord : TEXCOORD1;
};

StructuredBuffer<InstanceData> InstanceBuffer : register(t10);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
     // 공유 버퍼에서 올바른 인스턴스 데이터 가져오기
    // *** 중요: 현재 배치의 시작 오프셋 + VBO에서 온 상대 인덱스 ***
    InstanceData currentInstanceData = InstanceBuffer[BaseInstanceOffset + (uint) input.matrixIndex];
    //InstanceData currentInstanceData = InstanceBuffer[0];
    float4x4 world = currentInstanceData.WorldMatrix;
    
    output.position = mul(input.position, world);
    //output.position = mul(input.position, MVP);
    
    
    
    
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