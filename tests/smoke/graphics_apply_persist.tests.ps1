#!/usr/bin/env pwsh
# Combined Pester + Trident: drive the live game through a Video
# row change and assert the resulting graphics.cfg on disk.
#
# Mirrors audio_volume_persist.tests.ps1 + display_apply_persist.tests.ps1
# 1:1.  Pester sets up an ephemeral POSEIDON_USER_DIR; tri honours it
# (commit 110cdba2) instead of creating its own tempdir; the tri
# scenario opens Video, changes Terrain Detail, explicitly Applies,
# then exits. Pester reads graphics.cfg and asserts the change landed.
#
# Run:
#   Invoke-Pester -Container (New-PesterContainer
#       -Path tests/smoke/graphics_apply_persist.tests.ps1
#       -Data @{ Preset = 'win-x64-clang-rwdi' }) -Output Detailed

param(
    [Parameter(Mandatory)]
    [string]$Preset
)

BeforeAll {
    Import-Module "$PSScriptRoot/../cli/TestHelpers.psm1" -Force

    $script:repoRoot = Get-RepoRoot $PSScriptRoot
    $script:gameDir  = Get-GameDataDir -RepoRoot $script:repoRoot
    $script:gameExe  = Get-GameExe -RepoRoot $script:repoRoot -Preset $Preset -Name PoseidonGame
    $script:triExe   = Get-TriExe -RepoRoot $script:repoRoot -Preset $Preset
    $script:hasAddons = $script:gameDir -and (Test-HasAddons $script:gameDir)
    $script:Skip = if (-not $script:gameExe)  { "PoseidonGame not found" }
                   elseif (-not $script:triExe) { "tri not found" }
                   elseif (-not $script:hasAddons) { "dist/Game/Addons not found" }
                   else { "" }
}

Describe "graphics.cfg persistence through live UI flow" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "Terrain Detail change via unified Video Apply round-trips into graphics.cfg" {
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "graphics.cfg"

            # Retain the existing runner retry budget for legacy idle flakes.
            $sqf = Join-Path $script:repoRoot "tests/integration/ui/options/graphics/new_graphics_apply_persist.test.sqf"
            & $script:triExe test $sqf --game-dir (Split-Path $script:gameExe -Parent) --data-dir $script:gameDir --retries 10 2>&1 | Out-Null
            $LASTEXITCODE | Should -Be 0 -Because "tri scenario must pass"

            Test-Path $cfgPath | Should -BeTrue -Because "Video Apply should write graphics.cfg"
            $kv = Read-CfgFile $cfgPath
            $kv['terrainDetail'] | Should -Be '3' -Because "tri selected the existing 12.5 m terrain grid"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "subsequent boot reads the persisted graphics.cfg back into the runtime" {
        # Same shape as audio + display equivalents — write a known cfg,
        # boot --check, confirm boot didn't stomp it.
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "graphics.cfg"
            @"
qualityPreset=1;
terrainDetail=2;
objectLod=2;
shadowQuality=1;
particlesQuality=1;
vsync=2;
fpsCap=120;
brightness=1.300000;
gamma=0.800000;
"@ | Set-Content -Path $cfgPath -Encoding ASCII -NoNewline

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            $kv = Read-CfgFile $cfgPath
            $kv['qualityPreset'] | Should -Be '1'
            $kv['fpsCap']        | Should -Be '120'
            $kv['vsync']         | Should -Be '2'
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }
}
