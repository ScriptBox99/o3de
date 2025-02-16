/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestFixtures.h>

#include <AzFramework/Components/TransformComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>

// Base gradient components
#include <GradientSignal/Components/ConstantGradientComponent.h>
#include <GradientSignal/Components/ImageGradientComponent.h>
#include <GradientSignal/Components/PerlinGradientComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>
#include <GradientSignal/Components/ShapeAreaFalloffGradientComponent.h>

// Gradient modifier components
#include <GradientSignal/Components/DitherGradientComponent.h>
#include <GradientSignal/Components/InvertGradientComponent.h>
#include <GradientSignal/Components/LevelsGradientComponent.h>
#include <GradientSignal/Components/MixedGradientComponent.h>
#include <GradientSignal/Components/PosterizeGradientComponent.h>
#include <GradientSignal/Components/ReferenceGradientComponent.h>
#include <GradientSignal/Components/SmoothStepGradientComponent.h>
#include <GradientSignal/Components/ThresholdGradientComponent.h>

// Gradient surface data components
#include <GradientSignal/Components/SurfaceAltitudeGradientComponent.h>
#include <GradientSignal/Components/SurfaceMaskGradientComponent.h>
#include <GradientSignal/Components/SurfaceSlopeGradientComponent.h>

namespace UnitTest
{
    void GradientSignalBaseFixture::SetupCoreSystems()
    {
        m_app = AZStd::make_unique<AZ::ComponentApplication>();
        ASSERT_TRUE(m_app != nullptr);

        AZ::ComponentApplication::Descriptor componentAppDesc;

        m_systemEntity = m_app->Create(componentAppDesc);
        ASSERT_TRUE(m_systemEntity != nullptr);
        m_app->AddEntity(m_systemEntity);

        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
        AZ::Data::AssetManager::Descriptor desc;
        AZ::Data::AssetManager::Create(desc);
        m_mockHandler = new ImageAssetMockAssetHandler();
        AZ::Data::AssetManager::Instance().RegisterHandler(m_mockHandler, azrtti_typeid<GradientSignal::ImageAsset>());

        m_mockShapeHandlers = new AZStd::vector<AZStd::unique_ptr<testing::NiceMock<UnitTest::MockShapeComponentRequests>>>();
    }

    void GradientSignalBaseFixture::TearDownCoreSystems()
    {
        // Clear any mock shape handlers that we've created for our test entities.
        delete m_mockShapeHandlers;

        AZ::Data::AssetManager::Instance().UnregisterHandler(m_mockHandler);
        delete m_mockHandler; // delete after removing from the asset manager

        AzFramework::LegacyAssetEventBus::ClearQueuedEvents();
        AZ::Data::AssetManager::Destroy();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();

        m_app->Destroy();
        m_app.reset();
        m_systemEntity = nullptr;
    }

    AZStd::unique_ptr<testing::NiceMock<UnitTest::MockShapeComponentRequests>> GradientSignalBaseFixture::CreateMockShape(
        const AZ::Aabb& spawnerBox, const AZ::EntityId& shapeEntityId)
    {
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockShapeComponentRequests>> mockShape =
            AZStd::make_unique<testing::NiceMock<UnitTest::MockShapeComponentRequests>>(shapeEntityId);

        ON_CALL(*mockShape, GetEncompassingAabb).WillByDefault(testing::Return(spawnerBox));
        ON_CALL(*mockShape, GetTransformAndLocalBounds)
            .WillByDefault(
                [spawnerBox](AZ::Transform& transform, AZ::Aabb& bounds)
                {
                    transform = AZ::Transform::CreateTranslation(spawnerBox.GetCenter());
                    bounds = spawnerBox.GetTranslated(-spawnerBox.GetCenter());
                });
        ON_CALL(*mockShape, IsPointInside)
            .WillByDefault(
                [spawnerBox](const AZ::Vector3& point) -> bool
                {
                    return spawnerBox.Contains(point);
                });

        return mockShape;
    }

