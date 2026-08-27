# 清理 CubeMX 重新生成后工作区的无关噪声
# 用法: powershell -ExecutionPolicy Bypass -File clean_workspace.ps1

$ErrorActionPreference = 'Continue'
Set-Location -LiteralPath $PSScriptRoot

if (-not (Test-Path '.git')) { throw 'not a git repository' }

$drv = @(git status --porcelain -- Drivers/ Middlewares/)
$restore = @()

if ($drv.Count -gt 0) {
  $restore += $drv | Where-Object { $_ -match '^ D ' } | ForEach-Object { $_.Substring(3) }

  $numstat = @(git diff --numstat -- Drivers/ Middlewares/ 2>$null)
  if ($LASTEXITCODE -ne 0) {
    Write-Warning 'git diff --numstat failed; keeping all modified files'
    $numstat = @()
  }
  $real = @($numstat | ForEach-Object { ($_ -split "`t")[2] })

  foreach ($m in $drv | Where-Object { $_ -match '^ M ' }) {
    $p = $m.Substring(3)
    if ($p -in $real) {
      Write-Warning "real change kept: $p"
    } else {
      $restore += $p
    }
  }
}

if ($restore.Count -gt 0) {
  git checkout -- $restore
  if ($LASTEXITCODE -ne 0) { throw 'git checkout failed' }
  Write-Host "restored: $($restore -join ', ')"
}

$req = @(
  'Drivers/STM32U5xx_HAL_Driver/Inc/stm32u5xx_hal_xspi.h',
  'Drivers/STM32U5xx_HAL_Driver/Inc/stm32u5xx_ll_dlyb.h',
  'Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_xspi.c',
  'Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_ll_dlyb.c'
)
foreach ($f in $req) { if (-not (Test-Path $f)) { Write-Warning "BSP depends on missing: $f" } }

$cmake = Get-Content 'CMakeLists.txt' -Raw
if ($cmake -notmatch 'stm32u5xx_hal_xspi\.c') { Write-Warning 'CMakeLists.txt missing stm32u5xx_hal_xspi.c (BSP requires it)' }
if ($cmake -notmatch 'stm32u5xx_ll_dlyb\.c') { Write-Warning 'CMakeLists.txt missing stm32u5xx_ll_dlyb.c (BSP requires it)' }

Write-Host '=== remaining changes ==='
git status --short
