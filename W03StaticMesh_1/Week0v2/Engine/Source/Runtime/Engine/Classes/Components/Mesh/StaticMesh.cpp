#include "StaticMesh.h"
#include "Engine/FLoaderOBJ.h"
#include "UObject/ObjectFactory.h"
#include "Define.h"
UStaticMesh::UStaticMesh()
{

}

UStaticMesh::~UStaticMesh()
{
    if (staticMeshRenderData == nullptr) return;

    if (staticMeshRenderData->VertexBuffer) {
        staticMeshRenderData->VertexBuffer->Release();
        staticMeshRenderData->VertexBuffer = nullptr;
    }

    if (staticMeshRenderData->IndexBuffer) {
        staticMeshRenderData->IndexBuffer->Release();
        staticMeshRenderData->IndexBuffer = nullptr;
    }
}

uint32 UStaticMesh::GetMaterialIndex(FName MaterialSlotName) const
{
    for (uint32 materialIndex = 0; materialIndex < materials.Num(); materialIndex++) {
        if (materials[materialIndex]->MaterialSlotName == MaterialSlotName)
            return materialIndex;
    }

    return -1;
}

void UStaticMesh::GetUsedMaterials(TArray<UMaterial*>& Out) const
{
    for (const FStaticMaterial* Material : materials)
    {
        Out.Emplace(Material->Material);
    }
}
void UStaticMesh::SetData(OBJ::FStaticMeshRenderData* renderData)
{
    staticMeshRenderData = renderData;

    uint32 verticeNum = staticMeshRenderData->Vertices.Num();
    if (verticeNum <= 0) return;
    staticMeshRenderData->VertexBuffer = GetEngine().renderer.CreateVertexBuffer(staticMeshRenderData->Vertices, verticeNum * sizeof(FVertexSimple));

    uint32 indexNum = staticMeshRenderData->Indices.Num();
    if (indexNum > 0)
        staticMeshRenderData->IndexBuffer = GetEngine().renderer.CreateIndexBuffer(staticMeshRenderData->Indices, indexNum * sizeof(UINT));


    staticMeshRenderData->VertexBufferLOD.SetNum(staticMeshRenderData->VerticesLOD.Num());
    for (int i = 0; i < staticMeshRenderData->VerticesLOD.Num(); i++)
    {
        uint32 verticeNum = staticMeshRenderData->VerticesLOD[i].Num();
        if (verticeNum <= 0) continue;
        staticMeshRenderData->VertexBufferLOD[i] = GetEngine().renderer.CreateVertexBuffer(staticMeshRenderData->VerticesLOD[i], verticeNum * sizeof(FVertexSimple));
    }

    staticMeshRenderData->IndexBufferLOD.SetNum(staticMeshRenderData->IndicesLOD.Num());

    for (int i = 0; i < staticMeshRenderData->IndicesLOD.Num(); i++)
    {
        uint32 indexNum = staticMeshRenderData->IndicesLOD[i].Num();
        if (indexNum <= 0) continue;
        staticMeshRenderData->IndexBufferLOD[i] = GetEngine().renderer.CreateIndexBuffer(staticMeshRenderData->IndicesLOD[i], indexNum * sizeof(UINT));
    }

    for (int materialIndex = 0; materialIndex < staticMeshRenderData->Materials.Num(); materialIndex++) {


        FStaticMaterial* newMaterialSlot = new FStaticMaterial();
        UMaterial* newMaterial = FManagerOBJ::CreateMaterial(staticMeshRenderData->Materials[materialIndex]);

        newMaterialSlot->Material = newMaterial;
        newMaterialSlot->MaterialSlotName = staticMeshRenderData->Materials[materialIndex].MTLName;



        materials.Add(newMaterialSlot);
    }
}
