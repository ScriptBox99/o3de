/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeSelectionFixture.h>

namespace UnitTest
{
    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionSelectEntityWithFocusOnLevel)
    {
        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);
    }

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionSelectEntityWithFocusOnAncestor)
    {
        // Set the focus on the Street Entity (parent of the test entity)
        m_focusModeInterface->SetFocusRoot(m_entityMap[StreetEntityName]);

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);
    }

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionSelectEntityWithFocusOnItself)
    {
        // Set the focus on the Car Entity (test entity)
        m_focusModeInterface->SetFocusRoot(m_entityMap[CarEntityName]);

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);
    }

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionSelectEntityWithFocusOnSibling)
    {
        // Set the focus on the SportsCar Entity (sibling of the test entity)
        m_focusModeInterface->SetFocusRoot(m_entityMap[SportsCarEntityName]);

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 0);
    }

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionSelectEntityWithFocusOnDescendant)
    {
        // Set the focus on the Passenger1 Entity (child of the entity)
        m_focusModeInterface->SetFocusRoot(m_entityMap[Passenger1EntityName]);

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 0);
    }
} // namespace UnitTest
