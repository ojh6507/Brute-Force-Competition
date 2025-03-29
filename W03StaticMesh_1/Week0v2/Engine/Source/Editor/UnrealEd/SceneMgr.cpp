#include "UnrealEd/SceneMgr.h"
#include "JSON/json.hpp"
#include "UObject/Object.h"
#include "Components/SphereComp.h"
#include "Components/CubeComp.h"
#include "BaseGizmos/GizmoArrowComponent.h"
#include "UObject/ObjectFactory.h"
#include <fstream>
#include "Components/UBillboardComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkySphereComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/Casts.h"
#include "Runtime/Engine/World.h"
#include "Editor/LevelEditor/SLevelEditor.h"
#include "Editor/UnrealEd/EditorViewportClient.h"
#include <Engine/FLoaderOBJ.h>

#include "Math/JungleMath.h"

using json = nlohmann::json;

SceneData FSceneMgr::ParseSceneData(const FString& jsonStr)
{
    SceneData sceneData;

    try {
        json j = json::parse(*jsonStr);

        // 버전과 NextUUID 읽기

        if (j.contains("Version"))
        {
            sceneData.Version = j["Version"].get<int>();
        }
        if (j.contains("NextUUID"))
        {
            sceneData.NextUUID = j["NextUUID"].get<int>();
        }

        FBoundingBox WorldBoundingBox;
        WorldBoundingBox.max = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX); // float의 최소값으로 초기화
        WorldBoundingBox.min = FVector(FLT_MAX, FLT_MAX, FLT_MAX);   // float의 최대값으로 초기화

        int a=0;
        
        // Primitives 처리 (C++14 스타일)
        auto primitives = j["Primitives"];
        for (auto it = primitives.begin(); it != primitives.end(); ++it)
        {
            int id = std::stoi(it.key());  // Key는 문자열, 숫자로 변환
            const json& value = it.value();
            UObject* obj = nullptr;
            FBoundingBox LocalMeshBox;
            
            if (value.contains("Type"))
            {
                const FString TypeName = value["Type"].get<std::string>();

                if (TypeName == "StaticMeshComp")
                {
                    obj = FObjectFactory::ConstructObject<UStaticMeshComponent>();
                    if (value.contains("ObjStaticMeshAsset"))
                    {
                        UStaticMeshComponent* staticMeshComp = static_cast<UStaticMeshComponent*>(obj);
                        FString  str = value["ObjStaticMeshAsset"].get<std::string>();
                        UStaticMesh* Mesh = FManagerOBJ::CreateStaticMesh(str);
                        staticMeshComp->SetStaticMesh(Mesh);
                        
                        LocalMeshBox = staticMeshComp->GetBoundingBox(); 
                    }

                }
            }

            USceneComponent* sceneComp = static_cast<USceneComponent*>(obj);
            sceneData.BoundingBox = WorldBoundingBox;

            if (value.contains("Location"))
            {
                sceneComp->SetLocation(FVector(value["Location"].get<std::vector<float>>()[0],
                value["Location"].get<std::vector<float>>()[1],
                value["Location"].get<std::vector<float>>()[2]));
                
            }
            if (value.contains("Rotation"))
            {
                sceneComp->SetRotation(FVector(value["Rotation"].get<std::vector<float>>()[0],
                value["Rotation"].get<std::vector<float>>()[1],
                value["Rotation"].get<std::vector<float>>()[2]));
            }
            if (value.contains("Scale")) {
                sceneComp->SetScale(FVector(value["Scale"].get<std::vector<float>>()[0],
                value["Scale"].get<std::vector<float>>()[1],
                value["Scale"].get<std::vector<float>>()[2]));
            }
            if (value.contains("Type")) {
                UPrimitiveComponent* primitiveComp = Cast<UPrimitiveComponent>(sceneComp);
                if (primitiveComp) {
                    primitiveComp->SetType(value["Type"].get<std::string>());
                }
                else {
                    std::string name = value["Type"].get<std::string>();
                    sceneComp->NamePrivate = name.c_str();
                }
            }
            sceneData.Primitives[id] = sceneComp;

            // 2. 월드 변환 행렬을 계산합니다.
            FMatrix ModelMatrix = JungleMath::CreateModelMatrix(
                sceneComp->GetWorldLocation(),
                sceneComp->GetWorldRotation(),
                sceneComp->GetWorldScale()
            );

            // 3. 로컬 바운딩 박스의 8개 꼭짓점을 계산합니다.
            FVector LocalCorners[8];
            LocalCorners[0] = FVector(LocalMeshBox.min.x, LocalMeshBox.min.y, LocalMeshBox.min.z);
            LocalCorners[1] = FVector(LocalMeshBox.max.x, LocalMeshBox.min.y, LocalMeshBox.min.z);
            LocalCorners[2] = FVector(LocalMeshBox.min.x, LocalMeshBox.max.y, LocalMeshBox.min.z);
            LocalCorners[3] = FVector(LocalMeshBox.max.x, LocalMeshBox.max.y, LocalMeshBox.min.z);
            LocalCorners[4] = FVector(LocalMeshBox.min.x, LocalMeshBox.min.y, LocalMeshBox.max.z);
            LocalCorners[5] = FVector(LocalMeshBox.max.x, LocalMeshBox.min.y, LocalMeshBox.max.z);
            LocalCorners[6] = FVector(LocalMeshBox.min.x, LocalMeshBox.max.y, LocalMeshBox.max.z);
            LocalCorners[7] = FVector(LocalMeshBox.max.x, LocalMeshBox.max.y, LocalMeshBox.max.z);

            // 4. 8개의 꼭짓점을 월드 좌표로 변환하고, 변환된 점들로부터 새로운 Min/Max를 찾습니다.
            FVector TransformedCornersMin(FLT_MAX, FLT_MAX, FLT_MAX);
            FVector TransformedCornersMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            for (int i = 0; i < 8; ++i)
            {
                // 각 로컬 꼭짓점을 월드 좌표로 변환합니다.
                // ModelMatrix에 TransformPosition 함수가 정의되어 있다고 가정합니다.
                // (이전 질문에서 보여주신 함수 사용)
                FVector WorldCorner = ModelMatrix.TransformPosition(LocalCorners[i]);

                // 변환된 꼭짓점을 기준으로 Min/Max를 갱신합니다.
                TransformedCornersMin.x = FMath::Min(TransformedCornersMin.x, WorldCorner.x);
                TransformedCornersMin.y = FMath::Min(TransformedCornersMin.y, WorldCorner.y);
                TransformedCornersMin.z = FMath::Min(TransformedCornersMin.z, WorldCorner.z);

                TransformedCornersMax.x = FMath::Max(TransformedCornersMax.x, WorldCorner.x);
                TransformedCornersMax.y = FMath::Max(TransformedCornersMax.y, WorldCorner.y);
                TransformedCornersMax.z = FMath::Max(TransformedCornersMax.z, WorldCorner.z);
            }

            // 이제 TransformedCornersMin과 TransformedCornersMax가 이 컴포넌트의 정확한 월드 AABB입니다.

            // 5. 이 컴포넌트의 월드 AABB를 사용하여 전체 월드 바운딩 박스를 확장합니다.
            WorldBoundingBox.min.x = FMath::Min(WorldBoundingBox.min.x, TransformedCornersMin.x);
            WorldBoundingBox.min.y = FMath::Min(WorldBoundingBox.min.y, TransformedCornersMin.y);
            WorldBoundingBox.min.z = FMath::Min(WorldBoundingBox.min.z, TransformedCornersMin.z);

            WorldBoundingBox.max.x = FMath::Max(WorldBoundingBox.max.x, TransformedCornersMax.x);
            WorldBoundingBox.max.y = FMath::Max(WorldBoundingBox.max.y, TransformedCornersMax.y);
            WorldBoundingBox.max.z = FMath::Max(WorldBoundingBox.max.z, TransformedCornersMax.z);
        }
        
        auto perspectiveCamera = j["PerspectiveCamera"];
        std::shared_ptr<FEditorViewportClient> ViewPort = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();

        if (perspectiveCamera.contains("Location"))
            ViewPort->ViewTransformPerspective.SetLocation(FVector(perspectiveCamera["Location"].get<std::vector<float>>()[0],
            perspectiveCamera["Location"].get<std::vector<float>>()[1],
            perspectiveCamera["Location"].get<std::vector<float>>()[2]));
        if (perspectiveCamera.contains("Rotation")) 
            ViewPort->ViewTransformPerspective.SetRotation(FVector(perspectiveCamera["Rotation"].get<std::vector<float>>()[0],
            perspectiveCamera["Rotation"].get<std::vector<float>>()[1],
            perspectiveCamera["Rotation"].get<std::vector<float>>()[2]));
        if (perspectiveCamera.contains("FOV")) 
            ViewPort->ViewFOV = (perspectiveCamera["FOV"].get<std::vector<float>>()[0]);
        if (perspectiveCamera.contains("NearClip")) 
            ViewPort->nearPlane = (perspectiveCamera["NearClip"].get<std::vector<float>>()[0]);
        if (perspectiveCamera.contains("FarClip")) 
            ViewPort->farPlane = (perspectiveCamera["FarClip"].get<std::vector<float>>()[0]);

        ViewPort->UpdateMatrix();

    }
    catch (const std::exception& e) {
        FString errorMessage = "Error parsing JSON: ";
        errorMessage += e.what();

        UE_LOG(LogLevel::Error, "No Json file");
    }

    return sceneData;
}

