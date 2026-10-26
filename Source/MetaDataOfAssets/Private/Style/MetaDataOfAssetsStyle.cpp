#include "Style/MetaDataOfAssetsStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Log/MetaDataOfAssetsLogChannel.h"

TSharedPtr< FSlateStyleSet > FMetaDataOfAssetsStyle::StyleInstance = NULL;
const FVector2D FMetaDataOfAssetsStyle::Icon128X128(128.0f, 128.0f);

void FMetaDataOfAssetsStyle::Initialize()
{
	// 初始化时创建单例并注册
	if (!StyleInstance.IsValid())
	{
		StyleInstance = CreateStyle();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMetaDataOfAssetsStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FMetaDataOfAssetsStyle::GetStyleSetName()
{
	// 使用静态变量，避免每次调用都构造 FName
	static FName StyleSetName(TEXT("MetadataOfAssetsStyle"));
	return StyleSetName;
}

FSlateStyleSet& FMetaDataOfAssetsStyle::Get()
{
	check(StyleInstance.IsValid());
	return *StyleInstance;
}

FString FMetaDataOfAssetsStyle::GetPluginResourcesDir(const FString& ModuleName)
{
	// 查找插件
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(ModuleName);
	if (!Plugin.IsValid())
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : 找不到插件 %s"), __FUNCTION__, *ModuleName);
		return TEXT("");
	}

	// 返回插件的 Resources 目录
	return Plugin->GetBaseDir() / TEXT("Resources");
}

TSharedRef< FSlateStyleSet > FMetaDataOfAssetsStyle::CreateStyle()
{
	// 创建一个新的 Style 实例
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	// 获得插件的 Resources 目录的路径
	FString PluginResourcesDirectoryPath = GetPluginResourcesDir(TEXT("MetaDataOfAssets"));
	
	// 设置资源根目录为 Resources 目录
	Style->SetContentRoot(PluginResourcesDirectoryPath);

	// 设置图标，命令集名称 + FUICommandInfo 变量名
	Style->Set(FName(TEXT("MetadataOfAssets.Command_OpenMain")), new FSlateImageBrush(PluginResourcesDirectoryPath + TEXT("/Icon128.png"), Icon128X128));

	return Style;
}
