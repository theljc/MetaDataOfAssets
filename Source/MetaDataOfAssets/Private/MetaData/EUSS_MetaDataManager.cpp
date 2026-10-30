// Copyright 2026 ljc

#include "MetaData/EUSS_MetaDataManager.h"
#include "EditorAssetLibrary.h"
#include "Settings/MetaDataOfAssetsSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/AssetManagerSettings.h"
#include "Log/MetaDataOfAssetsLogChannel.h"
#include "Task/FAssetScanTask.h"


void UEUSS_MetaDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UMetaDataOfAssetsSettings* Settings = GetDefault<UMetaDataOfAssetsSettings>();
	if (!IsValid(Settings)) return;

	// 只有编辑器重启后才重新赋值
	bAllowManageAssetRegistryTagsDelete = Settings->bAllowManageAssetRegistryTagsDelete;
	
}

void UEUSS_MetaDataManager::Deinitialize()
{
	if (AssetSyncTask.IsValid())
	{
		AssetSyncTask->Cancel();
		AssetSyncTask.Reset();
	}
	
	Super::Deinitialize();
}

TArray<FScanDirectorySetting> UEUSS_MetaDataManager::GetNormalizedPathSettings(const TArray<FScanDirectorySetting>& Settings)
{
	TArray<FScanDirectorySetting> Result;

	if (Settings.IsEmpty()) return Result;

	TArray<FScanDirectorySetting> NormalizedSettings;
	NormalizedSettings.Reserve(Settings.Num());

	// 规范化路径分隔符的格式
	for (const FScanDirectorySetting& PathSetting : Settings)
	{
		FScanDirectorySetting Normalized = PathSetting;
		FPaths::NormalizeDirectoryName(Normalized.Directory.Path);
		NormalizedSettings.Add(MoveTemp(Normalized));
	}

	// 按路径长度排序，短路径优先
	NormalizedSettings.Sort([](const FScanDirectorySetting& A, const FScanDirectorySetting& B)
	{
		return A.Directory.Path.Len() < B.Directory.Path.Len();
	});

	// 过滤掉被父目录递归扫描覆盖的子目录
	for (const FScanDirectorySetting& Current : NormalizedSettings)
	{
		bool bIsCovered = false;
		for (const FScanDirectorySetting& Effective : Result)
		{
			if (Effective.bRecursive &&
				FPaths::IsUnderDirectory(Current.Directory.Path, Effective.Directory.Path))
			{
				bIsCovered = true;
				break;
			}
		}

		if (!bIsCovered)
		{
			Result.Add(Current);
		}
	}

	return Result;
}

void UEUSS_MetaDataManager::SyncAssetsInDirectory()
{
	// 是否允许管理元数据删除，不允许则不需要扫描资产
	if (!bAllowManageAssetRegistryTagsDelete) return;
	
	const UMetaDataOfAssetsSettings* Settings = GetDefault<UMetaDataOfAssetsSettings>();
	if (!IsValid(Settings)) return;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	// 获得规范化的路径
	TArray<FScanDirectorySetting> NormalizedPathSettings = GetNormalizedPathSettings(Settings->ScanSettings);

	TArray<FAssetData> AssetDataList;

	// 获得路径下所有的资产数据
	for (FScanDirectorySetting& Setting : NormalizedPathSettings)
	{
		if (Setting.Directory.Path.IsEmpty()) continue;
		
		TArray<FAssetData> AssetDataList_Sub;
		AssetRegistry.GetAssetsByPath(FName(*Setting.Directory.Path), AssetDataList_Sub, Setting.bRecursive);

		AssetDataList.Append(AssetDataList_Sub);
	}
	
	// 防止重复启动任务
	if (AssetSyncTask.IsValid() || AssetDataList.IsEmpty()) return;

	// 创建并启动资产同步任务
	AssetSyncTask = MakeShared<FAssetSyncTask>(AssetDataList);
	AssetSyncTask->Start();
	
}

void UEUSS_MetaDataManager::OnTaskFinished(FAssetSyncTask* FinishedTask)
{
	// 任务结束后，重置指针
	if (AssetSyncTask.IsValid() && AssetSyncTask.Get() == FinishedTask)
	{
		AssetSyncTask.Reset();
	}
}

void UEUSS_MetaDataManager::RemoveSavedMetaData(UObject* Asset, TMap<FName, FString>* TagMapToAdd)
{
	for (const auto& SavedMetaData : *TagMapToAdd)
	{
		RemoveTagFromRegisteredTags(SavedMetaData.Key, Asset);
	}
}

