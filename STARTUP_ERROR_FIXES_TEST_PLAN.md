# Startup Error Fixes - Test Plan

## Overview
This document outlines the testing strategy for the comprehensive startup error fixes implemented for both `StartAiFileSorter.exe` (Windows launcher) and `aifilesorter.exe` (main application) on Windows and Linux.

## Changes Summary

### 1. QTableView::dropEvent DLL Error Fix (Windows)
**Problem:** Qt DLLs loaded from system PATH before application directory DLLs
**Solution:** Enhanced DLL search path setup with multiple layers of protection

**Changes:**
- Added comprehensive error checking for `AddDllDirectory()` calls
- Set PATH environment variable as fallback (even when secure DLL search enabled)
- Set Qt plugin paths (`QT_PLUGIN_PATH`, `QT_QPA_PLATFORM_PLUGIN_PATH`) before Qt loads
- Added Qt version logging and comparison at startup
- Show detailed error dialogs if DLL path setup fails

### 2. StartAiFileSorter.exe Error Handling (Windows)
**Problem:** Silent failures when launching main executable
**Solution:** Comprehensive error logging and validation

**Changes:**
- `launchMainExecutable()`: Added detailed error logging, check `startDetached()` return value, log PID on success
- `resolveExecutableName()`: Return empty string on failure instead of invalid path
- Main function: Check if executable found before attempting launch, show error dialog if missing

### 3. aifilesorter.exe Error Handling (Windows & Linux)
**Problem:** Generic exception handling hides important startup errors
**Solution:** Differentiated exception handling with user-friendly error messages

**Changes:**
- Improved logger initialization error messages
- Added Windows error dialogs for initialization failures
- Differentiate exception types: `AppException`, `std::runtime_error`, `std::exception`, unknown
- Each exception type has specific error dialog with actionable information

### 4. startapp Linux Error Handling
**Problem:** Minimal error messages when launch fails
**Solution:** Detailed error messages with specific errno handling

**Changes:**
- `ensure_executable()`: Check file existence before checking execute permission
- `launch_with_env()`: Handle specific errno values (EACCES, ENOENT, ENOEXEC, ENOMEM)
- Provide actionable error messages (e.g., "Try: chmod +x <path>")

## Test Plan

### Windows Tests

#### Test 1: Normal Startup (Clean System)
**Objective:** Verify application starts normally on clean system

**Steps:**
1. Fresh Windows 10/11 system with no Qt installations
2. Install Visual C++ Redistributables
3. Extract application to `C:\Program Files\AIFileSorter`
4. Double-click `StartAiFileSorter.exe`

**Expected Result:**
- No error dialogs appear
- Application launches successfully
- Logs show: "Qt Runtime Version: 6.x.x | Compiled with: 6.x.x"
- Logs show: "Successfully launched main application process with PID: <number>"

**Pass Criteria:**
- Application starts without errors
- No Qt version mismatch warnings in logs
- QTableView components render correctly

---

#### Test 2: Conflicting Qt in PATH
**Objective:** Verify DLL protection works when wrong Qt version in system PATH

**Steps:**
1. Install different Qt version (e.g., Qt 6.7.0) via Qt Installer
2. Add Qt 6.7.0 bin directory to system PATH
3. Reboot to ensure PATH is loaded
4. Launch `StartAiFileSorter.exe`

**Expected Result:**
- Application launches successfully (uses local Qt DLLs)
- May show warning if AddDllDirectory fails but PATH fallback works
- Logs show correct Qt version loaded
- Logs may show: "Qt version mismatch detected!" but application continues

**Pass Criteria:**
- Application starts without crashes
- No "dropEvent not found" errors
- Application uses DLLs from its own directory (verify with Process Monitor)

---

#### Test 3: Missing Main Executable
**Objective:** Verify proper error handling when aifilesorter.exe is missing

**Steps:**
1. Rename or delete `aifilesorter.exe`
2. Launch `StartAiFileSorter.exe`

**Expected Result:**
- Error dialog appears: "Missing Executable"
- Dialog shows installation directory path
- Dialog advises to reinstall application
- Logs show: "Main executable not found in: <path>"
- Logs show searched file names

**Pass Criteria:**
- Clear, actionable error message
- User understands what is wrong and how to fix it
- Application exits gracefully (not crash)

---

#### Test 4: DLL Path Setup Failure
**Objective:** Verify error handling when DLL path setup completely fails

**Steps:**
1. Run as limited user with no PATH modification rights (if possible)
2. OR modify code to simulate AddDllDirectory failure
3. Launch `StartAiFileSorter.exe`

**Expected Result:**
- Warning dialog appears during startup
- Dialog explains: "Failed to configure DLL search paths properly"
- Dialog mentions potential "QTableView::dropEvent" errors
- Dialog suggests running as Administrator

