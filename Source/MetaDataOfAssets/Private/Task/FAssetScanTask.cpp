// Copyright 2026 ljc. All rights reserved.

#include "MetaDataOfAssets/Public/Task/FAssetScanTask.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Editor.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Global/MetaDataOfAssetsGlobal.h"
#include "Log/MetaDataOfAssetsLogChannel.h"
#include "MetaData/EUSS_MetaDataManager.h"
#include "Task/SProgressBarNotification_ScanAsset.h"
#include "Widget/EUSS_WidgetManager.h"
#include "Misc/ScopeExit.h"


void FAssetSyncTask::Start()
{
    // 创建一个强引用，防止执行时被释放
    // 局部变量，出函数后释放
    TSharedRef<FAssetSyncTask> KeepAlive = AsShared();
    
    if (bFinished) return;
    
    UEUSS_WidgetManager* WidgetManager = MetaDataOfAssetsGlobal::GetWidgetManagerSafe();
    
    if (Assets.Num() == 0 || !WidgetManager)
    {
        Finish(false);
        return;
    }

    WidgetManager->OnTaskStart();
    
    // 进度条控件
    ProgressWidget = SNew(SProgressBarNotification_ScanAsset)
        .ProgressText(NSLOCTEXT("AssetSyncTask", "LoadingAssetProgress", "Loading Assets"));
    
    // 创建通知信息
    FNotificationInfo Info(ProgressWidget);
    Info.bFireAndForget = false;
    Info.bAllowThrottleWhenFrameRateIsLow = false;
    Info.ExpireDuration = 0.0f;

    // 添加通知
    ProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);

    TArray<FSoftObjectPath> LoadedItems;
    for (FAssetData& Asset : Assets)
    {
        LoadedItems.AddUnique(Asset.GetSoftObjectPath());
    }

    TWeakPtr<FAssetSyncTask> WeakTask = AsShared();
    
    // 加载资产
    FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();
    StreamableHandle = Streamable.RequestAsyncLoad(
        LoadedItems,
        FStreamableDelegate::CreateLambda([WeakTask]()
        {
            if (TSharedPtr<FAssetSyncTask> Task = WeakTask.Pin())
            {
                // 加载完成后的回调
                Task->OnLoadComplete();
            }
        })
    );

    if (!StreamableHandle.IsValid())
    {
        Finish(false);
    }
}

void FAssetSyncTask::Finish(const bool bSuccess)
{
    TSharedRef<FAssetSyncTask> KeepAlive = AsShared();
    
    if (bFinished) return;
    bFinished = true;
    Phase = EPhase::Finished;
    
    UEUSS_MetaDataManager* MetaDataManager = MetaDataOfAssetsGlobal::GetMetaDataManagerSafe();
    UEUSS_WidgetManager* WidgetManager = MetaDataOfAssetsGlobal::GetWidgetManagerSafe();
    
    ON_SCOPE_EXIT
    {
        if (MetaDataManager)
        {
            MetaDataManager->OnTaskFinished(this);
        }
        
        if (WidgetManager)
        {
            WidgetManager->OnTaskFinished();
        }
    };
    
    if (SyncTickHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(SyncTickHandle);
        SyncTickHandle.Reset();
    }
    
    if (StreamableHandle.IsValid())
    {
        StreamableHandle->CancelHandle();
        StreamableHandle.Reset();
    }

    // 关闭通知
    if (ProgressNotification.IsValid())
    {
        ProgressNotification->ExpireAndFadeout();
        ProgressNotification.Reset();
    }

    ProgressWidget.Reset();

    if (bSuccess)
    {
        UE_LOG(
            LogMetaDataOfAssets,
            Log,
            TEXT("[%hs] 资产同步完成: 成功 %d, 失败 %d, 总数 %d"),
            __FUNCTION__,
            SyncedAssetCount,
            FailedAssetCount,
            Assets.Num());
    }
    else
    {
        UE_LOG(LogMetaDataOfAssets, Warning, TEXT("[%hs] 资产同步任务已取消"), __FUNCTION__);
    }
}

void FAssetSyncTask::OnLoadComplete()
{
    /** 资产加载完成后，开始同步资产 */
    if (bFinished || Phase != EPhase::Loading) return;
    
    Phase = EPhase::Syncing;

    if (ProgressWidget.IsValid())
    {
        ProgressWidget->SetProgressText(
            NSLOCTEXT("AssetSyncTask", "SyncingAssetProgress", "Syncing Assets"));
        ProgressWidget->SetProgressPercent(0.0f);
    }

    // 每帧同步资产
    SyncTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateSP(AsShared(), &FAssetSyncTask::SyncTick),
        0.0f
    );

    if (!SyncTickHandle.IsValid())
    {
        Finish(false);
    }
}

bool FAssetSyncTask::SyncTick(float DeltaTime)
{
    if (bFinished || Phase != EPhase::Syncing) return false;

    UEUSS_MetaDataManager* MetaDataManager = MetaDataOfAssetsGlobal::GetMetaDataManagerSafe();
    if (!MetaDataManager)
    {
        Finish(false);
        return false;
    }

    // 记录当前时间戳 + 预算时间
    const double SyncDeadline = FPlatformTime::Seconds() + SyncTimeBudgetSeconds;
    // 当前帧已处理的资产数量
    int32 ProcessedThisTick = 0;

    while (NextSyncIndex < Assets.Num() && ProcessedThisTick < SyncBatchSize)
    {
        FAssetData& AssetData = Assets[NextSyncIndex];
        // 已经加载过资产，GetAsset 不会再次加载
        UObject* Asset = AssetData.GetAsset();

        if (IsValid(Asset))
        {
            // 同步资产
            MetaDataManager->SyncAsset(Asset);
            ++SyncedAssetCount;
        }
        else
        {
            ++FailedAssetCount;
            UE_LOG(
                LogMetaDataOfAssets,
                Warning,
                TEXT("[%hs] 无法同步加载失败的资产: %s"),
                __FUNCTION__,
                *AssetData.GetSoftObjectPath().ToString());
        }

        ++NextSyncIndex;
        ++ProcessedThisTick;

        // 判断是否超时
        if (FPlatformTime::Seconds() >= SyncDeadline)
        {
            break;
        }
    }

    if (ProgressWidget.IsValid())
    {
        // 更新进度条
        const float SyncProgress = Assets.Num() > 0
            ? static_cast<float>(NextSyncIndex) / static_cast<float>(Assets.Num())
            : 1.0f;
        ProgressWidget->SetProgressPercent(FMath::Clamp(SyncProgress, 0.0f, 1.0f));
    }

    // 判断是否完成同步
    if (NextSyncIndex >= Assets.Num())
    {
        Finish(true);
        return false;
    }

    return true;
}
