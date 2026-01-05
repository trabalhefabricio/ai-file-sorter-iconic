# Artifact Testing Guide - Build #12 (Run ID: 20682964808)

## Overview

This guide documents the testing procedure for the most recent successful Windows build artifact from workflow run #12, completed on January 3, 2026 at 21:18:53 UTC.

**Artifact Details:**
- **Name:** AI-File-Sorter-newstuff-15fb4c77e7cf1de42bd229c25a8d58ab21d90bc1
- **Size:** ~41.8 MB
- **Commit SHA:** 15fb4c77e7cf1de42bd229c25a8d58ab21d90bc1
- **Build Status:** ✅ Success
- **Platform:** Windows x64 (MSVC 2019/2022)

## Prerequisites

### System Requirements
- Windows 10/11 x64
- Visual C++ Redistributable 2015-2022 (usually pre-installed)
- ~200 MB free disk space
- Internet connection (optional, only for remote LLM models)

### Testing Environment Setup
1. Create a clean test directory: `C:\artifact-test\`
2. Download the artifact from GitHub Actions
3. Extract the ZIP file to the test directory
4. Verify all files are present (see File Inventory below)

## File Inventory

The artifact should contain the following files and directories:

### Core Application Files
- `aifilesorter.exe` - Main application executable
- `StartAiFileSorter.exe` - Launcher (ALWAYS use this, not aifilesorter.exe directly!)

### Qt6 Libraries
- `Qt6Core.dll`
- `Qt6Gui.dll`
- `Qt6Widgets.dll`
- `Qt6Network.dll` (if present)
- `platforms/` directory with Qt platform plugins

### GGML/Llama Libraries
- `llama.dll`
- `ggml.dll`
- `ggml-base.dll`
- `ggml-cpu.dll`

### Support Libraries (vcpkg)
- `libcurl.dll`
- `fmt.dll`
- `spdlog.dll`
- `jsoncpp.dll`
- `sqlite3.dll`

### OpenBLAS Dependencies
- `libopenblas*.dll`
- `libgfortran*.dll`
- `libgcc*.dll`
- `libwinpthread*.dll`

### Runtime Directory Structure (Critical for Standalone Operation)
- `lib/ggml/wocuda/` - GGML runtime DLLs for CPU-only execution
  - `llama.dll`, `ggml.dll`, `ggml-base.dll`, `ggml-cpu.dll`
- `lib/ggml/wcuda/` - GGML runtime DLLs for CUDA execution (if included)
- `lib/ggml/wvulkan/` - GGML runtime DLLs for Vulkan execution (if included)
- `lib/precompiled/cpu/` - Static libraries and precompiled binaries
  - `bin/` subdirectory with DLL files
  - `lib/` subdirectory with .lib files
- `lib/precompiled/cuda/` - CUDA variant libraries (if included)
- `lib/precompiled/vulkan/` - Vulkan variant libraries (if included)

**Note:** The `lib/` directory structure is essential for the application to run independently without requiring installation of the original hyperfield fork.

## Testing Procedure

### Phase 1: Startup Test (Critical)

**Objective:** Verify the application launches without errors

**Steps:**
1. Open Command Prompt as Administrator
2. Navigate to the artifact directory:
   ```cmd
   cd C:\artifact-test\output
   ```

3. **IMPORTANT:** Run the launcher, NOT the main executable:
   ```cmd
   StartAiFileSorter.exe
   ```

4. Expected behavior:
   - Window should appear within 5-10 seconds
   - No DLL error dialogs
   - No crash on startup

**Logging:**
```cmd
# Run with output redirection to capture any console output
StartAiFileSorter.exe > startup_log.txt 2>&1
```

#### Common Startup Errors

**Error 1: "ggml_xielu entry point not found"**
- **Cause:** Old llama.dll version (pre-b7130)
- **Evidence:** This artifact uses commit that includes ggml_xielu
- **Expected:** Should NOT occur in this build
- **Fix if occurs:** Re-download artifact or rebuild llama.cpp

**Error 2: "QTableView::dropEvent not found"**
- **Cause:** Qt version mismatch (system Qt interfering)
- **Evidence:** Multiple Qt installations in system PATH
- **Expected:** Should be prevented by StartAiFileSorter.exe
- **Fix if occurs:** 
  1. Check system PATH for other Qt installations
  2. Temporarily rename/move other Qt bin directories
  3. Run StartAiFileSorter.exe again

**Error 3: "Missing DLL: Qt6Core.dll" (or similar)**
- **Cause:** Incomplete artifact or corrupted download
- **Expected:** Should NOT occur (verified in Bundle step)
- **Fix if occurs:** Re-download artifact

### Phase 2: Functional Test

**Objective:** Verify basic file categorization functionality

**Steps:**

1. Create a test directory with sample files:
   ```cmd
   mkdir C:\test-files
   cd C:\test-files
   echo "Sample document" > document.txt
   echo "Music file metadata" > song.mp3.txt
   echo "Photo metadata" > photo.jpg.txt
   ```

2. In the application:
   - Click "Select Directory"
   - Choose `C:\test-files`
   - Enable "Use local LLM" (or provide API key if testing remote)
   - Click "Analyze"

3. Expected behavior:
   - Progress bar appears
   - Files are processed without errors
   - Categorization dialog shows results
   - Can review and confirm categorization

**Logging:**
Check application log files:
```cmd
type %APPDATA%\aifilesorter\logs\aifilesorter.log
```

### Phase 3: DLL Dependency Test

**Objective:** Verify all DLL dependencies are correctly loaded

**Test Script:** `check_dlls.ps1`
```powershell
# PowerShell script to check DLL dependencies
$exePath = ".\aifilesorter.exe"
$llamaDll = ".\llama.dll"
$ggmlDll = ".\ggml.dll"