**Pass Criteria:**
- User is warned about potential issues
- Error message is specific and actionable
- Application attempts to continue (may fail later with Qt errors)

---

#### Test 5: Direct Launch Warning
**Objective:** Verify warning when launching aifilesorter.exe directly

**Steps:**
1. Double-click `aifilesorter.exe` directly (not via StartAiFileSorter.exe)

**Expected Result:**
- Warning dialog appears immediately
- Dialog explains to use StartAiFileSorter.exe instead
- Dialog lists potential errors (ggml_xielu, QTableView::dropEvent)
- User can choose "Continue anyway" or cancel

**Pass Criteria:**
- Warning is clear and actionable
- User understands the risk
- Application allows informed decision

---

#### Test 6: Logger Initialization Failure
**Objective:** Verify error handling when logger cannot initialize

**Steps:**
1. Remove write permissions from application directory
2. OR fill disk to 100%
3. Launch application

**Expected Result:**
- Error dialog appears: "Failed to initialize logging system"
- Dialog lists possible causes (disk space, permissions, logs directory)
- Console shows error details
- Application exits gracefully

**Pass Criteria:**
- Clear error message explaining the problem
- Actionable suggestions for fixing
- No crash or hang

---

#### Test 7: Process Launch Failure
**Objective:** Verify error when startDetached() fails

**Steps:**
1. Modify aifilesorter.exe to make it non-executable (e.g., corrupt it)
2. Launch StartAiFileSorter.exe

**Expected Result:**
- Error dialog: "Launch Failed"
- Logs show: "Failed to start detached process"
- Logs show: "Process error: <detailed Qt error>"
- Logs list possible causes (missing DLLs, permissions, antivirus, corrupted file)

**Pass Criteria:**
- Detailed error information in logs
- User-friendly error dialog
- Clear indication of what went wrong

---

### Linux Tests

#### Test 8: Normal Startup (Linux)
**Objective:** Verify application starts normally on Linux

**Steps:**
1. Fresh Ubuntu 22.04 or similar
2. Install dependencies (Qt6, libraries)
3. Extract application to `/opt/aifilesorter`
4. Run `./StartAiFileSorter`

**Expected Result:**
- Console shows: "Using <backend> backend"
- Console shows: "Launching main application: <path>"
- Application starts successfully

**Pass Criteria:**
- No errors in console
- Application launches and runs correctly
- Proper backend selection (CUDA/Vulkan/CPU)

---

#### Test 9: Missing Executable (Linux)
**Objective:** Verify error when main executable missing

**Steps:**
1. Delete or rename `bin/aifilesorter`
2. Run `./StartAiFileSorter`

**Expected Result:**
- Console shows: "CRITICAL ERROR: Main executable not found: <path>"
- Console shows: "The application cannot start without the main executable file"
- Console shows: "Please verify the installation is complete"
- Exits with error code

**Pass Criteria:**
- Clear error message
- Explains what is missing
- Suggests solution

---

#### Test 10: Non-Executable File (Linux)
**Objective:** Verify error when executable permissions missing

**Steps:**
1. Remove execute permission: `chmod -x bin/aifilesorter`
2. Run `./StartAiFileSorter`

**Expected Result:**
- Console shows: "CRITICAL ERROR: Main executable is not executable"
- Console shows: "Permission denied"
- Console shows: "Try running: chmod +x <path>"
- Exits with error code

**Pass Criteria:**
- Identifies permission problem
- Provides exact command to fix
- Actionable error message

---

#### Test 11: execve Failure (Linux)
**Objective:** Verify detailed error handling for launch failures

**Steps:**
1. Create a corrupted/invalid executable file
2. OR exhaust system memory
3. Run `./StartAiFileSorter`

**Expected Result:**
- Console shows: "CRITICAL ERROR: Failed to launch main application"
- Console shows specific errno (EACCES, ENOENT, ENOEXEC, ENOMEM)
- Console provides context-specific error message
- Console suggests fix based on error type

**Pass Criteria:**
- Error type correctly identified
- Specific actionable advice given
- User can understand and fix the problem

---

### Verification Tests

#### Test 12: Qt Version Mismatch Detection
**Objective:** Verify Qt version logging works correctly

**Steps:**
1. Launch application normally
2. Check logs immediately after startup

**Expected Result:**
- Logs contain: "Qt Runtime Version: X.Y.Z | Compiled with: A.B.C"
- If versions differ: "WARNING: Qt version mismatch detected!"
- If versions match: No warning

**Pass Criteria:**
- Version comparison is accurate
- Mismatch is logged clearly
- User can identify version issues from logs

---

#### Test 13: DLL Search Path Logging
**Objective:** Verify DLL path setup is logged

**Steps:**
1. Launch StartAiFileSorter.exe
2. Check console output or logs

