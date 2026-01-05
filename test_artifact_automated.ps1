# Artifact Testing Automation Script
# Run this script in PowerShell after downloading the artifact
# Usage: .\test_artifact_automated.ps1 -ArtifactPath "C:\path\to\extracted\artifact"

param(
    [Parameter(Mandatory=$false)]
    [string]$ArtifactPath = ".\output",
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipGuiTest,
    
    [Parameter(Mandatory=$false)]
    [switch]$Verbose,
    
    [Parameter(Mandatory=$false)]
    [string]$LogFile = "artifact_test_results.log"
)

$ErrorActionPreference = "Continue"
$script:testsPassed = 0
$script:testsFailed = 0
$script:testsSkipped = 0
$script:logEntries = @()

# Color-coded logging function
function Write-TestLog {
    param(
        [string]$Message,
        [ValidateSet("INFO", "PASS", "FAIL", "WARN", "SKIP")]
        [string]$Level = "INFO"
    )
    
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logEntry = "[$timestamp] [$Level] $Message"
    $script:logEntries += $logEntry
    
    switch ($Level) {
        "PASS" { Write-Host $Message -ForegroundColor Green }
        "FAIL" { Write-Host $Message -ForegroundColor Red }
        "WARN" { Write-Host $Message -ForegroundColor Yellow }
        "SKIP" { Write-Host $Message -ForegroundColor Cyan }
        default { Write-Host $Message -ForegroundColor White }
    }
}

function Test-FileExists {
    param(
        [string]$Path,
        [string]$Description
    )
    
    if (Test-Path $Path) {
        Write-TestLog "[✓] $Description" -Level PASS
        $script:testsPassed++
        return $true
    } else {
        Write-TestLog "[✗] $Description - Not found: $Path" -Level FAIL
        $script:testsFailed++
        return $false
    }
}

function Test-DllExports {
    param(
        [string]$DllPath,
        [string]$SymbolName
    )
    
    $dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
    
    if (-not $dumpbin) {
        # Try to find dumpbin in Visual Studio installations
        $vsPaths = @(
            "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe",
            "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe",
            "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe",
            "C:\Program Files (x86)\Microsoft Visual Studio\2019\*\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe"
        )
        
        foreach ($path in $vsPaths) {
            $found = Get-Item $path -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($found) {
                $dumpbin = $found.FullName
                break
            }
        }
    } else {
        $dumpbin = $dumpbin.Source
    }
    
    if (-not $dumpbin) {
        Write-TestLog "[~] Cannot verify DLL exports: dumpbin.exe not found" -Level SKIP
        $script:testsSkipped++
        return $null
    }
    
    try {
        $exports = & $dumpbin /EXPORTS $DllPath 2>&1 | Out-String
        
        if ($exports -match [regex]::Escape($SymbolName)) {
            Write-TestLog "[✓] Symbol '$SymbolName' found in $(Split-Path $DllPath -Leaf)" -Level PASS
            $script:testsPassed++
            return $true
        } else {
            Write-TestLog "[✗] Symbol '$SymbolName' NOT found in $(Split-Path $DllPath -Leaf)" -Level FAIL
            $script:testsFailed++
            return $false
        }
    } catch {
        Write-TestLog "[✗] Error checking exports: $_" -Level FAIL
        $script:testsFailed++
        return $false
    }
}

function Test-QtConflicts {
    Write-TestLog "`n=== Checking for Qt conflicts in PATH ===" -Level INFO
    
    $pathDirs = $env:PATH -split ';'
    $qtConflicts = @()
    
    foreach ($dir in $pathDirs) {
        if ((Test-Path "$dir\Qt6Core.dll") -or (Test-Path "$dir\Qt5Core.dll")) {
            $qtConflicts += $dir
        }
    }
    
    if ($qtConflicts.Count -gt 0) {
        Write-TestLog "[!] WARNING: Found Qt installations in system PATH that may interfere:" -Level WARN
        foreach ($conflict in $qtConflicts) {
            Write-TestLog "    - $conflict" -Level WARN
        }
        Write-TestLog "    Recommendation: Always use StartAiFileSorter.exe to avoid conflicts" -Level WARN
        return $false
    } else {
        Write-TestLog "[✓] No conflicting Qt installations in system PATH" -Level PASS
        $script:testsPassed++
        return $true
    }
}