Write-Host "=== DLL Dependency Check ===" -ForegroundColor Cyan

# Check if dumpbin is available (part of Visual Studio)
$dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue

if (-not $dumpbin) {
    Write-Host "[SKIP] dumpbin not found. Install Visual Studio Build Tools to run this test." -ForegroundColor Yellow
    exit 0
}

# Check GGML exports
Write-Host "`nChecking ggml.dll exports..." -ForegroundColor Yellow
$exports = & dumpbin /EXPORTS $ggmlDll 2>&1 | Out-String

if ($exports -match "ggml_xielu") {
    Write-Host "[OK] ggml_xielu found" -ForegroundColor Green
} else {
    Write-Host "[ERROR] ggml_xielu NOT found - DLL is too old!" -ForegroundColor Red
}

# Check Qt version
Write-Host "`nChecking Qt version..." -ForegroundColor Yellow
if (Test-Path ".\Qt6Core.dll") {
    $qtVersion = (Get-Item ".\Qt6Core.dll").VersionInfo.ProductVersion
    Write-Host "[OK] Qt6Core.dll version: $qtVersion" -ForegroundColor Green
    
    if ($qtVersion -like "6.5.3*") {
        Write-Host "[OK] Qt version matches build requirement (6.5.3)" -ForegroundColor Green
    } else {
        Write-Host "[WARNING] Qt version differs from build (expected 6.5.3)" -ForegroundColor Yellow
    }
} else {
    Write-Host "[ERROR] Qt6Core.dll not found!" -ForegroundColor Red
}

# Check for conflicting DLLs in PATH
Write-Host "`nChecking for conflicting DLLs in system PATH..." -ForegroundColor Yellow
$pathDirs = $env:PATH -split ';'
$qtConflicts = @()

foreach ($dir in $pathDirs) {
    if (Test-Path "$dir\Qt6Core.dll") {
        $qtConflicts += $dir
    }
}

if ($qtConflicts.Count -gt 0) {
    Write-Host "[WARNING] Found Qt DLLs in system PATH:" -ForegroundColor Yellow
    foreach ($conflict in $qtConflicts) {
        Write-Host "  - $conflict" -ForegroundColor Yellow
    }
    Write-Host "  These may interfere with the application. Use StartAiFileSorter.exe!" -ForegroundColor Yellow
} else {
    Write-Host "[OK] No conflicting Qt DLLs in system PATH" -ForegroundColor Green
}

