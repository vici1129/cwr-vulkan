#pragma once
#include <Poseidon/UI/Options/DisplayPage.hpp>
#include <Poseidon/UI/Settings/GraphicsConfig.hpp>

namespace Poseidon
{
// One staged notebook page; DisplayPage supplies the existing mode lists and
// display switching, not another store or a nested menu.
class VideoPage : public DisplayPage
{
  public:
    VideoPage();
    const char* TitleText() const override;
    void Mount(OptionsShell&) override;
    void Unmount(OptionsShell&) override;
    void OnReshown(OptionsShell&) override;
    void OnSimulate(OptionsShell&) override;
    bool OnButtonClicked(OptionsShell&, int) override;
    bool OnKeyDown(OptionsShell&, unsigned) override;
    // Stable logical identifiers; the scroll list maps these to visible slots.
    enum Row
    {
        DisplayHeader,
        Monitor,
        Mode,
        Resolution,
        Refresh,
        Vsync,
        Fps,
        Brightness,
        Gamma,
        Style,
        HudWidth,
        ImageHeader,
        AA,
        Scale,
        ViewDistance,
        MissionView,
        Terrain,
        Objects,
        GraphicsHeader,
        Overall,
        Shadows,
        Clouds,
        Water,
        AO,
        VolumetricLighting,
        ObjectShadows,
        VehicleShadows,
        Particles,
        Multitexturing,
        Apply,
        OK,
        Cancel
    };

  protected:
    OptionsScrollList::Provider& ProviderRef() override { return m_video; }
    int FindRow(Row row) const;

  private:
    class VideoProvider : public OptionsScrollList::Provider
    {
      public:
        VideoPage* page = nullptr;
        int RowCount() const override;
        const char* RowLabel(int) const override;
        const char* RowDescription(int) const override;
        OptionsScrollList::RowDef RowFor(int) const override;
        int RowValue(int) const override;
        void SetRowValue(int, int) override;
        OptionsScrollList::Kind RowKind(int) const override;
        bool IsDisabled(int) const override;
        bool IsPending(int) const override;
        const char* SliderValueText(int) const override;
        void OnRowAction(int, Display&) override;
        int PinnedFooterRow() const override { return RowCount() - 1; }
    } m_video;
    std::vector<Row> m_rows;
    std::vector<std::string> m_aaModes;
    mutable std::vector<std::string> m_choices;
    mutable std::vector<const char*> m_choicePtrs;
    mutable std::string m_valueText;
    GraphicsConfig m_graphics, m_saved, m_liveGraphics; // Staged, rollback, last applied.
    ImageQuality m_image, m_oldImage;
    GraphicsQuality m_quality, m_oldQuality;
    float m_view = 900, m_oldView = 900;
    bool m_missionView = true, m_oldMissionView = true;
    bool m_objectShadows = false, m_vehicleShadows = false, m_oldObjects = false, m_oldVehicles = false;
    bool m_vulkan = false, m_applying = false, m_close = false, m_retrySave = false;
    bool m_qualityTouched = false, m_imageTouched = false;
    bool m_shadowsTouched = false, m_waterTouched = false;
    int m_waitFrames = 0;
    std::string m_status, m_rtReason, m_fsrReason;
    void Capture();
    void CaptureDisplay();
    void ObserveWindowSize();
    void BuildRows();
    void Commit(OptionsShell&, bool close);
    void ApplySelection();
    bool Finish(OptionsShell&);
    void Restore();
    bool Dirty() const;
    bool Save();
    int DisplayRow(Row) const;
    std::vector<int> ScaleChoices() const;
};
} // namespace Poseidon