function Test-ApplicationStartup {
    param(
        [string]$ExecutablePath,
        [int]$TimeoutSeconds = 10
    )
    
    if ($SkipGuiTest) {
        Write-TestLog "[~] GUI test skipped (--SkipGuiTest)" -Level SKIP
        $script:testsSkipped++
        return $null
    }
    
    if ($env:CI -eq "true") {
        Write-TestLog "[~] Skipping GUI test in CI environment" -Level SKIP
        $script:testsSkipped++
        return $null
    }
    
    Write-TestLog "[→] Starting application: $ExecutablePath" -Level INFO
    Write-TestLog "    (Will wait $TimeoutSeconds seconds, then close automatically)" -Level INFO
    
    try {
        $process = Start-Process -FilePath $ExecutablePath -PassThru -ErrorAction Stop
        Start-Sleep -Seconds 3
        
        if ($process.HasExited) {
            $exitCode = $process.ExitCode
            Write-TestLog "[✗] Application exited immediately with code: $exitCode (crash or error)" -Level FAIL
            $script:testsFailed++
            return $false
        }
        
        Write-TestLog "[✓] Application started successfully (PID: $($process.Id))" -Level PASS
        $script:testsPassed++
        
        # Wait for timeout or manual close
        $timeoutMs = $TimeoutSeconds * 1000  # Convert seconds to milliseconds
        $process.WaitForExit($timeoutMs)
        
        if (-not $process.HasExited) {
            Write-TestLog "[→] Closing application after timeout..." -Level INFO
            $process.CloseMainWindow() | Out-Null
            Start-Sleep -Seconds 2
            
            if (-not $process.HasExited) {
                $process.Kill()
            }
        }
        
        return $true
    } catch {
        Write-TestLog "[✗] Failed to start application: $_" -Level FAIL
        $script:testsFailed++
        return $false
    }
}

function Get-SystemInfo {
    Write-TestLog "`n=== System Information ===" -Level INFO
    
    $os = Get-CimInstance Win32_OperatingSystem
    Write-TestLog "OS: $($os.Caption) ($($os.Version))" -Level INFO
    
    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    Write-TestLog "CPU: $($cpu.Name)" -Level INFO
    
    $memory = [math]::Round($os.TotalVisibleMemorySize / 1MB, 2)
    Write-TestLog "RAM: $memory GB" -Level INFO
    
    Write-TestLog "PowerShell: $($PSVersionTable.PSVersion)" -Level INFO
}

# ============================================================================
# Main Test Execution
# ============================================================================

Write-Host "`n" -NoNewline
Write-Host "╔═══════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║     AI File Sorter - Artifact Testing Automation v1.0        ║" -ForegroundColor Cyan
Write-Host "╚═══════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host "`n"

$startTime = Get-Date

# Resolve absolute path
$ArtifactPath = Resolve-Path $ArtifactPath -ErrorAction SilentlyContinue
if (-not $ArtifactPath) {
    Write-TestLog "[✗] Artifact path not found. Please provide valid path." -Level FAIL
    exit 1
}

Write-TestLog "Artifact Path: $ArtifactPath" -Level INFO
Write-TestLog "Test Started: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -Level INFO

# Collect system information
Get-SystemInfo

# ============================================================================
# TEST SUITE 1: File Inventory
# ============================================================================

Write-TestLog "`n=== TEST SUITE 1: File Inventory ===" -Level INFO

$criticalFiles = @(
    @{Path = "aifilesorter.exe"; Description = "Main application executable"},
    @{Path = "StartAiFileSorter.exe"; Description = "Application launcher (REQUIRED)"},
    @{Path = "Qt6Core.dll"; Description = "Qt Core library"},
    @{Path = "Qt6Gui.dll"; Description = "Qt GUI library"},
    @{Path = "Qt6Widgets.dll"; Description = "Qt Widgets library"},
    @{Path = "llama.dll"; Description = "Llama inference engine"},
    @{Path = "ggml.dll"; Description = "GGML math library"},
    @{Path = "ggml-base.dll"; Description = "GGML base library"},
    @{Path = "ggml-cpu.dll"; Description = "GGML CPU backend"}
)

