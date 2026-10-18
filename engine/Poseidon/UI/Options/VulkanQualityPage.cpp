#include <Poseidon/UI/Options/VulkanQualityPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <Poseidon/UI/OptionsUICommon.hpp>
#include <Poseidon/UI/Options/GraphicsPage.hpp>
#include <Poseidon/UI/Settings/GraphicsConfig.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Foundation/Common/GamePaths.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <SDL3/SDL_keycode.h>

namespace Poseidon
{
namespace
{
const char* const overall[] = {"Standard", "Enhanced", "Custom"};
const char* const shadows[] = {"Standard", "Enhanced", "Ray Traced", "Custom"};
const char* const rasterShadows[] = {"Standard", "Enhanced", "Custom"};
const char* const group[] = {"Standard", "Enhanced", "Custom"};
const char* const ao[] = {"Off", "On"};
std::string QualityConfigPath() { return GamePaths::Instance().UserDir() + "graphics.cfg"; }
}

VulkanQualityPage::VulkanQualityPage()
{
    m_provider.page = this;
    if (GEngine)
    {
        m_pending = m_applied = GEngine->GetGraphicsQuality();
        m_rtReason = static_cast<const char*>(GEngine->GraphicsQualityRTUnavailable());
    }
}

void VulkanQualityPage::Mount(OptionsShell& shell)
{
    if (GEngine)
    {
        m_pending = m_applied = GEngine->GetGraphicsQuality();
        m_rtReason = static_cast<const char*>(GEngine->GraphicsQualityRTUnavailable());
    }
    ScrollListPage::Mount(shell);
}

const char* VulkanQualityPage::QualityProvider::RowLabel(int row) const
{
    static const char* labels[] = {"Overall Quality", "Shadow Quality", "Cloud Quality", "Water Quality",
                                   "Ambient Occlusion", "Display & detail", "Apply", "OK", "Cancel"};
    return row >= 0 && row < 9 ? labels[row] : "";
}

const char* VulkanQualityPage::QualityProvider::RowDescription(int row) const
{
    if (!page->m_status.empty()) return page->m_status.c_str();
    switch (row)
    {
    case 0: return "Standard or Enhanced sets all four groups. Changes take effect with Apply or OK.";
    case 1: return page->m_rtReason.empty()
        ? "Standard: CSM. Enhanced: soft, contact and local shadows. Ray Traced adds soft sunlight and dynamic casters."
        : page->m_rtReason.c_str();
    case 2: return "Standard: original clouds. Enhanced: volumetric clouds and synchronized cloud shadows.";
    case 3: return "Standard: original water. Enhanced: refraction, shoreline foam and rain ripples.";
    case 4: return "Existing SSAO, with fixed validated parameters. Independent of hardware ray tracing.";
    case 5: return "Existing display, brightness, gamma, VSync, frame cap, view distance and scene-detail controls.";
    case 6: return "Apply and save the complete selection. Other video settings keep their current values.";
    case 7: return "Apply, save and return.";
    case 8: return "Discard changes made since the last Apply and return. Escape also cancels.";
    }
    return "";
}

OptionsScrollList::RowDef VulkanQualityPage::QualityProvider::RowFor(int row) const
{
    switch (row)
    {
    case 0: return {502, overall, 3};
    case 1:
        if (!page->m_rtReason.empty()) return {512, rasterShadows, page->m_pending.shadows < 0 ? 3 : 2};
        return {512, shadows, page->m_pending.shadows < 0 ? 4 : 3};
    case 2: return {522, group, page->m_pending.clouds < 0 ? 3 : 2};
    case 3: return {532, group, page->m_pending.water < 0 ? 3 : 2};
    case 4: return {542, ao, 2};
    default: return {-1, nullptr, 0};
    }
}

int VulkanQualityPage::QualityProvider::RowValue(int row) const
{
    const auto& q = page->m_pending;
    switch (row)
    {
    case 0: return q.Preset();
    case 1: return q.shadows < 0 ? (page->m_rtReason.empty() ? 3 : 2) : q.shadows;
    case 2: return q.clouds < 0 ? 2 : q.clouds;
    case 3: return q.water < 0 ? 2 : q.water;
    case 4: return q.ao;
    default: return 0;
    }
}

void VulkanQualityPage::QualityProvider::SetRowValue(int row, int value)
{
    if (IsDisabled(row)) return;
    auto& q = page->m_pending;
    switch (row)
    {
    case 0:
        // Custom is derived. Skip it when cycling a matching preset in
        // either direction, rather than trapping the stepper at an endpoint.
        if (value == 2 && q.Preset() != 2) value = 1 - q.Preset();
        if (value == 0) q = {}; else if (value == 1) q = GraphicsQuality::Enhanced();
        break;
    case 1: if (value >= 0 && value <= (page->m_rtReason.empty() ? 2 : 1)) q.shadows = value; break;
    case 2: if (value >= 0 && value <= 1) q.clouds = value; break;
    case 3: if (value >= 0 && value <= 1) q.water = value; break;
    case 4: if (value == 0 || value == 1) q.ao = value != 0; break;
    }
    page->m_status.clear();
}

OptionsScrollList::Kind VulkanQualityPage::QualityProvider::RowKind(int row) const
{
    return row >= 5 ? OptionsScrollList::KindAction : (row == 4 ? OptionsScrollList::KindBoolean : OptionsScrollList::KindStepper);
}
bool VulkanQualityPage::QualityProvider::IsDisabled(int row) const
{
    return page->m_applying || !GEngine || !GEngine->HasVulkanGraphics() ||
           (row == 6 && page->m_pending == page->m_applied && page->m_status.empty());
}
bool VulkanQualityPage::QualityProvider::IsPending(int row) const
{
    return row < 5 && page->m_pending != page->m_applied;
}
void VulkanQualityPage::QualityProvider::OnRowAction(int row, Display& host)
{
    if (IsDisabled(row)) return;
    if (row == 8) host.OnButtonClicked(IDC_CANCEL);
    else if (row == 6 || row == 7) page->Commit(row == 7);
    else if (row == 5) host.CreateChild(new OptionsShell(&host, host.SimulationEnabled(), false, std::make_unique<GraphicsPage>()));
}

bool VulkanQualityPage::Commit(bool close)
{
    if (!GEngine || m_applying) return false;
    if (!GEngine->ApplyGraphicsQuality(m_pending))
    {
        m_status = static_cast<const char*>(GEngine->GraphicsQualityError());
        m_pending = m_applied = GEngine->GetGraphicsQuality();
        return false;
    }
    m_applying = true;
    m_closeAfterApply = close;
    m_status = "Applying graphics...";
    return true;
}

void VulkanQualityPage::OnSimulate(OptionsShell& shell)
{
    if (m_applying && GEngine && !GEngine->GraphicsQualityPending())
    {
        m_applying = false;
        m_status = static_cast<const char*>(GEngine->GraphicsQualityError());
        const auto active = GEngine->GetGraphicsQuality();
        if (m_status.empty() && active == m_pending)
        {
            // Reload to preserve display/detail changes made on other pages.
            GraphicsConfig cfg;
            cfg.Load(QualityConfigPath());
            cfg.vulkan = active;
            if (!cfg.Save(QualityConfigPath())) m_status = "Graphics are active, but graphics.cfg could not be saved. Retry Apply.";
            else
            {
                m_applied = active;
                if (m_closeAfterApply) { shell.PopPage(); return; }
                m_status = "Graphics applied and saved.";
            }
        }
        else m_pending = m_applied = active; // failed requests never reach disk
    }
    if (!m_applying && GEngine && m_pending == m_applied)
        m_pending = m_applied = GEngine->GetGraphicsQuality();
    if (List()) List()->RenderPage();
    ScrollListPage::OnSimulate(shell);
}

bool VulkanQualityPage::OnButtonClicked(OptionsShell& shell, int idc)
{
    if (idc == IDC_CANCEL && m_applying) return true;
    return ScrollListPage::OnButtonClicked(shell, idc);
}
bool VulkanQualityPage::OnKeyDown(OptionsShell& shell, unsigned key)
{
    if (key == SDLK_ESCAPE && m_applying) return true;
    return ScrollListPage::OnKeyDown(shell, key);
}
}