**Expected Result:**
- Logs show: "Secure DLL search enabled" OR "SetDefaultDllDirectories unavailable"
- If setup fails: Detailed error with error code
- If setup succeeds: Confirmation message

**Pass Criteria:**
- DLL path setup status is clear
- Errors are reported with details
- Success is confirmed

---

#### Test 14: Process Monitor Verification (Windows)
**Objective:** Verify DLLs are loaded from correct location

**Steps:**
1. Install and run Process Monitor (Sysinternals)
2. Filter for StartAiFileSorter.exe and aifilesorter.exe processes
3. Filter for "LoadImage" operations
4. Launch application
5. Examine DLL load locations

**Expected Result:**
- Qt DLLs (Qt6Core.dll, Qt6Widgets.dll, etc.) loaded from application directory
- No Qt DLLs loaded from system PATH
- Application directory is searched first

**Pass Criteria:**
- Application DLLs have priority over system DLLs
- No wrong-version Qt DLLs are loaded
- DLL search order is correct

---

#### Test 15: Exception Handling Differentiation
**Objective:** Verify different exception types are handled correctly

**Steps:**
1. Trigger different exception types (requires code modification or specific conditions):
   - AppException: Trigger application-specific error
   - runtime_error: Cause file I/O error
   - std::exception: Cause generic error
   - Unknown: Cause non-exception error

**Expected Result:**
- Each exception type shows appropriate error dialog (Windows)
- Each exception type logs appropriate message
- Error dialogs match exception type

**Pass Criteria:**
- Exception types are correctly differentiated
- Error messages are appropriate for each type
- User gets relevant information for each error

---

## Regression Tests

#### Test 16: Normal Operation Not Affected
**Objective:** Verify fixes don't break normal operation

**Steps:**
1. Complete full application workflow:
   - Launch application
   - Configure settings
   - Scan files
   - Categorize files
   - Move files
2. Check for any new errors or warnings

**Expected Result:**
- All features work as before
- No new errors introduced
- No performance degradation

**Pass Criteria:**
- Application functionality unchanged
- No regressions in existing features

---

## Performance Tests

#### Test 17: Startup Time
**Objective:** Verify fixes don't significantly impact startup time

**Steps:**
1. Measure time from launching StartAiFileSorter.exe to main window appears
2. Compare with baseline (if available)
3. Repeat 10 times and average

**Expected Result:**
- Startup time increase < 500ms
- No noticeable delay to user

**Pass Criteria:**
- Startup performance acceptable
- DLL checks don't cause significant delay

---

## Test Environment Requirements

### Windows
- Windows 10 20H2 or later / Windows 11
- Visual C++ Redistributable 2019 or later
- 4GB RAM minimum
- Administrator rights for some tests
- Process Monitor (Sysinternals) for verification tests

### Linux
- Ubuntu 22.04 LTS or equivalent
- Qt 6.5.3 or later
- 4GB RAM minimum
- Standard development tools (gcc, make, etc.)

### Tools
- Process Monitor (Windows)
- Procmon equivalent for Linux (strace, lsof)
- Log viewer
- Text editor for log analysis

## Success Criteria

### Critical (Must Pass)
- [ ] Test 1: Normal startup works
- [ ] Test 2: Conflicting Qt doesn't cause crashes
- [ ] Test 3: Missing executable shows clear error
- [ ] Test 8: Linux normal startup works
- [ ] Test 16: No regressions in existing features

### Important (Should Pass)
- [ ] Test 4: DLL setup failure is handled gracefully
- [ ] Test 5: Direct launch warning appears
- [ ] Test 6: Logger failure is handled
- [ ] Test 9: Linux missing executable error
- [ ] Test 10: Linux permission error
- [ ] Test 12: Qt version mismatch detection

### Nice to Have (Can Pass)
- [ ] Test 7: Process launch failure details
- [ ] Test 11: Linux execve error details
- [ ] Test 13: DLL path logging
- [ ] Test 14: Process Monitor verification
- [ ] Test 15: Exception differentiation
- [ ] Test 17: Startup time acceptable

## Known Limitations

1. **Windows 7**: AddDllDirectory requires KB2533623 update. PATH fallback handles this.
2. **Admin Rights**: Some DLL path modifications may require admin rights on restricted systems.
3. **Antivirus**: May interfere with process launching. Cannot be fully tested.
4. **Corrupted Executables**: Hard to test without breaking the installation.

## Test Execution Notes

- Tests should be run on clean VMs when possible
- Document system configuration for each test
- Capture logs for all tests (success and failure)
- Take screenshots of error dialogs
- Note any unexpected behavior even if test passes

## Sign-off

| Test Category | Tester | Date | Result | Notes |
|--------------|--------|------|--------|-------|
| Windows Startup | | | | |
| Linux Startup | | | | |
| Error Handling | | | | |
| Verification | | | | |
| Regression | | | | |
| Performance | | | | |