foreach ($file in $criticalFiles) {
    $fullPath = Join-Path $ArtifactPath $file.Path
    Test-FileExists -Path $fullPath -Description $file.Description | Out-Null
}

# Check for critical directory structure
Write-TestLog "`n--- Checking directory structure for standalone operation ---" -Level INFO

$criticalDirs = @(
    @{Path = "lib\ggml\wocuda"; Description = "GGML runtime DLLs (CPU variant)"},
    @{Path = "lib\precompiled\cpu\bin"; Description = "Precompiled binaries directory"}
)

foreach ($dir in $criticalDirs) {
    $fullPath = Join-Path $ArtifactPath $dir.Path
    if (Test-Path $fullPath) {
        $fileCount = (Get-ChildItem $fullPath -File -ErrorAction SilentlyContinue).Count
        Write-TestLog "[✓] $($dir.Description): Found ($fileCount files)" -Level PASS
        $script:testsPassed++
    } else {
        Write-TestLog "[✗] $($dir.Description): Directory not found at $($dir.Path)" -Level FAIL
        Write-TestLog "    This directory is REQUIRED for standalone operation!" -Level FAIL
        $script:testsFailed++
    }
}

# ============================================================================
# TEST SUITE 2: DLL Version Verification
# ============================================================================

Write-TestLog "`n=== TEST SUITE 2: DLL Version Verification ===" -Level INFO

# Test for ggml_xielu symbol (required since llama.cpp b7130)
$ggmlDll = Join-Path $ArtifactPath "ggml.dll"
if (Test-Path $ggmlDll) {
    Test-DllExports -DllPath $ggmlDll -SymbolName "ggml_xielu" | Out-Null
} else {
    Write-TestLog "[✗] ggml.dll not found, cannot verify exports" -Level FAIL
    $script:testsFailed++
}

# Check Qt version (expected: 6.5.3 based on workflow configuration)
$expectedQtVersion = "6.5.3"  # Update this when Qt version changes in workflow
$qtCoreDll = Join-Path $ArtifactPath "Qt6Core.dll"
if (Test-Path $qtCoreDll) {
    try {
        $qtVersion = (Get-Item $qtCoreDll).VersionInfo.ProductVersion
        Write-TestLog "[✓] Qt6Core.dll version: $qtVersion" -Level PASS
        
        if ($qtVersion -like "$expectedQtVersion*") {
            Write-TestLog "[✓] Qt version matches expected build version ($expectedQtVersion)" -Level PASS
            $script:testsPassed++
        } else {
            Write-TestLog "[!] Qt version differs from build expectation ($expectedQtVersion vs $qtVersion)" -Level WARN
        }
    } catch {
        Write-TestLog "[✗] Failed to read Qt version: $_" -Level FAIL
        $script:testsFailed++
    }
} else {
    Write-TestLog "[✗] Qt6Core.dll not found" -Level FAIL
    $script:testsFailed++
}

# ============================================================================
# TEST SUITE 3: Environment Check
# ============================================================================

Write-TestLog "`n=== TEST SUITE 3: Environment Check ===" -Level INFO

# Check for Qt conflicts in PATH
Test-QtConflicts | Out-Null

# Check for Visual C++ Redistributable (optional but recommended)
# Check multiple versions: 2015-2022 (versions 14.x use same runtime)
$vcRedistPaths = @(
    "HKLM:\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64",  # 2015
    "HKLM:\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64"  # 2015 (32-bit registry)
)

$vcRedist = $null
foreach ($path in $vcRedistPaths) {
    $vcRedist = Get-ItemProperty $path -ErrorAction SilentlyContinue
    if ($vcRedist) {
        break
    }
}

if ($vcRedist) {
    $vcVersion = $vcRedist.Version
    Write-TestLog "[✓] Visual C++ Redistributable installed (version $vcVersion)" -Level PASS
    $script:testsPassed++
} else {
    Write-TestLog "[!] Visual C++ Redistributable 2015-2022 not detected (may cause issues)" -Level WARN
}

