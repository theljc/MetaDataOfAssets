// Copyright 2026 ljc

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/Ticker.h"
#include "Widgets/Notifications/SNotificationList.h"

struct FStreamableHandle;
class UEUSS_MetaDataManager;
class SProgressBarNotification_ScanAsset;
class STestProgressBarNotification;


class FAssetSyncTask : public TSharedFromThis<FAssetSyncTask>
{
public:
	
	explicit FAssetSyncTask(const TArray<FAssetData>& InAssets)
		: Assets(InAssets)
	{
	}

	// 开始任务
	void Start();

	// 取消任务
	void Cancel();

private:

	// 更新进度条
	bool Tick(float DeltaTime);
	// 结束任务
	void Finish(const bool bSuccess);
	// 加载完成时
	void OnLoadComplete();

	// 资产列表
	TArray<FAssetData> Assets;

	// 进度条控件
	TSharedPtr<SProgressBarNotification_ScanAsset> ProgressWidget;
	// 进度条通知
	TSharedPtr<SNotificationItem> ProgressNotification;
	
	FTSTicker::FDelegateHandle TickHandle;
	TSharedPtr<FStreamableHandle> StreamableHandle;

	// 防止重复执行
	bool bFinished = false;
	
};