void UEUSS_MetaDataManager::AddActualMetaData(UObject* Asset, const TMap<FName, FString>& ObjectMetaDataMap, TMap<FName, FString>* TagMapToAdd)
{
	for (auto& CurrentMetaData : ObjectMetaDataMap)
	{
		AddTagToRegisteredTags(CurrentMetaData.Key, Asset);
		TagMapToAdd->Add(CurrentMetaData.Key, CurrentMetaData.Value);
	}
}

void UEUSS_MetaDataManager::UpdateAssetTagStates(UObject* Asset)
{
	if (!IsValid(Asset)) return;

	TMap<FName, FString> ObjectMetaDataMap = UEditorAssetLibrary::GetMetadataTagValues(Asset);
	
	// 构建 FSoftObjectPath
	FSoftObjectPath AssetPath(Asset);

	// 查找资产已保存的元数据
	FMetaDataMap* TagMapToAdd = AssetTagStates.Find(AssetPath);

	// ObjectMetaDataMap 为空，表示这个资产当前没有元数据
	if (ObjectMetaDataMap.IsEmpty())
	{
		// !TagMapToAdd 为 true 时，表示当前资产没有被同步过（同步后会添加到 AssetTagStates 中）
		if (!TagMapToAdd)
		{
			// 添加空值即可，表示此资产已经被同步过
			AssetTagStates.Add(AssetPath);
		}
		// 被同步过
		else
		{
			// IsEmpty 为 true，表示上一次同步资产的时候，资产里没有元数据
			if (TagMapToAdd->Map.IsEmpty()) return;

			// 资产实际没有元数据，但保存的 TagMapToAdd 中有元数据，直接移除保存的所有旧的元数据
			RemoveSavedMetaData(Asset, &TagMapToAdd->Map);
		}
	}
	// 有元数据
	else
	{
		// 没有被同步过
		if (!TagMapToAdd)
		{
			// 资产实际有元数据，但没有被同步过，则同步并添加到 AssetRegistry 中
			FMetaDataMap& TagMap = AssetTagStates.Add(AssetPath);
			AddActualMetaData(Asset, ObjectMetaDataMap, &TagMap.Map);
		}
		// 被同步过
		else
		{
			// 上一次同步时没有元数据，则添加资产实际的元数据
			if (TagMapToAdd->Map.IsEmpty())
			{
				AddActualMetaData(Asset, ObjectMetaDataMap, &TagMapToAdd->Map);
			}
			// 上次同步时有元数据，而资产实际也有元数据，则需要更新
			else
			{
				// 删除所有保存的旧的元数据
				RemoveSavedMetaData(Asset, &TagMapToAdd->Map);
				
				// 添加资产实际的元数据
				AddActualMetaData(Asset, ObjectMetaDataMap, &TagMapToAdd->Map);
			}
		}
	}
	
}

void UEUSS_MetaDataManager::RemoveFromAssetTagStates(UObject* Asset)
{
	if (!IsValid(Asset)) return;

	// 实际的当前资产的元数据
	TMap<FName, FString> ObjectMetaDataMap = UEditorAssetLibrary::GetMetadataTagValues(Asset);

	// 构建 FSoftObjectPath
	FSoftObjectPath AssetPath(Asset);
	
	FMetaDataMap* TagMapToRemove = AssetTagStates.Find(AssetPath);
	
	// 表示这个资产当前没有元数据
	if (ObjectMetaDataMap.IsEmpty())
	{
		// 没有被同步过（不需要被管理）
		if (!TagMapToRemove) return;
	}
	// 有元数据
	else
	{
		// 没有被同步过
		if (!TagMapToRemove)
		{
			// Note:如果从未同步过，无法处理，资产必须同步后才能被管理
			return;
		}
	}
	
	// 被同步过，则移除保存的旧的元数据，移除时会根据引用计数判断是否需要从 AssetRegistry 中移除标签
	for (const auto& SavedMetaData : TagMapToRemove->Map)
	{
		RemoveTagFromRegisteredTags(SavedMetaData.Key, Asset);
	}
	
	// 移除对此资产的管理
	AssetTagStates.Remove(AssetPath);

}

