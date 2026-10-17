$ErrorActionPreference = 'Stop'

$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $RepositoryRoot 'ProjW.uproject'
$UnrealEditorCmd = 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$AutomationRoot = Join-Path $RepositoryRoot 'Saved\Automation'
$LogRoot = Join-Path $RepositoryRoot 'Saved\Logs'
$DdcPath = Join-Path $RepositoryRoot 'Saved\AutomationDDC'
$ShaderWorkingPath = Join-Path $DdcPath 'ShaderWorking'
$RunId = Get-Date -Format 'yyyyMMdd-HHmmss'
$ReportPath = Join-Path $AutomationRoot "CodexValidation-$RunId"
$LogPath = Join-Path $LogRoot "CodexValidation-$RunId.log"
$ReportJson = Join-Path $ReportPath 'index.json'

foreach ($Directory in @($AutomationRoot, $LogRoot, $DdcPath, $ShaderWorkingPath, $ReportPath)) {
    New-Item -ItemType Directory -Force -Path $Directory | Out-Null
}

if (-not (Test-Path -LiteralPath $UnrealEditorCmd)) { throw "UnrealEditor-Cmd.exe not found: $UnrealEditorCmd" }
if (-not (Test-Path -LiteralPath $ProjectFile)) { throw "Project file not found: $ProjectFile" }

Write-Host "Running ProjW automation from $RepositoryRoot"
Write-Host "Report: $ReportPath"
Write-Host "Log:    $LogPath"

Push-Location $RepositoryRoot
try {
    & $UnrealEditorCmd $ProjectFile `
        '-d3d11' `
        "-LocalDataCachePath=$DdcPath" `
        "-ShaderWorkingDir=$ShaderWorkingPath" `
        '-unattended' '-nop4' '-nosound' `
        '-ExecCmds=Automation RunTests ProjW.' `
        '-TestExit=Automation Test Queue Empty' `
        "-ReportExportPath=$ReportPath" `
        "-abslog=$LogPath" `
        '-stdout' '-FullStdOutLogOutput'
    $UnrealExitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}

$Succeeded = $null
$Warnings = $null
$Failed = $null
$NotRun = $null
if (Test-Path -LiteralPath $ReportJson) {
    $Report = Get-Content -Raw -LiteralPath $ReportJson | ConvertFrom-Json
    $Succeeded = [int]$Report.succeeded
    $Warnings = [int]$Report.succeededWithWarnings
    $Failed = [int]$Report.failed
    $NotRun = [int]$Report.notRun
    Write-Host ("Summary: total={0}, passed={1}, warnings={2}, failed={3}, skipped/not-run={4}, process-exit={5}" -f ($Succeeded + $Warnings + $Failed + $NotRun), $Succeeded, $Warnings, $Failed, $NotRun, $UnrealExitCode)
}
else {
    Write-Host "Summary unavailable: automation report was not produced. process-exit=$UnrealExitCode"
}

if ($UnrealExitCode -ne 0 -or $null -eq $Failed -or $Failed -ne 0 -or $NotRun -ne 0) {
    exit 1
}
exit 0
