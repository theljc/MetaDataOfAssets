#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WidgetInterface_MetaDataPlugin.h"
#include "UW_BaseDataWidget.generated.h"

struct FInstancedStruct;

/**
 * 本插件的所有 UserWidget 都继承自该基类
 */
UCLASS(BlueprintType, Blueprintable)
class TEST_API UUW_BaseDataWidget : public UUserWidget, public IWidgetInterface_MetaDataPlugin
{
	GENERATED_BODY()

protected:
	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

// IWidgetInterface
	virtual void OnOpen_Implementation(const FInstancedStruct& Params) override;
	
	virtual void OnClose_Implementation() override;

	virtual void OnInitialize(const FInstancedStruct& Params) override;
	
	virtual void OnDeinitialize() override;
// End IWidgetInterface
	
};
