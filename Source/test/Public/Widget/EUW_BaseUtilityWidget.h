#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "WidgetInterface_MetaDataPlugin.h"
#include "EUW_BaseUtilityWidget.generated.h"

struct FInstancedStruct;

/**
 * 本插件的所有 EditorUtilityWidget 都继承自该基类
 */
UCLASS(BlueprintType, Blueprintable)
class TEST_API UEUW_BaseUtilityWidget : public UEditorUtilityWidget, public IWidgetInterface_MetaDataPlugin
{
	GENERATED_BODY()

protected:
	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	
// IWidgetInterface
	virtual void OnOpen_Implementation(const FInstancedStruct& Params) override;
	
	virtual void OnClose_Implementation() override;

	virtual void OnInitialize(const FInstancedStruct& Params) override;

	virtual void OnDeinitialize() override;
// End IWidgetInterface
	
};
