#pragma once

#include <Poseidon/UI/Options/ScrollListPage.hpp>
#include <Poseidon/Graphics/Core/GraphicsQuality.hpp>
#include <string>

namespace Poseidon
{
// Uses the existing notebook slots, input model and graphics.cfg store.
class VulkanQualityPage : public ScrollListPage
{
public:
    VulkanQualityPage();
    const char* TitleText() const override { return "Vulkan Graphics"; }
    void Mount(OptionsShell& shell) override;
    void OnSimulate(OptionsShell& shell) override;
    bool OnButtonClicked(OptionsShell& shell, int idc) override;
    bool OnKeyDown(OptionsShell& shell, unsigned key) override;

protected:
    OptionsScrollList::Provider& ProviderRef() override { return m_provider; }

private:
    class QualityProvider : public OptionsScrollList::Provider
    {
    public:
        VulkanQualityPage* page = nullptr;
        int RowCount() const override { return 9; }
        const char* RowLabel(int row) const override;
        const char* RowDescription(int row) const override;
        OptionsScrollList::RowDef RowFor(int row) const override;
        int RowValue(int row) const override;
        void SetRowValue(int row, int value) override;
        OptionsScrollList::Kind RowKind(int row) const override;
        bool IsDisabled(int row) const override;
        bool IsPending(int row) const override;
        void OnRowAction(int row, Display& host) override;
    } m_provider;

    GraphicsQuality m_pending, m_applied;
    std::string m_status, m_rtReason;
    bool m_applying = false, m_closeAfterApply = false;
    bool Commit(bool close);
};
}
