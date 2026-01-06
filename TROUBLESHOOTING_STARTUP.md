# Troubleshooting Executable Startup Issues

This guide helps you diagnose and fix issues when `aifilesorter.exe` or `StartAiFileSorter.exe` won't run.

## Quick Fixes (Try These First!)

### 1. Always Use StartAiFileSorter.exe
**Problem:** Running `aifilesorter.exe` directly can cause DLL errors.  
**Solution:** Always launch via `StartAiFileSorter.exe`

### 2. Run as Administrator
**Problem:** DLL path configuration fails without admin rights.  
**Solution:** Right-click `StartAiFileSorter.exe` → "Run as Administrator"

### 3. Check for Multiple Qt Installations
**Problem:** System has conflicting Qt versions in PATH.  
**Solution:** 
1. Open Command Prompt
2. Run: `echo %PATH%`
3. Look for Qt directories (e.g., `C:\Qt\6.7.0\bin`)
4. Remove them from system PATH if you find any

### 4. Missing DLLs
**Problem:** Installation incomplete or DLLs not built.  
**Solution:** 
- Reinstall the application
- If building from source, run: `app\scripts\build_llama_windows.ps1`
- Check that `lib\ggml\wocuda` directory exists with DLLs

## Diagnostic Tool

Run the diagnostic tool to identify issues:
```bash
# In the application directory
diagnose_startup.exe
```

This will create `startup_diagnostic.txt` with detailed information about:
- Missing executables
- Missing DLLs
- Qt version conflicts
- Path issues
- Permission problems

## Common Errors and Solutions

### "QTableView::dropEvent not found"
**Cause:** Qt version mismatch - wrong Qt DLLs loaded from system PATH.  
**Fix:**
1. Run as Administrator
2. Remove other Qt installations from PATH
3. Use StartAiFileSorter.exe (not aifilesorter.exe directly)

### "ggml_xielu entry point not found"
**Cause:** Outdated GGML DLLs that don't match the application version.  
**Fix:**
1. Rebuild llama.cpp libraries: `app\scripts\build_llama_windows.ps1`
2. Or download latest prebuilt binaries
3. Ensure all files in `lib\ggml\wocuda` are up to date

### "Could not find main executable"
**Cause:** `aifilesorter.exe` missing or in wrong location.  
**Fix:**
1. Check that `aifilesorter.exe` exists in application directory
2. Reinstall if missing
3. Check antivirus hasn't quarantined it

### "Failed to configure DLL search paths"
**Cause:** Insufficient permissions or PATH issues.  
**Fix:**
1. Run as Administrator
2. Install to a simpler path (e.g., `C:\AIFileSorter` instead of `C:\Program Files\...`)
3. Avoid paths with spaces or special characters

### Application starts but crashes immediately
**Cause:** DLL version incompatibility despite passing initial checks.  
**Fix:**
1. Check `startup_log.txt` in application directory
2. Look for Qt version mismatch messages
3. Rebuild all DLLs from source to ensure compatibility
4. Try running in compatibility mode (Windows 8)

## Startup Logs

The application creates several log files to help diagnose issues:

### startup_log.txt
Created by `StartAiFileSorter.exe` - shows initialization steps.
```
[2026-01-06 12:34:56] === StartAiFileSorter.exe started ===
[2026-01-06 12:34:56] Enabling DPI awareness...
[2026-01-06 12:34:56] Setting up DLL search paths...
[2026-01-06 12:34:56] Secure DLL search enabled
```

### logs/errors.log
Detailed error information from ErrorReporter system.

### logs/COPILOT_ERROR_*.md
User-friendly error reports for GitHub Copilot users.

## Step-by-Step Debugging

### Step 1: Verify Installation
```cmd
cd C:\Path\To\AIFileSorter
dir aifilesorter.exe
dir StartAiFileSorter.exe
dir lib\ggml\wocuda\*.dll
```

All files should exist. If not, reinstall.

### Step 2: Check Qt DLLs
```cmd
dir Qt6*.dll
```

Should see: `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`