    AZStd::unique_ptr<MockSurfaceDataSystem> GradientSignalBaseFixture::CreateMockSurfaceDataSystem(const AZ::Aabb& spawnerBox)
    {
        SurfaceData::SurfacePoint point;
        AZStd::unique_ptr<MockSurfaceDataSystem> mockSurfaceDataSystem = AZStd::make_unique<MockSurfaceDataSystem>();

        // Give the mock surface data a bunch of fake point values to return.
        for (float y = spawnerBox.GetMin().GetY(); y < spawnerBox.GetMax().GetY(); y+= 1.0f)
        {
            for (float x = spawnerBox.GetMin().GetX(); x < spawnerBox.GetMax().GetX(); x += 1.0f)
            {
                // Use our x distance into the spawnerBox as an arbitrary percentage value that we'll use to calculate
                // our other arbitrary values below.
                float arbitraryPercentage = AZStd::abs(x / spawnerBox.GetExtents().GetX());

                // Create a position that's between min and max Z of the box.
                point.m_position = AZ::Vector3(x, y, AZ::Lerp(spawnerBox.GetMin().GetZ(), spawnerBox.GetMax().GetZ(), arbitraryPercentage));
                // Create an arbitrary normal value.
                point.m_normal = point.m_position.GetNormalized();
                // Create an arbitrary surface value.
                point.m_masks[AZ_CRC_CE("test_mask")] = arbitraryPercentage;

                mockSurfaceDataSystem->m_GetSurfacePoints[AZStd::make_pair(x, y)] = { { point } };
            }
        }

        return mockSurfaceDataSystem;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::CreateTestEntity(float shapeHalfBounds)
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testEntity = CreateEntity();

        // Create a mock Shape component that describes the bounds that we're using to map our gradient into world space.
        CreateComponent<MockShapeComponent>(testEntity.get());

        // Create and keep a reference to a mock shape handler that will respond to shape requests for the mock shape.
        auto mockShapeHandler =
            CreateMockShape(AZ::Aabb::CreateCenterRadius(AZ::Vector3(shapeHalfBounds), shapeHalfBounds), testEntity->GetId());
        m_mockShapeHandlers->push_back(AZStd::move(mockShapeHandler));

        // Create a transform that locates our gradient in the center of our desired mock Shape.
        auto transform = CreateComponent<AzFramework::TransformComponent>(testEntity.get());
        transform->SetLocalTM(AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds)));
        transform->SetWorldTM(AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds)));

        return testEntity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestConstantGradient(float shapeHalfBounds)
    {
        // Create a Constant Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ConstantGradientConfig config;
        config.m_value = 0.75f;
        CreateComponent<GradientSignal::ConstantGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestImageGradient(float shapeHalfBounds)
    {
        // Create an Image Gradient Component with arbitrary sizes and parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ImageGradientConfig config;
        const uint32_t imageSize = 4096;
        const int32_t imageSeed = 12345;
        config.m_imageAsset = ImageAssetMockAssetHandler::CreateImageAsset(imageSize, imageSize, imageSeed);
        config.m_tilingX = 1.0f;
        config.m_tilingY = 1.0f;
        CreateComponent<GradientSignal::ImageGradientComponent>(entity.get(), config);

        // Create a Gradient Transform Component with arbitrary parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(entity.get(), gradientTransformConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestPerlinGradient(float shapeHalfBounds)
    {
        // Create a Perlin Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::PerlinGradientConfig config;
        config.m_amplitude = 1.0f;
        config.m_frequency = 1.1f;
        config.m_octave = 4;
        config.m_randomSeed = 12345;
        CreateComponent<GradientSignal::PerlinGradientComponent>(entity.get(), config);

        // Create a Gradient Transform Component with arbitrary parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(entity.get(), gradientTransformConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestRandomGradient(float shapeHalfBounds)
    {
        // Create a Random Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::RandomGradientConfig config;
        config.m_randomSeed = 12345;
        CreateComponent<GradientSignal::RandomGradientComponent>(entity.get(), config);

        // Create a Gradient Transform Component with arbitrary parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(entity.get(), gradientTransformConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestShapeAreaFalloffGradient(float shapeHalfBounds)
    {
        // Create a Shape Area Falloff Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ShapeAreaFalloffGradientConfig config;
        config.m_shapeEntityId = entity->GetId();
        config.m_falloffWidth = 16.0f;
        config.m_falloffType = GradientSignal::FalloffType::InnerOuter;
        CreateComponent<GradientSignal::ShapeAreaFalloffGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestDitherGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Dither Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::DitherGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 1.0f;
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4;
        CreateComponent<GradientSignal::DitherGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestInvertGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create an Invert Gradient Component.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::InvertGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        CreateComponent<GradientSignal::InvertGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestLevelsGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Levels Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::LevelsGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_inputMin = 0.1f;
        config.m_inputMid = 0.3f;
        config.m_inputMax = 0.9f;
        config.m_outputMin = 0.0f;
        config.m_outputMax = 1.0f;
        CreateComponent<GradientSignal::LevelsGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestMixedGradient(
        float shapeHalfBounds, const AZ::EntityId& baseGradientId, const AZ::EntityId& mixedGradientId)
    {
        // Create a Mixed Gradient Component that mixes two input gradients together in arbitrary ways.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::MixedGradientConfig config;

        GradientSignal::MixedGradientLayer layer;
        layer.m_enabled = true;

        layer.m_operation = GradientSignal::MixedGradientLayer::MixingOperation::Initialize;
        layer.m_gradientSampler.m_gradientId = baseGradientId;
        layer.m_gradientSampler.m_opacity = 1.0f;
        config.m_layers.push_back(layer);

        layer.m_operation = GradientSignal::MixedGradientLayer::MixingOperation::Overlay;
        layer.m_gradientSampler.m_gradientId = mixedGradientId;
        layer.m_gradientSampler.m_opacity = 0.75f;
        config.m_layers.push_back(layer);

        CreateComponent<GradientSignal::MixedGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestPosterizeGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Posterize Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::PosterizeGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_mode = GradientSignal::PosterizeGradientConfig::ModeType::Ps;
        config.m_bands = 5;
        CreateComponent<GradientSignal::PosterizeGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestReferenceGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Reference Gradient Component.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ReferenceGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_gradientSampler.m_ownerEntityId = entity->GetId();
        CreateComponent<GradientSignal::ReferenceGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestSmoothStepGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Smooth Step Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::SmoothStepGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_smoothStep.m_falloffMidpoint = 0.75f;
        config.m_smoothStep.m_falloffRange = 0.125f;
        config.m_smoothStep.m_falloffStrength = 0.25f;
        CreateComponent<GradientSignal::SmoothStepGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestThresholdGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Threshold Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ThresholdGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_threshold = 0.75f;
        CreateComponent<GradientSignal::ThresholdGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestSurfaceAltitudeGradient(float shapeHalfBounds)
    {
        // Create a Surface Altitude Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::SurfaceAltitudeGradientConfig config;
        config.m_altitudeMin = -5.0f;
        config.m_altitudeMax = 15.0f;
        CreateComponent<GradientSignal::SurfaceAltitudeGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestSurfaceMaskGradient(float shapeHalfBounds)
    {
        // Create a Surface Mask Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::SurfaceMaskGradientConfig config;
        config.m_surfaceTagList.push_back(AZ_CRC_CE("test_mask"));
        CreateComponent<GradientSignal::SurfaceMaskGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestSurfaceSlopeGradient(float shapeHalfBounds)
    {
        // Create a Surface Slope Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::SurfaceSlopeGradientConfig config;
        config.m_slopeMin = 5.0f;
        config.m_slopeMax = 50.0f;
        config.m_rampType = GradientSignal::SurfaceSlopeGradientConfig::RampType::SMOOTH_STEP;
        config.m_smoothStep.m_falloffMidpoint = 0.75f;
        config.m_smoothStep.m_falloffRange = 0.125f;
        config.m_smoothStep.m_falloffStrength = 0.25f;
        CreateComponent<GradientSignal::SurfaceSlopeGradientComponent>(entity.get(), config);

        ActivateEntity(entity.get());
        return entity;
    }

    void GradientSignalTest::TestFixedDataSampler(const AZStd::vector<float>& expectedOutput, int size, AZ::EntityId gradientEntityId)
    {
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = gradientEntityId;

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(static_cast<float>(x), static_cast<float>(y), 0.0f);

                const int index = y * size + x;
                float actualValue = gradientSampler.GetValue(params);
                float expectedValue = expectedOutput[index];

                EXPECT_NEAR(actualValue, expectedValue, 0.01f);
            }
        }
    }
}

