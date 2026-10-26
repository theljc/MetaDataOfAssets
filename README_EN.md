<div align="center">
<h1>MetaData of Assets Plugin</h1>

![UE Version](https://img.shields.io/badge/UE-5.0%2B-blue)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

[Chinese Version](README.md)

</div>




## Table of Contents

- [Video Tutorial](#video-tutorial)
- [Introduction](#introduction)
- [Plugin Settings](#plugin-settings)
- [Usage](#usage)
- [Extending the Plugin](#extending-the-plugin)
  - [Directory Structure](#directory-structure)
  - [Extension Guidelines](#extension-guidelines)
- [Notes](#notes)




## Video Tutorial

**Link to be added (recording in progress)**



## Introduction

- **Features**
  - Batch manage asset metadata in the Unreal Engine Editor and automatically register it with the Asset Registry
- **Applicable version**
  - UE 5.0 or above
- **Download**
  - Option 1: Download directly from the FAB Marketplace: **[link to be added]**
  - Option 2: Download the compressed file from https://github.com/theljc/MetaDataOfAssets/releases which contains the corresponding plugins for all versions of UE5. Extract the required version to the `Plugins` folder of your project
- **Enable**
  - Open the project, search for **MetaDataOfAssets** in the [Plugins] panel, and enable the plugin



## Plugin Settings

> [!IMPORTANT]
>
> Because the plugin uses a reference-counting mechanism internally, after enabling **Allow Manage Asset Registry Tags Delete**, you must synchronize the metadata in existing assets. Configure the paths in Project Settings and click **Sync assets in configured path** in the main widget. Otherwise, metadata tags in the Asset Registry may be deleted incorrectly.

- [Project Settings -> MetaDataOfAssets]
  - **Allow Manage Asset Registry Tags Delete**: Determines whether tags should be automatically removed from the Asset Registry (disabled by default). Restart the editor after changing this setting.
  - **Scan Settings**: Available after **Allow Manage Asset Registry Tags Delete** is enabled.
    - **Directory**: Specifies the directories to scan.
    - **Recursive**: Determines whether to scan the directory recursively.
- [Editor Preferences -> Keyboard Shortcuts]
  - MetaDataOfAssets Shortcut Key
    - **Open Main Widget**: The shortcut for opening the main widget. You can customize it. The default shortcut is **Alt + Z**.
    - **Close All Widgets**: Closes all widgets created by this plugin. You can customize it. The default shortcut is **Alt + Shift + Z**.



## Usage

- Open the main widget

    - Click the button on the Level Editor toolbar to open the main widget, or use the keyboard shortcut (default: **Alt + Z**).

      ![](./Picture/OpenMainWidget.png)


- Select and add assets

   - Option 1: Select assets directly in the Content Browser, then click **Add selected assets**.

   - Option 2: Click the arrow at the top of the main widget to expand it, configure the directory from which assets should be added in Selected Folder, then click **Add Assets Through Folder Config**.

     ![](./Picture/SelectandAddAssets.png)

- Select the desired operation

   ​	You can operate on an individual asset directly in the main widget, or use the **Batch operation** buttons to operate on all assets already added to the widget.

   - **Add**: Adds metadata to the selected assets. During batch add, each metadata entry is added to each asset individually. When **Allow Manage Asset Registry Tags Delete** is enabled in Project Settings, the internal reference count is incremented by one.
   - **Modify**: Modifies metadata values. During batch modification, only metadata shared by the selected assets is modified.
   - **Delete**: Deletes asset metadata. During batch deletion, only metadata shared by the selected assets is deleted. If **Allow Manage Asset Registry Tags Delete** is enabled, the internal reference count determines whether the tags registered with the Asset Registry should be removed.
   - **Sync**: Adds tags from assets that are not yet registered with the Asset Registry. If **Allow Manage Asset Registry Tags Delete** is enabled, the reference count determines whether tags registered with the Asset Registry should be removed.
   - **Copy**: Copies metadata from one asset to another asset, copying only metadata that does not already exist on the target asset, and automatically registers it with the Asset Registry. During batch copy, each selected source asset is copied to all target assets.

- When selecting assets, hold **Ctrl** to toggle selection, **Shift** to select a range, or **Ctrl + A** to select all.

- Use the keyboard shortcut (default: **Alt + Shift + Z**) to close all open windows.

- The **Sync assets in configured path** button in the upper-right corner synchronizes assets under the paths configured in Project Settings. It is available after **Allow Manage Asset Registry Tags Delete** is enabled in Project Settings.

- Console commands

   - **MetaDataOfAssets.EnableDebug**: Enables UE_LOG and PrintString output related to this plugin (enabled by default).
   - **MetaDataOfAssets.DisableDebug**: Disables UE_LOG and PrintString output related to this plugin.



## Extending the Plugin

### Directory Structure

```
MetaDataOfAssets/
	Resources/					# Toolbar icons
	Config/						# Plugin-related configuration
	Content/
		EUW_MainWidget.uasset	# Main widget asset
		Widget/					# Contains all child widgets
		Data/					# Contains all data
		Localization/			# Plugin-related localization data
	Source/MetaDataOfAssets/
		Command/				# Defines plugin commands
		Style/					# Defines plugin styles
		Log/					# Custom log category
		Global/					# Global properties
		MetaData/				# Contains MetaDataManager, which handles metadata
		Settings/				# Configurable properties in Project Settings
		Task/					# Asset synchronization tasks
		Widget/					# Contains the widget manager and widget base classes
```

### Extension Guidelines

- **Conventions**
  - When creating a UserWidget, you must use `UEUSS_WidgetManager::CreateUserWidget` and construct the parameters with `InstancedStruct`.
  - When creating an EditorUtilityWidget, you must use `UEUSS_WidgetManager::CreateEditorUtilityWidget` and construct the parameters with `InstancedStruct`.
  - New UserWidgets must inherit from `UUW_BaseDataWidget`.
  - New EditorUtilityWidgets must inherit from `UEUW_BaseUtilityWidget`.
  - New widgets can override the `OnOpen` interface to receive parameters and initialize themselves, and the `OnClose` interface to perform cleanup when the widget is destroyed.
  - If you want a widget to close when its parent is closed, use `UEUSS_WidgetManager::AddToChildNode` in the widget's `OnOpen` to add it to the node tree. Pass the parent widget as `ParentWidget` and the widget itself as `ChildWidget`.
  - Avoid modifying the existing parent-child relationships.



## Notes

- This is an editor plugin.
- To use this plugin, you need to enable both the `Struct Utils` and `Editor Scripting Utilities` plugins.
- The plugin is localized in Chinese and English. The displayed text changes automatically when the Editor language is switched in [Editor Preferences -> Region & Language].
- All project-related configuration for this plugin is stored in [Config/DefaultMetaDataOfAssets.ini] in the plugin directory.
  - `[/Script/MetaDataOfAssets.MetaDataOfAssetsSettings]` section
    - `bAllowManageAssetRegistryTagsDelete`: Whether to allow automatic deletion of tags from the Asset Registry.
    - `ScanSettings`: The scan paths used when synchronizing assets.
    - `bEnableDebug`: Whether to enable UE_LOG and PrintString output related to this plugin.
    - `MainWidgetPath`: The path to the main widget asset. Do not modify it manually; otherwise, the button on the Editor toolbar will not be able to open the main widget.
- This asset is loaded when assets are added to the main widget and when **Sync assets in configured path** is clicked.
- When **Sync assets in configured path** is clicked, assets are loaded across multiple frames in the editor, and the loading progress is displayed in the bottom-right corner.

