// Copyright 2026 ljc. All rights reserved.

#include "MetaDataOfAssets/Public/Global/MetaDataOfAssetsGlobal.h"
#include "Widget/EUSS_WidgetManager.h"
#include "MetaData/EUSS_MetaDataManager.h"
#include "Editor.h"


UEUSS_WidgetManager* MetaDataOfAssetsGlobal::GetWidgetManagerSafe()
{
	return GEditor
		? GEditor->GetEditorSubsystem<UEUSS_WidgetManager>()
		: nullptr;
}

UEUSS_MetaDataManager* MetaDataOfAssetsGlobal::GetMetaDataManagerSafe()
{
	return GEditor
		? GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>()
		: nullptr;
}