void UEUSS_MetaDataManager::OnAssetRemoved(const FAssetData& AssetData)
{
	// 获取被删除的资产
	UObject* AssetToRemove = AssetData.GetAsset();
	
	if (bAllowManageAssetRegistryTagsDelete)
	{
		// 资产被删除后，包含的元数据也从 Manager 的管理中移除
		RemoveFromAssetTagStates(AssetToRemove);
	}

	// 资产删除通知
	OnAssetDeleted.Broadcast(AssetToRemove);

}

void UEUSS_MetaDataManager::AddMetaData(UObject* Asset, FName TagToAdd, FString ValueToAdd)
{
	if (!IsValid(Asset)) return;

	// 添加元数据
	UEditorAssetLibrary::SetMetadataTag(Asset, TagToAdd, ValueToAdd);

	if (bAllowManageAssetRegistryTagsDelete)
	{
		FSoftObjectPath AssetPath(Asset);

		FMetaDataMap& TagMap = AssetTagStates.FindOrAdd(AssetPath);
		FString& Value = TagMap.Map.FindOrAdd(TagToAdd);
		Value = ValueToAdd;
		
		// 每次添加都同步元数据
		SyncAsset(Asset);
	}
	else
	{
		// 仅添加模式，添加到 AssetRegistry
		AddTagToRegisteredTags(TagToAdd, Asset);
		UEditorAssetLibrary::SaveLoadedAsset(Asset, false);
	}
	
}

void UEUSS_MetaDataManager::ModifyMetaData(UObject* Asset, FName TagToAdd, FString ValueToAdd)
{
	if (!IsValid(Asset)) return;

	// 修改元数据
	UEditorAssetLibrary::SetMetadataTag(Asset, TagToAdd, ValueToAdd);

	if (bAllowManageAssetRegistryTagsDelete)
	{
		FSoftObjectPath AssetPath(Asset);

		FMetaDataMap& TagMap = AssetTagStates.FindOrAdd(AssetPath);
		FString& Value = TagMap.Map.FindOrAdd(TagToAdd);
		Value = ValueToAdd;
		
		SyncAsset(Asset);
	}
	else
	{
		UEditorAssetLibrary::SaveLoadedAsset(Asset, false);
	}
	
	OnMetaDataModified.Broadcast(Asset);
}

void UEUSS_MetaDataManager::DeleteMetaData(UObject* Asset, FName TagToAdd)
{
	if (!IsValid(Asset)) return;

	// 移除元数据
	UEditorAssetLibrary::RemoveMetadataTag(Asset, TagToAdd);

	if (bAllowManageAssetRegistryTagsDelete)
	{
		FSoftObjectPath AssetPath(Asset);
		
		FMetaDataMap* TagMap = AssetTagStates.Find(AssetPath);

		// 删除时没有被同步过
		if (!TagMap) return;

		SyncAsset(Asset);
	}
	else
	{
		UEditorAssetLibrary::SaveLoadedAsset(Asset, false);
	}
	
	OnMetaDataDeleted.Broadcast(Asset);
}

void UEUSS_MetaDataManager::CopyMetaData(TArray<UObject*> SourceAsset, TArray<UObject*> TargetAsset)
{
	if (SourceAsset.Num() == 0 || TargetAsset.Num() == 0) return;

	// 遍历源资产和目标资产
	for (UObject* SourceAssetObj : SourceAsset)
	{
		for (UObject* TargetAssetObj : TargetAsset)
		{
			// 获得源资产和目标资产的元数据
			TMap<FName, FString> SourceAssetMetaDatas = UEditorAssetLibrary::GetMetadataTagValues(SourceAssetObj);
			TMap<FName, FString> TargetAssetMetaDatas = UEditorAssetLibrary::GetMetadataTagValues(TargetAssetObj);
			
			for (const auto& SourceAssetMetaData : SourceAssetMetaDatas)
			{
				// 只复制 TargetAsset 中没有的元数据
				if (!TargetAssetMetaDatas.Contains(SourceAssetMetaData.Key))
				{
					AddMetaData(TargetAssetObj, SourceAssetMetaData.Key, SourceAssetMetaData.Value);
				}
			}

			// 每次复制都触发广播
			OnMetaDataCopied.Broadcast(SourceAssetObj, TargetAssetObj);
		}
	}
	
}