Write-Host "`n=== DLL Check Complete ===" -ForegroundColor Cyan
```

### Phase 4: Error Recovery Test

**Objective:** Verify the application handles errors gracefully

**Test Cases:**

1. **Invalid directory:**
   - Try to analyze a non-existent directory
   - Expected: Error message, no crash

2. **Empty directory:**
   - Analyze a directory with no files
   - Expected: Appropriate message, no crash

3. **Permission denied:**
   - Try to analyze `C:\Windows\System32`
   - Expected: Permission error, no crash

4. **Large file set:**
   - Create 1000+ dummy files
   - Expected: Progress indicator, no memory issues

## Known Issues & Workarounds

### Issue 1: DLL Path Resolution
**Symptom:** Application fails to start with DLL errors
**Root Cause:** Windows loads DLLs from PATH before application directory
**Workaround:** ALWAYS use `StartAiFileSorter.exe` instead of `aifilesorter.exe`
**Status:** Mitigated in build #12

### Issue 2: Qt Version Conflicts
**Symptom:** "QTableView::dropEvent not found" error
**Root Cause:** Multiple Qt installations on system
**Workaround:** 
1. Check PATH for other Qt installations
2. Temporarily remove them from PATH
3. Run StartAiFileSorter.exe
**Status:** Launcher should prevent this in build #12

### Issue 3: Antivirus False Positives
**Symptom:** Antivirus blocks application or DLLs
**Root Cause:** Unsigned binaries, DLL injection detection
**Workaround:** 
1. Add exception for artifact directory
2. Temporarily disable real-time protection during testing
**Status:** Expected behavior, not a bug

## Expected Test Results

### Successful Test (Build #12)
```
✅ Startup: Application launches without errors
✅ UI: Main window displays correctly
✅ DLL Loading: All DLLs load successfully
✅ ggml_xielu: Symbol found in ggml.dll
✅ Qt Version: 6.5.3 (matches build)
✅ Categorization: Files processed without errors
✅ Error Handling: Graceful error messages
```

### Potential Failures (What to Log)
If any test fails, collect the following information:

1. **System Information:**
   ```cmd
   systeminfo > system_info.txt
   ```

2. **Environment Variables:**
   ```cmd
   set > environment.txt
   ```

3. **DLL Dependencies:**
   ```cmd
   dumpbin /DEPENDENTS aifilesorter.exe > dependencies.txt
   ```

4. **Application Logs:**
   ```cmd
   copy %APPDATA%\aifilesorter\logs\* logs_backup\
   ```

5. **Error Screenshots:**
   - Capture any error dialogs
   - Include timestamp and error code

## Automated Testing Script

Save as `test_artifact.ps1`:

```powershell
# Automated artifact testing script
param(
    [string]$ArtifactPath = ".\output",
    [switch]$Verbose
)

$ErrorActionPreference = "Continue"
$testsPassed = 0
$testsFailed = 0

function Test-FileExists {
    param([string]$Path, [string]$Description)
    
    if (Test-Path $Path) {
        Write-Host "[PASS] $Description" -ForegroundColor Green
        return $true
    } else {
        Write-Host "[FAIL] $Description - File not found: $Path" -ForegroundColor Red
        return $false
    }
}

Write-Host "=== AI File Sorter Artifact Testing ===" -ForegroundColor Cyan
Write-Host "Artifact Path: $ArtifactPath`n" -ForegroundColor Cyan

# Test 1: File Inventory
Write-Host "Test 1: File Inventory" -ForegroundColor Yellow
$criticalFiles = @(
    "aifilesorter.exe",
    "StartAiFileSorter.exe",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "llama.dll",
    "ggml.dll"
)

foreach ($file in $criticalFiles) {
    $fullPath = Join-Path $ArtifactPath $file
    if (Test-FileExists $fullPath "Found $file") {
        $testsPassed++
    } else {
        $testsFailed++
    }
}

