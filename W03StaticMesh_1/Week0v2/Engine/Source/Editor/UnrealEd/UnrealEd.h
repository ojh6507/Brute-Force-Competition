#pragma once
#include "Container/Map.h"
#include "Container/String.h"
#include "PropertyEditor/ControlEditorPanel.h"
class UEditorPanel;

class UnrealEd
{
public:
    UnrealEd() = default;
    ~UnrealEd() = default;
    void Initialize();
    
     void Render() const;
     void OnResize(HWND hWnd) const;
    
    void AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel);
    std::shared_ptr<UEditorPanel> GetEditorPanel(const FString& PanelId);

private:
    std::shared_ptr<ControlEditorPanel> ControlPanel;
};
