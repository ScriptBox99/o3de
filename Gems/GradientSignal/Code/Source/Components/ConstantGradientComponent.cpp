/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/ConstantGradientComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace GradientSignal
{
    void ConstantGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ConstantGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("Value", &ConstantGradientConfig::m_value)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ConstantGradientConfig>(
                    "Constant Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ConstantGradientConfig::m_value, "Value", "Value always returned by this gradient.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ConstantGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("constantValue", BehaviorValueProperty(&ConstantGradientConfig::m_value))
                ;
        }
    }

    void ConstantGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void ConstantGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void ConstantGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void ConstantGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        ConstantGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ConstantGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &ConstantGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("ConstantGradientComponentTypeId", BehaviorConstant(ConstantGradientComponentTypeId));

            behaviorContext->Class<ConstantGradientComponent>()->RequestBus("ConstantGradientRequestBus");

            behaviorContext->EBus<ConstantGradientRequestBus>("ConstantGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetConstantValue", &ConstantGradientRequestBus::Events::GetConstantValue)
                ->Event("SetConstantValue", &ConstantGradientRequestBus::Events::SetConstantValue)
                ->VirtualProperty("ConstantValue", "GetConstantValue", "SetConstantValue")
                ;
        }
    }

    ConstantGradientComponent::ConstantGradientComponent(const ConstantGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void ConstantGradientComponent::Activate()
    {
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        ConstantGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ConstantGradientComponent::Deactivate()
    {
        GradientRequestBus::Handler::BusDisconnect();
        ConstantGradientRequestBus::Handler::BusDisconnect();
    }

    bool ConstantGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const ConstantGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool ConstantGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<ConstantGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float ConstantGradientComponent::GetValue([[maybe_unused]] const GradientSampleParams& sampleParams) const
    {
        return m_configuration.m_value;
    }

    float ConstantGradientComponent::GetConstantValue() const
    {
        return m_configuration.m_value;
    }

    void ConstantGradientComponent::SetConstantValue(float constant)
    {
        m_configuration.m_value = constant;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