void UEUSS_MetaDataManager::SyncAsset(UObject* Asset)
{
	if (bAllowManageAssetRegistryTagsDelete)
	{
		// 非仅添加模式，需要更新 AssetTagStates
		UpdateAssetTagStates(Asset);
	}
	else
	{
		// 仅添加模式，直接添加并更新 AssetRegistry
		TMap<FName, FString> ObjectMetaDataMap = UEditorAssetLibrary::GetMetadataTagValues(Asset);
		for (auto& Pair : ObjectMetaDataMap)
		{
			AddTagToAssetRegistry(Pair.Key);
		}
	}
	
	UEditorAssetLibrary::SaveLoadedAsset(Asset, false);
	OnAssetSynced.Broadcast(Asset);
}

void UEUSS_MetaDataManager::AddTagToRegisteredTags(const FName TagToAdd, UObject* Asset)
{
	if (TagToAdd.IsNone() || !IsValid(Asset))
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : TagToAdd 为空或 Asset 无效"), __FUNCTION__);
		return;
	}

	// 仅添加模式，不增加引用计数
	if (!bAllowManageAssetRegistryTagsDelete)
	{
		AddTagToAssetRegistry(TagToAdd);
		
		OnMetaDataAdded.Broadcast(Asset);
		
		return;
	}

	int32& Count = RegisteredTags.FindOrAdd(TagToAdd);
	Count++;
	
	// 注册 Tag 到 AssetRegistry
	AddTagToAssetRegistry(TagToAdd);

	OnMetaDataAdded.Broadcast(Asset);

}

void UEUSS_MetaDataManager::RemoveTagFromRegisteredTags(const FName TagToRemove, UObject* Asset)
{
	if (TagToRemove.IsNone() || !IsValid(Asset))
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : TagToRemove 为空或 Asset 无效"), __FUNCTION__);
		return;
	}
	
	// 仅添加模式，不从 AssetRegistry 中移除
	if (!bAllowManageAssetRegistryTagsDelete)
	{
		OnMetaDataDeleted.Broadcast(Asset);
		return;
	}

	// 获得当前 Tag 的引用计数
	int32* CountPtr = RegisteredTags.Find(TagToRemove);
	if (!CountPtr) return;

	int32& Count = *CountPtr;
	if (Count <= 0)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : RegisteredTags 中 %s 的引用计数为 %d"), __FUNCTION__, *TagToRemove.ToString(), Count);
		RegisteredTags.Remove(TagToRemove);
		return;
	}

	// 引用计数减 1
	Count--;
	if (Count == 0)
	{
		RegisteredTags.Remove(TagToRemove);
		
		// 引用计数归零，从 AssetRegistry 中删除
		RemoveTagFromAssetRegistry(TagToRemove, Asset);
		
	}

	OnMetaDataDeleted.Broadcast(Asset);

}

void UEUSS_MetaDataManager::AddTagToAssetRegistry(const FName TagToAdd)
{
	// 获得 UAssetManagerSettings
	UAssetManagerSettings* Settings = GetMutableDefault<UAssetManagerSettings>();
	if (!Settings)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : UAssetManagerSettings 无效"), __FUNCTION__);
		return;
	}

	// 标签不存在则添加
	if (!Settings->MetaDataTagsForAssetRegistry.Contains(TagToAdd))
	{
		// 添加到配置源
		Settings->MetaDataTagsForAssetRegistry.Add(TagToAdd);
		
		// 添加到全局缓存
		TSet<FName>& GlobalTagsForAssetRegistry = UObject::GetMetaDataTagsForAssetRegistry();
		if (!GlobalTagsForAssetRegistry.Contains(TagToAdd))
		{
			GlobalTagsForAssetRegistry.Add(TagToAdd);
		}
		
		// 更新配置文件
		Settings->TryUpdateDefaultConfigFile();
	}
	
}

void UEUSS_MetaDataManager::RemoveTagFromAssetRegistry(const FName TagToRemove, UObject* Asset)
{
	UAssetManagerSettings* Settings = GetMutableDefault<UAssetManagerSettings>();
	if (!Settings)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : UAssetManagerSettings 无效"), __FUNCTION__);
		return;
	}

	// 标签存在则移除
	if (Settings->MetaDataTagsForAssetRegistry.Contains(TagToRemove))
	{
		// 从配置源移除
		Settings->MetaDataTagsForAssetRegistry.Remove(TagToRemove);

		// 从全局缓存移除
		TSet<FName>& GlobalTagsForAssetRegistry = UObject::GetMetaDataTagsForAssetRegistry();
		GlobalTagsForAssetRegistry.Remove(TagToRemove);

		// 更新配置文件
		Settings->TryUpdateDefaultConfigFile();
	}

}
