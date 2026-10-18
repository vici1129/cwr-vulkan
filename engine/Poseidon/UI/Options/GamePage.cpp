#include <Poseidon/UI/Options/GamePage.hpp>

#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/Core/Config/UserConfig.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Game/Chat.hpp>
#include <Poseidon/UI/Settings/GameSettingsConfig.hpp>

#include <Poseidon/UI/Locale/SupportedLanguages.hpp>
#include <Poseidon/UI/Locale/LanguageRegistry.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>

#include <algorithm>
#include <cmath>
#include <stdio.h>
#include <vector>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

void ApplyCurrentMissionViewDistance();

namespace
{
int CurrentLanguageIndex(const std::string& language)
{
    const int idx = CfgLib::FindSupportedLanguageIndex(language);
    return idx >= 0 ? idx : 0;
}

int ViewDistanceToSliderImpl(float value)
{
    float clamped = ClampPreferredViewDistance(value);
    return (int)std::lround((clamped - GameSettingsConfig::kMinViewDistance) /
                            (GameSettingsConfig::kMaxViewDistance - GameSettingsConfig::kMinViewDistance) * 100.0f);
}

float SliderToViewDistanceImpl(int slider)
{
    int clamped = std::clamp(slider, 0, 100);
    float value = GameSettingsConfig::kMinViewDistance +
                  (clamped / 100.0f) * (GameSettingsConfig::kMaxViewDistance - GameSettingsConfig::kMinViewDistance);
    return std::round(value / 100.0f) * 100.0f;
}
} // namespace

const char* GamePage::TitleText() const
{
    return LocalizeString("STR_DISP_MAIN_OPT_GAME");
}

int GamePage::ViewDistanceToSlider(float value)
{
    return ViewDistanceToSliderImpl(value);
}

float GamePage::SliderToViewDistance(int slider)
{
    return SliderToViewDistanceImpl(slider);
}

const char* GamePage::CloseLabel()
{
    return LocalizeString("STR_DISP_CLOSE");
}

const char* GamePage::CloseDescription()
{
    return LocalizeString("STR_DISP_MAIN_OPT_CLOSE_DESC");
}

void GamePage::OnReshown(OptionsShell& shell)
{
    m_provider.SetCloseTexts(CloseLabel(), CloseDescription());
    ScrollListPage::OnReshown(shell);
}

void GamePage::Unmount(OptionsShell& shell)
{
    SaveGameSettings();
    UserConfig_SaveDifficulties(USER_CONFIG);
    ScrollListPage::Unmount(shell);
}

const char* GamePage::GameProvider::RowLabel(int row) const
{
    switch (row)
    {
        case kRowTextLanguage:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_TEXT_LANGUAGE");
        case kRowVoiceLanguage:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_VOICE_LANGUAGE");
        case kRowBlood:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_BLOOD");
        case kRowSubtitles:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_SUBTITLES");
        case kRowRadioSubtitles:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_RADIO_SUBTITLES");
        default:
            return "";
    }
}

const char* GamePage::GameProvider::RowDescription(int row) const
{
    switch (row)
    {
        case kRowTextLanguage:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_TEXT_LANGUAGE_DESC");
        case kRowVoiceLanguage:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_VOICE_LANGUAGE_DESC");
        case kRowBlood:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_BLOOD_DESC");
        case kRowSubtitles:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_SUBTITLES_DESC");
        case kRowRadioSubtitles:
            return LocalizeString("STR_DISP_MAIN_OPT_GAME_RADIO_SUBTITLES_DESC");
        default:
            return "";
    }
}

OptionsScrollList::RowDef GamePage::GameProvider::RowFor(int row) const
{
    RefreshToggleTexts();

    switch (row)
    {
        case kRowTextLanguage:
            return {502, CfgLib::LanguageRegistry::Instance().DisplayNamePtrs(),
                    CfgLib::LanguageRegistry::Instance().Count()};
        case kRowVoiceLanguage:
            // Only languages the game provides dubbing for (hasVoice), not the full text set.
            return {512, CfgLib::LanguageRegistry::Instance().VoiceDisplayNamePtrs(),
                    CfgLib::LanguageRegistry::Instance().VoiceCount()};
        case kRowBlood:
            return {542, m_toggleOptions.data(), 2};
        case kRowSubtitles:
            return {552, m_toggleOptions.data(), 2};
        case kRowRadioSubtitles:
            return {562, m_toggleOptions.data(), 2};
        default:
            return {-1, nullptr, 0};
    }
}

void GamePage::GameProvider::RefreshToggleTexts() const
{
    m_toggleText[0] = LocalizeString("STR_DISABLED");
    m_toggleText[1] = LocalizeString("STR_ENABLED");
    m_toggleOptions[0] = m_toggleText[0].c_str();
    m_toggleOptions[1] = m_toggleText[1].c_str();
}

int GamePage::GameProvider::RowValue(int row) const
{
    switch (row)
    {
        case kRowTextLanguage:
            return CurrentLanguageIndex((const char*)GLanguage);
        case kRowVoiceLanguage:
        {
            // Index into the voice-capable subset; fall back to the first (English) if the
            // persisted voice language isn't voice-capable (e.g. an old config or a text-only pick).
            const int v = CfgLib::LanguageRegistry::Instance().VoiceIndexOf(GetSelectedVoiceLanguage());
            return v < 0 ? 0 : v;
        }
        case kRowBlood:
            return ENGINE_CONFIG.blood ? 1 : 0;
        case kRowSubtitles:
            return USER_CONFIG.showTitles ? 1 : 0;
        case kRowRadioSubtitles:
            return GChatList.Enabled() ? 1 : 0;
        default:
            return 0;
    }
}

void GamePage::GameProvider::SetRowValue(int row, int value)
{
    if (row == kRowBlood || row == kRowSubtitles || row == kRowRadioSubtitles)
    {
        if (value < 0 || value >= 2)
            return;
    }
    else if (row == kRowVoiceLanguage)
    {
        // Voice row indexes the voice-capable subset, not the full language set.
        if (value < 0 || value >= CfgLib::LanguageRegistry::Instance().VoiceCount())
            return;
    }
    else if (value < 0 || value >= CfgLib::LanguageRegistry::Instance().Count())
    {
        return;
    }

    switch (row)
    {
        case kRowTextLanguage:
            SetLanguage(RString(CfgLib::LanguageRegistry::Instance().Languages()[value].name.c_str()));
            return;
        case kRowVoiceLanguage:
            SetSelectedVoiceLanguage(CfgLib::LanguageRegistry::Instance().VoiceNameAt(value));
            return;
        case kRowBlood:
            ENGINE_CONFIG.blood = value != 0;
            return;
        case kRowSubtitles:
            USER_CONFIG.showTitles = value != 0;
            return;
        case kRowRadioSubtitles:
            GChatList.Enable(value != 0);
            return;
    }
}

OptionsScrollList::Kind GamePage::GameProvider::RowKind(int row) const
{
    if (row == kRowBlood || row == kRowSubtitles || row == kRowRadioSubtitles)
        return OptionsScrollList::KindBoolean;
    return OptionsScrollList::Provider::RowKind(row);
}

} // namespace Poseidon
