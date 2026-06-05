#include "custom_ui.h"

#include "glfw_adapter.h"
#include "simulate.h"

#include <mujoco/mujoco.h>

#include <cstddef>
#include <cstring>

namespace mujoco
{
namespace
{
constexpr int kViewerFontIndex = 2;
constexpr int kViewerFontScale = 50 * (kViewerFontIndex + 1);
constexpr char kUnitTestEqualityName[] = "unit_test";
constexpr char kUnitTestSectionName[] = "Unit test";

void SetRgb(float rgb[3], float r, float g, float b)
{
  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}

void CollapseUiPanels(Simulate *sim)
{
  sim->ui0_enable = 0;
  sim->ui1_enable = 0;
}

int FindUnitTestEqualityIndex(const Simulate *sim)
{
  if (!sim || !sim->m_ || !sim->d_)
  {
    return -1;
  }

  for (int i = 0; i < sim->m_->neq; ++i)
  {
    const char *name = nullptr;
    if (sim->equality_names_.size() > static_cast<std::size_t>(i))
    {
      name = sim->equality_names_[i].c_str();
    }
    else if (sim->m_->name_eqadr && sim->m_->name_eqadr[i] >= 0)
    {
      name = sim->m_->names + sim->m_->name_eqadr[i];
    }

    if (name && std::strcmp(name, kUnitTestEqualityName) == 0)
    {
      return i;
    }
  }

  return -1;
}

void RefreshUi(mjUI *ui, mjuiState *state, const mjrContext *context)
{
  mjui_resize(ui, context);
  mjui_update(-1, -1, ui, state, context);
}

class CustomUiGlfwAdapter final : public GlfwAdapter
{
public:
  void SetSimulate(Simulate *sim)
  {
    sim_ = sim;
  }

  void SetKeyCallback(CustomKeyCallback callback)
  {
    key_callback_ = callback;
  }

  void PollEvents() override
  {
    GlfwAdapter::PollEvents();

    ApplyEqualityDefaults();
    ApplyUnitTestUi();
    SyncUnitTestEquality();

    if (sim_ && sim_->ui0.nsect > 0 &&
        (!theme_applied_ || sim_->ui0.nsect != themed_ui0_nsect_ ||
         sim_->ui1.nsect != themed_ui1_nsect_))
    {
      CollapseUiPanels(sim_);
      ApplyViewerUiTheme(sim_, true);
      theme_applied_ = true;
      themed_ui0_nsect_ = sim_->ui0.nsect;
      themed_ui1_nsect_ = sim_->ui1.nsect;
    }
  }

private:
  void OnKey(int key, int scancode, int act) override
  {
    if (IsKeyDownEvent(act))
    {
      if (key == GLFW_KEY_BACKSPACE && sim_ && sim_->m_ && sim_->d_ && !sim_->is_passive_)
      {
        sim_->pending_.reset = true;
      }

      if (key_callback_)
      {
        key_callback_(key, scancode, act);
      }
    }

    PlatformUIAdapter::OnKey(key, scancode, act);
  }

  void ApplyEqualityDefaults()
  {
    const int equality_index = FindUnitTestEqualityIndex(sim_);
    if (equality_index < 0)
    {
      equality_defaults_model_ = nullptr;
      equality_defaults_data_ = nullptr;
      unit_test_enabled_ = 0;
      return;
    }

    if (sim_->m_ == equality_defaults_model_ && sim_->d_ == equality_defaults_data_)
    {
      return;
    }

    bool changed = false;
    if (sim_->m_->eq_active0[equality_index])
    {
      sim_->m_->eq_active0[equality_index] = 0;
      changed = true;
    }

    equality_defaults_model_ = sim_->m_;
    equality_defaults_data_ = sim_->d_;
    changed |= SetUnitTestEqualityActive(equality_index, 0);

    if (changed && sim_->ui1.nsect > 0)
    {
      mjui_update(-1, -1, &sim_->ui1, &sim_->uistate, &sim_->platform_ui->mjr_context());
    }
  }

