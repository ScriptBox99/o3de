#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME Atom_AtomBridge.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atombridge_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PRIVATE
            ENABLE_ATOM_DEBUG_DISPLAY=1
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AtomCore
            AZ::AzFramework
            Gem::Atom_RPI.Public
            Gem::Atom_Bootstrap.Headers
            Legacy::CryCommon
)

ly_add_target(
    NAME Atom_AtomBridge ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atombridge_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/additional_${PAL_PLATFORM_NAME_LOWERCASE}_runtime_deps.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            ENABLE_ATOM_DEBUG_DISPLAY=1
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Atom_AtomBridge.Static
    RUNTIME_DEPENDENCIES
        Gem::Atom_RHI.Private
        Gem::Atom_RPI.Private
        Gem::Atom_RHI_Null.Private
        Gem::Atom_Feature_Common
        Gem::Atom_Bootstrap
        Gem::Atom_Component_DebugCamera
        Gem::AtomImGuiTools
        Gem::AtomLyIntegration_CommonFeatures
        Gem::EMotionFX_Atom
        Gem::ImguiAtom
        Gem::AtomFont
        Gem::AtomViewportDisplayInfo
)

# Any 'runtime-like' applications should use Gem::Atom_AtomBridge:
ly_create_alias(NAME Atom_AtomBridge.Clients NAMESPACE Gem TARGETS Gem::Atom_AtomBridge)
ly_create_alias(NAME Atom_AtomBridge.Servers NAMESPACE Gem TARGETS Gem::Atom_AtomBridge)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    set(additional_tool_deps ${pal_dir}/additional_${PAL_PLATFORM_NAME_LOWERCASE}_tool_deps.cmake)
    foreach(pal_tools_platform ${LY_PAL_TOOLS_ENABLED})
        string(TOLOWER ${pal_tools_platform} pal_tools_platform_lowercase)
        ly_get_list_relative_pal_filename(pal_runtime_dependencies_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${pal_tools_platform})
        list(APPEND additional_tool_deps ${pal_runtime_dependencies_source_dir}/additional_${pal_tools_platform_lowercase}_tool_deps.cmake)
    endforeach()

    ly_add_target(
        NAME Atom_AtomBridge.Editor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atombridge_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        PLATFORM_INCLUDE_FILES
           ${additional_tool_deps}
        COMPILE_DEFINITIONS
            PRIVATE
                EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AssetBuilderSDK
                Gem::Atom_Utils.Static
                Gem::Atom_AtomBridge.Static
        RUNTIME_DEPENDENCIES
            Gem::Atom_RHI.Private
            Gem::Atom_RPI.Builders
            Gem::Atom_RPI.Editor
            Gem::Atom_RHI_Null.Private
            Gem::Atom_RHI_Null.Builders
            Gem::Atom_Feature_Common.Builders
            Gem::Atom_Feature_Common.Editor
            Gem::Atom_Bootstrap
            Gem::Atom_Asset_Shader.Builders
            Gem::Atom_Component_DebugCamera
            Gem::AtomImGuiTools
            Gem::AtomLyIntegration_CommonFeatures.Editor
            Gem::EMotionFX_Atom.Editor
            Gem::ImageProcessingAtom.Editor
            Gem::ImguiAtom
            Gem::AtomFont
            Gem::AtomToolsFramework.Editor
            Gem::AtomViewportDisplayInfo
            Gem::AtomViewportDisplayIcons.Editor
            Gem::MaterialEditor.Builders
    )


    # Any 'tool' and 'builder' type applications should use Gem::Atom_AtomBridge.Editor:
    ly_create_alias(NAME Atom_AtomBridge.Builders NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Editor)
    ly_create_alias(NAME Atom_AtomBridge.Tools    NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Editor)
endif()

# The "Atom" Gem will alias the real Atom_AtomBridge target variants
# allows the enabling and disabling the "Atom" Gem to build the pre-requisite dependencies
# The "AtomLyIntegration" Gem will also alias the real Atom_AtomBridge target variants
# The Atom Gem does the same at the moment.
ly_create_alias(NAME Atom.Clients NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Clients)
ly_create_alias(NAME Atom.Servers NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Servers)
ly_create_alias(NAME AtomLyIntegration.Clients NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Clients)
ly_create_alias(NAME AtomLyIntegration.Servers NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Servers)
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME Atom.Builders NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Builders)
    ly_create_alias(NAME Atom.Tools    NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Tools)
    ly_create_alias(NAME AtomLyIntegration.Builders NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Builders)
    ly_create_alias(NAME AtomLyIntegration.Tools    NAMESPACE Gem TARGETS Gem::Atom_AtomBridge.Tools)
endif()
