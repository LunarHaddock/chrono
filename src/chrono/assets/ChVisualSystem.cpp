// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2022 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Radu Serban
// =============================================================================

#include "chrono/assets/ChVisualSystem.h"

namespace chrono {

ChVisualSystem ::~ChVisualSystem() {
    for (auto s : m_systems)
        s->visual_system = nullptr;
}

void ChVisualSystem::AttachSystem(ChSystem* sys) {
    m_systems.push_back(sys);
    sys->visual_system = this;
}

}  // namespace chrono