FString FSceneMgr::LoadSceneFromFile(const FString& filename)
{
    std::ifstream inFile(*filename);
    if (!inFile) {
        UE_LOG(LogLevel::Error, "Failed to open file for reading: %s", *filename);
        return FString();
    }

    json j;
    try {
        inFile >> j; // JSON 파일 읽기
    }
    catch (const std::exception& e) {
        UE_LOG(LogLevel::Error, "Error parsing JSON: %s", e.what());
        return FString();
    }

    inFile.close();

    return j.dump(4);
}

std::string FSceneMgr::SerializeSceneData(const SceneData& sceneData)
{
    json j;

    // Version과 NextUUID 저장
    j["Version"] = sceneData.Version;
    j["NextUUID"] = sceneData.NextUUID;

    // Primitives 처리 (C++17 스타일)
    for (const auto& [Id, Obj] : sceneData.Primitives)
    {
        USceneComponent* primitive = static_cast<USceneComponent*>(Obj);
        std::vector<float> Location = { primitive->GetWorldLocation().x,primitive->GetWorldLocation().y,primitive->GetWorldLocation().z };
        std::vector<float> Rotation = { primitive->GetWorldRotation().x,primitive->GetWorldRotation().y,primitive->GetWorldRotation().z };
        std::vector<float> Scale = { primitive->GetWorldScale().x,primitive->GetWorldScale().y,primitive->GetWorldScale().z };

        std::string primitiveName = *primitive->GetName();
        size_t pos = primitiveName.rfind('_');
        if (pos != INDEX_NONE) {
            primitiveName = primitiveName.substr(0, pos);
        }
        j["Primitives"][std::to_string(Id)] = {
            {"Location", Location},
            {"Rotation", Rotation},
            {"Scale", Scale},
            {"Type",primitiveName}
        };
    }

    for (const auto& [id, camera] : sceneData.Cameras)
    {
        UCameraComponent* cameraComponent = static_cast<UCameraComponent*>(camera);
        TArray<float> Location = { cameraComponent->GetWorldLocation().x, cameraComponent->GetWorldLocation().y, cameraComponent->GetWorldLocation().z };
        TArray<float> Rotation = { 0.0f, cameraComponent->GetWorldRotation().y, cameraComponent->GetWorldRotation().z };
        float FOV = cameraComponent->GetFOV();
        float nearClip = cameraComponent->GetNearClip();
        float farClip = cameraComponent->GetFarClip();
    
        //
        j["PerspectiveCamera"][std::to_string(id)] = {
            {"Location", Location},
            {"Rotation", Rotation},
            {"FOV", FOV},
            {"NearClip", nearClip},
            {"FarClip", farClip}
        };
    }


    return j.dump(4); // 4는 들여쓰기 수준
}

bool FSceneMgr::SaveSceneToFile(const FString& filename, const SceneData& sceneData)
{
    std::ofstream outFile(*filename);
    if (!outFile) {
        FString errorMessage = "Failed to open file for writing: ";
        MessageBoxA(NULL, *errorMessage, "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    std::string jsonData = SerializeSceneData(sceneData);
    outFile << jsonData;
    outFile.close();

    return true;
}

