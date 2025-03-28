#include "UnrealEd.h"
#include "EditorPanel.h"
#include "PropertyEditor/OutlinerEditorPanel.h"
#include "PropertyEditor/PropertyEditorPanel.h"

void UnrealEd::Initialize()
{
    //ControlPanel = std::make_shared<ControlEditorPanel>();
}

void UnrealEd::Render() const
{
    //ControlPanel->Render();
}

void UnrealEd::AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel)
{
   
}

void UnrealEd::OnResize(HWND hWnd) const
{
    //ControlPanel->OnResize(hWnd);
}

std::shared_ptr<UEditorPanel> UnrealEd::GetEditorPanel(const FString& PanelId)
{
    return {};
}
