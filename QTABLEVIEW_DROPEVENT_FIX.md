# Fix for QTableView::dropEvent DLL Error

## Problem Description

The error `?dropEvent@QTableview@@MEAAXPEAVQDropEvent@@@Z` (mangled name for QTableView::dropEvent) appears during startup on Windows. This is a Qt virtual function symbol that cannot be found, indicating a version mismatch between the Qt DLLs used at compile time and runtime.

## Root Cause Analysis

The issue occurs when:
1. Application is built with one version of Qt (e.g., Qt 6.5.3)
2. At runtime, Windows loads a different Qt version from system PATH (e.g., Qt 6.7.0)
3. Virtual function tables (vtables) don't match between versions
4. When QTableView is instantiated, the wrong dropEvent method is called
5. Application crashes with "entry point not found" error

## Previous Attempt

A previous fix (PR #58) added DLL version checking, but the check happened AFTER Qt was loaded:
```cpp
// Line 816: QApplication created - Qt DLLs load here
QApplication app(argc, argv);

// Line 903: Version check happens too late
check_dll_compatibility(ggmlPath, exeDir);
```

This approach couldn't prevent the error because Qt was already loaded with potentially wrong DLLs.

## Current Fix

The fix enhances DLL loading in `app/startapp_windows.cpp` with the following changes:

### 1. Enhanced DLL Directory Setup (Lines 767-795)

**Before:**
```cpp
if (secureSearchEnabled && !exeDirW.empty()) {
    if (AddDllDirectory(exeDirW.c_str()) == nullptr) {
        // Failed silently
    }
    // ...
}
```

**After:**
```cpp
if (secureSearchEnabled && !exeDirW.empty()) {
    if (AddDllDirectory(exeDirW.c_str()) == nullptr) {
        DWORD error = GetLastError();
        std::wstring errorMsg = L"Failed to add application directory to DLL search path (error ";
        errorMsg += std::to_wstring(error);
        errorMsg += L"). This may cause Qt version mismatch errors.";
        MessageBoxW(NULL, errorMsg.c_str(), L"DLL Setup Warning", MB_ICONWARNING | MB_OK);
    } else {
        dllPathSetupSuccessful = true;
    }
    // ... also check bin subdirectory
}
```

**Why this helps:**
- Errors are no longer silent - user knows if DLL setup fails
- `dllPathSetupSuccessful` flag tracks whether setup worked
- Shows specific Windows error code for debugging

### 2. PATH Fallback Always Enabled (Lines 797-847)

**Before:**
- PATH was only modified if `secureSearchEnabled` was false
- If `AddDllDirectory` failed, no fallback was attempted

**After:**
```cpp
// Always set PATH as a fallback, even if secure search is enabled
if (!exeDirW.empty()) {
    // Prepend application directory to PATH
    // This is CRITICAL for preventing Qt version mismatch errors
    // ...
    if (!SetEnvironmentVariableW(L"PATH", newPath.c_str())) {
        // Show error with specific error code
    } else {
        dllPathSetupSuccessful = true;
    }
}
```

**Why this helps:**
- Double protection: uses both `AddDllDirectory` AND PATH modification
- If one method fails, the other may still work
- Ensures application directory is FIRST in search order

### 3. Critical Error Warning (Lines 850-861)

```cpp
if (!dllPathSetupSuccessful && pathLen > 0) {
    MessageBoxW(NULL,
        L"Failed to configure DLL search paths properly.\n\n"
        L"This may cause \"entry point not found\" errors such as:\n"
        L"- QTableView::dropEvent not found\n"
        L"- Other Qt virtual function errors\n\n"
        L"The application may not start correctly.\n\n"
        L"Try running as Administrator or check your system PATH for conflicting Qt installations.",
        L"Critical DLL Setup Error",
        MB_ICONERROR | MB_OK);
}
```

**Why this helps:**
- User is immediately warned if DLL setup completely failed
- Clear explanation of what might go wrong
- Actionable advice (run as admin, check PATH)

### 4. Qt Plugin Path Configuration (Lines 863-878)

```cpp
// Set Qt plugin path to application directory to prevent loading plugins from wrong Qt version
// This must be done BEFORE creating QApplication
if (pathLen > 0 && pathLen < MAX_PATH * 2) {
    std::wstring pluginPath = exeDirW + L"\\plugins";
    SetEnvironmentVariableW(L"QT_PLUGIN_PATH", pluginPath.c_str());
    SetEnvironmentVariableW(L"QT_QPA_PLATFORM_PLUGIN_PATH", pluginPath.c_str());
}
```

**Why this helps:**
- Qt plugins must match Qt DLL version
- Setting paths before QApplication ensures correct plugins are loaded
- Prevents loading wrong platform plugins (which can also cause crashes)

### 5. Qt Version Logging (Lines 895-901)

```cpp
// Log the actual Qt version being used to help diagnose version mismatch issues
qInfo() << "Qt Runtime Version:" << qVersion() << "| Compiled with:" << QT_VERSION_STR;
if (QString::fromLatin1(qVersion()) != QString::fromLatin1(QT_VERSION_STR)) {
    qWarning() << "WARNING: Qt version mismatch detected! This may cause virtual function errors.";
    qWarning() << "Built with Qt" << QT_VERSION_STR << "but running with Qt" << qVersion();
}
```

**Why this helps:**
- Immediately shows if version mismatch occurred
- Logs appear in console/log files for debugging
- User can see exact versions being compared

## Testing on Windows

To verify the fix works:

### Test 1: Clean System (Should Work)
1. Fresh Windows system with no Qt installations
2. Run `StartAiFileSorter.exe`
3. Expected: No errors, application starts normally
4. Check logs: Qt versions should match

### Test 2: Conflicting Qt in PATH (Should Work Now)
1. Install a different Qt version (e.g., Qt 6.7.0) 
2. Add it to system PATH
3. Run `StartAiFileSorter.exe`
4. Expected: Warning dialogs appear if DLL setup struggles, but application should still work
5. Check logs: May show version mismatch warning but should use local DLLs

### Test 3: DLL Setup Failure (Should Show Clear Error)
1. Remove write permissions from application directory
2. Run `StartAiFileSorter.exe`
3. Expected: Critical error dialog appears explaining the problem
4. Error message should mention QTableView::dropEvent specifically

### Test 4: Verify DLL Loading Order
Use Process Monitor (procmon) from Sysinternals:
1. Run Process Monitor as Administrator
2. Filter for `StartAiFileSorter.exe` process
3. Look for `LoadImage` operations for Qt DLLs
4. Verify DLLs are loaded from application directory FIRST
5. No Qt DLLs should be loaded from system PATH

### Test 5: Direct Launch Warning
1. Run `aifilesorter.exe` directly (not via launcher)
2. Expected: Warning dialog appears (this is the old behavior, still works)
3. User can choose to continue or abort

## Verification Steps

After implementing the fix:

1. **Check Error Logs**: No more "dropEvent not found" errors
2. **Check Qt Version**: Runtime version matches compile-time version
3. **Check DLL Paths**: Application DLLs loaded before system DLLs
4. **Check Plugin Loading**: Qt plugins loaded from application directory
5. **Monitor Startup**: No crashes during QTableView instantiation

## Expected Outcomes

✅ **Success Indicators:**
- Application starts without DLL errors
- QTableView can be created without crashes
- Clear error messages if DLL setup fails
- Logs show matching Qt versions

❌ **Failure Indicators:**
- Still seeing "dropEvent not found" errors
- Silent failures during startup
- Qt version mismatch in logs
- Crashes when creating QTableView

## Rollback Plan

If this fix causes issues:

1. Revert commit 85094ac
2. The previous DLL checking still works (checks after Qt loads)
3. Users must manually fix their PATH environment variable

## Technical Notes

### Why AddDllDirectory + PATH?
- `AddDllDirectory` is the modern, secure way (Windows 7+)
- But it requires KB2533623 update on Windows 7
- PATH modification is the fallback for older systems
- Using both ensures maximum compatibility

### Why Set Plugin Paths?
- Qt loads plugins dynamically at runtime
- Wrong plugin version + right DLL version = still crashes
- Setting both ensures complete consistency

### Why Log Qt Version?
- Previous fix checked version but didn't log it
- Logging helps users report issues with exact versions
- Developers can diagnose version mismatches from logs

## Related Files

- `app/startapp_windows.cpp` - Main fix implementation
- `app/lib/DllVersionChecker.cpp` - Version checking (called after Qt loads)
- `app/main.cpp` - Warning dialog if launched directly
- `README.md` - Updated documentation

## Additional Resources

- [Qt Documentation: Deploying Qt Applications](https://doc.qt.io/qt-6/windows-deployment.html)
- [Microsoft: Dynamic-Link Library Search Order](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order)
- [Microsoft: AddDllDirectory function](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-adddlldirectory)