# ============================================================================
# TEST SUITE 4: Application Startup
# ============================================================================

Write-TestLog "`n=== TEST SUITE 4: Application Startup Test ===" -Level INFO

$starterExe = Join-Path $ArtifactPath "StartAiFileSorter.exe"
if (Test-Path $starterExe) {
    Test-ApplicationStartup -ExecutablePath $starterExe -TimeoutSeconds 10 | Out-Null
} else {
    Write-TestLog "[✗] StartAiFileSorter.exe not found, cannot test startup" -Level FAIL
    $script:testsFailed++
}

# ============================================================================
# TEST SUITE 5: Log File Analysis
# ============================================================================

Write-TestLog "`n=== TEST SUITE 5: Log File Analysis ===" -Level INFO

$logDir = "$env:APPDATA\aifilesorter\logs"
if (Test-Path $logDir) {
    $logFiles = Get-ChildItem $logDir -Filter "*.log" -ErrorAction SilentlyContinue
    
    if ($logFiles) {
        Write-TestLog "[✓] Found $($logFiles.Count) log file(s) in $logDir" -Level PASS
        
        # Check latest log for errors
        $latestLog = $logFiles | Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if ($latestLog) {
            $logContent = Get-Content $latestLog.FullName -ErrorAction SilentlyContinue
            $errorLines = $logContent | Select-String -Pattern "ERROR|CRITICAL|FATAL" -CaseSensitive:$false
            
            if ($errorLines) {
                Write-TestLog "[!] Found $($errorLines.Count) error/critical entries in latest log:" -Level WARN
                foreach ($line in $errorLines | Select-Object -First 5) {
                    Write-TestLog "    $line" -Level WARN
                }
            } else {
                Write-TestLog "[✓] No errors found in latest log file" -Level PASS
                $script:testsPassed++
            }
        }
    } else {
        Write-TestLog "[~] No log files found (application may not have run yet)" -Level SKIP
        $script:testsSkipped++
    }
} else {
    Write-TestLog "[~] Log directory doesn't exist yet (application not run)" -Level SKIP
    $script:testsSkipped++
}

# ============================================================================
# Test Summary and Reporting
# ============================================================================

$endTime = Get-Date
$duration = ($endTime - $startTime).TotalSeconds

Write-Host "`n" -NoNewline
Write-Host "╔═══════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║                      TEST SUMMARY                             ║" -ForegroundColor Cyan
Write-Host "╚═══════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host "`n"

Write-TestLog "Tests Passed:  $script:testsPassed" -Level PASS
Write-TestLog "Tests Failed:  $script:testsFailed" -Level FAIL
Write-TestLog "Tests Skipped: $script:testsSkipped" -Level SKIP
Write-TestLog "Duration:      $([math]::Round($duration, 2)) seconds" -Level INFO

$totalTests = $script:testsPassed + $script:testsFailed
if ($totalTests -gt 0) {
    $successRate = [math]::Round(($script:testsPassed / $totalTests) * 100, 1)
    Write-TestLog "Success Rate:  $successRate%" -Level INFO
}

# Save log to file
$script:logEntries | Out-File -FilePath $LogFile -Encoding UTF8
Write-TestLog "`nTest log saved to: $LogFile" -Level INFO

# Final verdict
Write-Host "`n"
if ($script:testsFailed -eq 0) {
    Write-Host "╔═══════════════════════════════════════════════════════════════╗" -ForegroundColor Green
    Write-Host "║  [✓] SUCCESS: All tests passed!                              ║" -ForegroundColor Green
    Write-Host "║      Artifact appears to be working correctly.                ║" -ForegroundColor Green
    Write-Host "╚═══════════════════════════════════════════════════════════════╝" -ForegroundColor Green
    exit 0
} else {
    Write-Host "╔═══════════════════════════════════════════════════════════════╗" -ForegroundColor Red
    Write-Host "║  [✗] FAILURE: Some tests failed.                             ║" -ForegroundColor Red
    Write-Host "║      Review the log above for details.                        ║" -ForegroundColor Red
    Write-Host "╚═══════════════════════════════════════════════════════════════╝" -ForegroundColor Red
    exit 1
}