  void ApplyUnitTestUi()
  {
    const int equality_index = FindUnitTestEqualityIndex(sim_);
    if (equality_index < 0 || sim_->ui1.nsect <= 0)
    {
      return;
    }

    if (!FindUnitTestSection() && AddUnitTestSection())
    {
      RefreshUi(&sim_->ui1, &sim_->uistate, &sim_->platform_ui->mjr_context());
    }
  }

  bool AddUnitTestSection()
  {
    if (sim_->ui1.nsect >= mjMAXUISECT)
    {
      return false;
    }

    mjuiDef defUnitTest[] = {
        {mjITEM_SECTION, "Unit test", mjPRESERVE, nullptr, "AU"},
        {mjITEM_RADIO, "", 5, &unit_test_enabled_, "Off\nOn"},
        {mjITEM_END}};

    const int nsect_before = sim_->ui1.nsect;
    mjui_add(&sim_->ui1, defUnitTest);
    return sim_->ui1.nsect != nsect_before;
  }

  mjuiSection *FindUnitTestSection()
  {
    if (!sim_)
    {
      return nullptr;
    }

    for (int i = 0; i < sim_->ui1.nsect; ++i)
    {
      if (std::strcmp(sim_->ui1.sect[i].name, kUnitTestSectionName) == 0)
      {
        return &sim_->ui1.sect[i];
      }
    }
    return nullptr;
  }

  bool SetEqActive(mjtByte *value, int active)
  {
    const auto byte_active = static_cast<mjtByte>(active ? 1 : 0);
    if (*value == byte_active)
    {
      return false;
    }
    *value = byte_active;
    return true;
  }

  bool SetUnitTestEqualityActive(int equality_index, int active)
  {
    if (equality_index < 0 || !sim_ || !sim_->d_)
    {
      return false;
    }

    active = active ? 1 : 0;
    unit_test_enabled_ = active;

    bool changed = SetEqActive(&sim_->d_->eq_active[equality_index], active);
    if (sim_->eq_active_.size() > static_cast<std::size_t>(equality_index))
    {
      changed |= SetEqActive(&sim_->eq_active_[equality_index], active);
    }
    if (sim_->eq_active_prev_.size() > static_cast<std::size_t>(equality_index))
    {
      changed |= SetEqActive(&sim_->eq_active_prev_[equality_index], active);
    }
    return changed;
  }

  void SyncUnitTestEquality()
  {
    const int equality_index = FindUnitTestEqualityIndex(sim_);
    if (equality_index < 0)
    {
      unit_test_enabled_ = 0;
      return;
    }

    if (SetUnitTestEqualityActive(equality_index, unit_test_enabled_) && sim_->ui1.nsect > 0)
    {
      mjui_update(-1, -1, &sim_->ui1, &sim_->uistate, &sim_->platform_ui->mjr_context());
    }
  }