### Step 3: Run Diagnostic
```cmd
diagnose_startup.exe > diagnosis.txt 2>&1
type diagnosis.txt
```

Review output for specific issues.

### Step 4: Try Administrator Mode
```cmd
# Right-click StartAiFileSorter.exe
# Select "Run as Administrator"
```

### Step 5: Check PATH
```cmd
echo %PATH% | findstr /i "Qt"
```

If this finds Qt installations, you have a conflict.

### Step 6: Review Logs
```cmd
type startup_log.txt
type logs\errors.log
```

Look for ERROR or CRITICAL messages.

## Building from Source

If prebuilt executables don't work, building from source may help:

### Prerequisites
- Visual Studio 2022 (with C++ Desktop Development)
- CMake 3.21+
- vcpkg (for dependencies)
- Qt 6.5+

### Build Steps
```powershell
# 1. Build GGML/llama.cpp
cd app\scripts
.\build_llama_windows.ps1 -vcpkgroot C:\vcpkg

# 2. Build main application
cd ..\..
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --config Release

# 3. Copy dependencies
# Qt DLLs, GGML DLLs, etc. must be copied to build output
```

## Getting Help

### For GitHub Copilot Users
1. Look for `logs\COPILOT_ERROR_*.md` files
2. Copy the entire file contents
3. Paste into GitHub Copilot Chat
4. Copilot will provide step-by-step troubleshooting

### For Manual Troubleshooting
1. Run `diagnose_startup.exe`
2. Copy output from `startup_diagnostic.txt`
3. Include contents of `startup_log.txt`
4. Check `logs\errors.log` for detailed errors
5. Create an issue with all this information

### Information to Include in Bug Reports
- Windows version (e.g., Windows 11 23H2)
- Installation path
- Contents of `startup_diagnostic.txt`
- Last 50 lines of `startup_log.txt`
- Any error dialogs (screenshot)
- What you tried from this guide

## Prevention Tips

### For Clean Installations
1. Install to `C:\AIFileSorter` (avoid Program Files)
2. No spaces or special characters in path
3. Don't install other Qt applications system-wide
4. Keep Windows up to date
5. Use Administrator account for first run

### For Development Builds
1. Always rebuild GGML when updating llama.cpp
2. Match Qt version exactly (don't mix 6.5 and 6.7)
3. Use vcpkg for consistent dependencies
4. Test both aifilesorter.exe and StartAiFileSorter.exe
5. Check startup_log.txt after every build

## Still Not Working?

If you've tried everything above:

1. **Create a minimal test case:**
   ```cmd
   # Try running Qt test app
   Qt6Widgets.dll  # Should not crash
   ```

2. **Check system integrity:**
   ```cmd
   sfc /scannow
   ```

3. **Try a clean Windows boot:**
   - Restart in Safe Mode with Networking
   - Try running the app
   - If it works, a background app is interfering

4. **Last resort - Clean reinstall:**
   ```cmd
   # Uninstall completely
   # Delete installation directory
   # Restart computer
   # Reinstall as Administrator
   # Install to C:\AIFileSorter
   ```

## Technical Details

### Startup Sequence
1. `StartAiFileSorter.exe` starts
2. DPI awareness enabled
3. Secure DLL search configured
4. Application directory added to DLL search path
5. Qt plugin paths set
6. QApplication created
7. Qt version checked
8. GGML backend selected (CPU/CUDA/Vulkan)
9. GGML directory resolved
10. DLL compatibility checked
11. `aifilesorter.exe` launched with environment

### Critical Dependencies
- **Qt 6.5+**: GUI framework
- **GGML/llama.cpp**: Local LLM backend
- **CURL**: Network operations
- **OpenSSL**: API encryption
- **SQLite**: Database operations
- **JsonCpp**: Configuration parsing

### DLL Search Order (Windows)
1. Application directory (if SetDefaultDllDirectories used)
2. System32
3. Windows directory
4. Current directory
5. PATH directories

StartAiFileSorter.exe manipulates this to ensure application DLLs load first.

---

**Last Updated:** 2026-01-06  
**Applies to:** AI File Sorter v1.5.0+
