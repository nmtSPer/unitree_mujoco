#pragma once

#include <memory>

namespace mujoco
{
class PlatformUIAdapter;
class Simulate;

using CustomKeyCallback = void (*)(int key, int scancode, int act);

std::unique_ptr<PlatformUIAdapter> MakeCustomUiAdapter();
void AttachCustomUi(Simulate *sim, PlatformUIAdapter *adapter);
void SetCustomKeyCallback(PlatformUIAdapter *adapter, CustomKeyCallback callback);
void ApplyViewerUiTheme(Simulate *sim, bool refresh_ui);
} // namespace mujoco