  Simulate *sim_ = nullptr;
  const mjModel *equality_defaults_model_ = nullptr;
  const mjData *equality_defaults_data_ = nullptr;
  int unit_test_enabled_ = 0;
  CustomKeyCallback key_callback_ = nullptr;
  bool theme_applied_ = false;
  int themed_ui0_nsect_ = -1;
  int themed_ui1_nsect_ = -1;
};
} // namespace

std::unique_ptr<PlatformUIAdapter> MakeCustomUiAdapter()
{
  return std::make_unique<CustomUiGlfwAdapter>();
}

void AttachCustomUi(Simulate *sim, PlatformUIAdapter *adapter)
{
  if (auto *custom_adapter = dynamic_cast<CustomUiGlfwAdapter *>(adapter))
  {
    custom_adapter->SetSimulate(sim);
  }
}

void SetCustomKeyCallback(PlatformUIAdapter *adapter, CustomKeyCallback callback)
{
  if (auto *custom_adapter = dynamic_cast<CustomUiGlfwAdapter *>(adapter))
  {
    custom_adapter->SetKeyCallback(callback);
  }
}

void ApplyViewerUiTheme(Simulate *sim, bool refresh_ui)
{
  mjuiThemeColor color = mjui_themeColor(0);

  SetRgb(color.master, 0.05f, 0.05f, 0.10f);
  SetRgb(color.thumb, 0.120f, 0.110f, 0.25f);

  SetRgb(color.secttitle, 0.150f, 0.100f, 0.600f);
  SetRgb(color.secttitle2, 0.120f, 0.100f, 0.300f);
  SetRgb(color.secttitleuncheck, 0.450f, 0.170f, 0.170f);
  SetRgb(color.secttitleuncheck2, 0.450f, 0.170f, 0.170f);
  SetRgb(color.secttitlecheck, 0.450f, 0.170f, 0.170f);
  SetRgb(color.secttitlecheck2, 0.450f, 0.170f, 0.170f);

  SetRgb(color.sectfont, 1.000f, 1.000f, 1.000f);
  SetRgb(color.sectsymbol, 0.700f, 0.700f, 0.700f);
  SetRgb(color.sectpane, 0.150f, 0.100f, 0.200f);

  SetRgb(color.separator, 0.10f, 0.30f, 0.600f);
  SetRgb(color.separator2, 0.05f, 0.100f, 0.500f);
  SetRgb(color.shortcut, 0.000f, 0.000f, 1.000f);

  SetRgb(color.fontactive, 1.000f, 1.000f, 1.000f);
  SetRgb(color.fontinactive, 0.500f, 0.500f, 0.500f);

  SetRgb(color.decorinactive, 0.300f, 0.300f, 0.300f);
  SetRgb(color.decorinactive2, 0.400f, 0.400f, 0.400f);

  SetRgb(color.button, 0.600f, 0.400f, 0.400f);
  SetRgb(color.check, 0.400f, 0.400f, 0.700f);
  SetRgb(color.radio, 0.400f, 0.600f, 0.400f);
  SetRgb(color.select, 0.400f, 0.600f, 0.600f);
  SetRgb(color.select2, 0.200f, 0.300f, 0.300f);
  SetRgb(color.slider, 0.200f, 0.100f, 0.600f);
  SetRgb(color.slider2, 0.300f, 0.200f, 0.800f);
  SetRgb(color.edit, 0.600f, 0.600f, 0.400f);
  SetRgb(color.edit2, 0.700f, 0.000f, 0.000f);
  SetRgb(color.cursor, 0.900f, 0.900f, 0.900f);

  sim->ui0.color = color;
  sim->ui1.color = color;

  mjuiThemeSpacing spacing = mjui_themeSpacing(0);
  spacing.total = 270;
  spacing.scroll = 15;
  spacing.label = 120;
  spacing.section = 8;
  spacing.cornersect = 6;
  spacing.cornersep = 6;
  spacing.itemside = 4;
  spacing.itemmid = 4;
  spacing.itemver = 4;
  spacing.texthor = 8;
  spacing.textver = 4;
  spacing.linescroll = 30;
  spacing.samples = 4;

  sim->ui0.spacing = spacing;
  sim->ui1.spacing = spacing;
  sim->font = kViewerFontIndex;

  if (refresh_ui)
  {
    mjr_changeFont(kViewerFontScale, &sim->platform_ui->mjr_context());
    mjui_resize(&sim->ui0, &sim->platform_ui->mjr_context());
    mjui_resize(&sim->ui1, &sim->platform_ui->mjr_context());
    mjui_update(-1, -1, &sim->ui0, &sim->uistate, &sim->platform_ui->mjr_context());
    mjui_update(-1, -1, &sim->ui1, &sim->uistate, &sim->platform_ui->mjr_context());
  }
}
} // namespace mujoco