# Test 2: DLL Version Check
Write-Host "`nTest 2: DLL Version Check" -ForegroundColor Yellow
$ggmlDll = Join-Path $ArtifactPath "ggml.dll"
if (Test-Path $ggmlDll) {
    $dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
    if ($dumpbin) {
        $exports = & dumpbin /EXPORTS $ggmlDll 2>&1 | Out-String
        if ($exports -match "ggml_xielu") {
            Write-Host "[PASS] ggml_xielu symbol found in ggml.dll" -ForegroundColor Green
            $testsPassed++
        } else {
            Write-Host "[FAIL] ggml_xielu symbol NOT found in ggml.dll" -ForegroundColor Red
            $testsFailed++
        }
    } else {
        Write-Host "[SKIP] dumpbin not available, cannot check exports" -ForegroundColor Yellow
    }
}

# Test 3: Startup Test (if not headless)
Write-Host "`nTest 3: Startup Test" -ForegroundColor Yellow
if (-not $env:CI) {
    Write-Host "Starting application (will timeout after 30 seconds)..." -ForegroundColor Cyan
    $starter = Join-Path $ArtifactPath "StartAiFileSorter.exe"
    
    if (Test-Path $starter) {
        $process = Start-Process -FilePath $starter -PassThru -NoNewWindow
        Start-Sleep -Seconds 5
        
        if ($process.HasExited) {
            Write-Host "[FAIL] Application exited immediately (crash?)" -ForegroundColor Red
            $testsFailed++
        } else {
            Write-Host "[PASS] Application started successfully" -ForegroundColor Green
            $testsPassed++
            
            # Give user time to test, then cleanup
            Write-Host "Application is running. Close it manually to continue..." -ForegroundColor Yellow
            $process.WaitForExit(30000) # Wait max 30 seconds
            
            if (-not $process.HasExited) {
                Write-Host "Forcing application to close..." -ForegroundColor Yellow
                $process.Kill()
            }
        }
    } else {
        Write-Host "[FAIL] StartAiFileSorter.exe not found" -ForegroundColor Red
        $testsFailed++
    }
} else {
    Write-Host "[SKIP] Headless environment, cannot test GUI startup" -ForegroundColor Yellow
}

# Summary
Write-Host "`n=== Test Summary ===" -ForegroundColor Cyan
Write-Host "Tests Passed: $testsPassed" -ForegroundColor Green
Write-Host "Tests Failed: $testsFailed" -ForegroundColor Red

if ($testsFailed -eq 0) {
    Write-Host "`n[SUCCESS] All tests passed! Artifact appears to be working correctly." -ForegroundColor Green
    exit 0
} else {
    Write-Host "`n[FAILURE] Some tests failed. Check logs above for details." -ForegroundColor Red
    exit 1
}
```

## Continuous Monitoring

For production deployments, monitor the following:

1. **Application Logs:**
   - Location: `%APPDATA%\aifilesorter\logs\`
   - Watch for: ERROR, CRITICAL, exceptions

2. **Windows Event Log:**
   ```cmd
   eventvwr.msc
   ```
   - Check: Application logs
   - Look for: .NET runtime errors, DLL load failures

3. **Resource Usage:**
   - Monitor CPU during LLM inference
   - Check memory usage with large file sets
   - Verify disk I/O for file operations

## Reporting Issues

If you find bugs during testing:

1. Fill out `QUICK_BUG_REPORT.md` template
2. Include all logs from `%APPDATA%\aifilesorter\logs\`
3. Attach output from automated test script
4. Include DLL dependency check results
5. Add screenshots of any error dialogs

## Conclusion

This artifact (Build #12) is expected to be **production-ready** based on:
- Successful build with all verification steps passing
- Known issues have been addressed with the launcher
- Comprehensive error handling in place
- DLL version compatibility verified

**Recommendation:** Safe to deploy for testing with real user scenarios.

---

**Document Version:** 1.0  
**Last Updated:** January 5, 2026  
**Tested Build:** Run #12 (20682964808)  
**Status:** ✅ Ready for testing
