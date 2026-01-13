#!/usr/bin/env python3
"""
AI File Sorter - Comprehensive Diagnostic Tool

This tool performs a comprehensive health check of the AI File Sorter installation,
validates all features, and generates a detailed diagnostic report.

Usage:
    python3 diagnostic_tool.py [--verbose] [--output FILE]
"""

import os
import sys
import json
import platform
import subprocess
import sqlite3
import datetime
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# ANSI color codes for terminal output
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class DiagnosticResult:
    """Stores the result of a diagnostic check"""
    def __init__(self, name: str, status: str, message: str, details: Optional[str] = None):
        self.name = name
        self.status = status  # OK, WARNING, FAIL, INFO
        self.message = message
        self.details = details
        self.timestamp = datetime.datetime.now().isoformat()

class DiagnosticTool:
    """Main diagnostic tool class"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.results: List[DiagnosticResult] = []
        self.platform = platform.system()
        self.start_time = datetime.datetime.now()
        
        # Determine base directory
        if self.platform == "Windows":
            self.app_dir = Path("app/build-windows/Release")
            if not self.app_dir.exists():
                self.app_dir = Path(".")
        else:
            self.app_dir = Path("app/bin")
            if not self.app_dir.exists():
                self.app_dir = Path(".")
    
    def log(self, message: str, color: str = ""):
        """Print a log message"""
        if color:
            print(f"{color}{message}{Colors.ENDC}")
        else:
            print(message)
    
    def add_result(self, name: str, status: str, message: str, details: Optional[str] = None):
        """Add a diagnostic result"""
        result = DiagnosticResult(name, status, message, details)
        self.results.append(result)
        
        # Print result
        status_color = {
            "OK": Colors.OKGREEN,
            "WARNING": Colors.WARNING,
            "FAIL": Colors.FAIL,
            "INFO": Colors.OKBLUE
        }.get(status, "")
        
        status_symbol = {
            "OK": "✓",
            "WARNING": "⚠",
            "FAIL": "✗",
            "INFO": "ℹ"
        }.get(status, "•")
        
        self.log(f"  {status_symbol} {name}: {message}", status_color)
        
        if self.verbose and details:
            self.log(f"    Details: {details}", Colors.OKCYAN)
    
    def section_header(self, title: str):
        """Print a section header"""
        self.log(f"\n{'='*80}", Colors.HEADER)
        self.log(f"{title.upper()}", Colors.HEADER + Colors.BOLD)
        self.log(f"{'='*80}", Colors.HEADER)
    
    # ==================== System Information ====================
    
    def check_system_info(self):
        """Check basic system information"""
        self.section_header("System Information")
        
        # Platform
        self.add_result(
            "Platform",
            "INFO",
            f"{platform.system()} {platform.release()}",
            f"{platform.platform()}"
        )
        
        # Architecture
        self.add_result(
            "Architecture",
            "INFO",
            platform.machine(),
            f"Processor: {platform.processor()}"
        )
        
        # Python version
        self.add_result(
            "Python Version",
            "INFO",
            f"{sys.version.split()[0]}",
            sys.version
        )
    
    # ==================== File System Structure ====================
    
    def check_file_structure(self):
        """Check if required files and directories exist"""
        self.section_header("File System Structure")
        
        # Required executables
        if self.platform == "Windows":
            executables = [
                "StartAiFileSorter.exe",
                "aifilesorter.exe"
            ]
        else:
            executables = [
                "app/bin/aifilesorter",
                "app/bin/run_aifilesorter.sh"
            ]
        
        for exe in executables:
            try:
                exe_path = Path(exe)
                if exe_path.exists():
                    self.add_result(
                        f"Executable: {exe}",
                        "OK",
                        "Found",
                        f"Path: {exe_path.absolute()}"
                    )
                else:
                    self.add_result(
                        f"Executable: {exe}",
                        "FAIL",
                        "Not found",
                        f"Expected at: {exe_path.absolute()}"
                    )
            except Exception as e:
                self.add_result(
                    f"Executable: {exe}",
                    "FAIL",
                    f"Error checking executable: {str(e)}",
                    None
                )
        
        # Required directories
        required_dirs = [
            "app/include",
            "app/lib",
            "app/resources",
        ]
        
        for dir_path in required_dirs:
            try:
                dir_path = Path(dir_path)
                if dir_path.exists() and dir_path.is_dir():
                    file_count = sum(1 for _ in dir_path.rglob('*') if _.is_file())
                    self.add_result(
                        f"Directory: {dir_path}",
                        "OK",
                        f"Found ({file_count} files)",
                        f"Path: {dir_path.absolute()}"
                    )
                else:
                    self.add_result(
                        f"Directory: {dir_path}",
                        "WARNING",
                        "Not found",
                        f"Expected at: {dir_path.absolute()}"
                    )
            except Exception as e:
                self.add_result(
                    f"Directory: {dir_path}",
                    "FAIL",
                    f"Error checking directory: {str(e)}",
                    None
                )
    
    # ==================== Dependencies ====================
    
    def check_dependencies(self):
        """Check if required dependencies are available"""
        self.section_header("Dependencies")
        
        # Qt libraries (platform-specific)
        if self.platform == "Windows":
            qt_libs = ["Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll"]
            lib_dir = self.app_dir
        else:
            # On Linux/macOS, Qt is typically system-wide
            self.add_result(
                "Qt6",
                "INFO",
                "System-wide installation (checking via pkg-config)",
                None
            )
            
            try:
                result = subprocess.run(
                    ["pkg-config", "--modversion", "Qt6Widgets"],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0:
                    version = result.stdout.strip()
                    self.add_result(
                        "Qt6Widgets",
                        "OK",
                        f"Found version {version}",
                        None
                    )
                else:
                    self.add_result(
                        "Qt6Widgets",
                        "WARNING",
                        "Not found via pkg-config",
                        "May still be available via system paths"
                    )
            except subprocess.TimeoutExpired:
                self.add_result(
                    "Qt6",
                    "WARNING",
                    "pkg-config timeout",
                    "Command took too long to execute"
                )
            except FileNotFoundError:
                self.add_result(
                    "Qt6",
                    "WARNING",
                    "pkg-config not available",
                    "Cannot verify Qt installation"
                )
            except Exception as e:
                self.add_result(
                    "Qt6",
                    "FAIL",
                    f"Error checking Qt: {str(e)}",
                    None
                )
            return
        
        # Check Windows Qt DLLs
        for lib in qt_libs:
            try:
                lib_path = lib_dir / lib
                if lib_path.exists():
                    size = lib_path.stat().st_size
                    size_kb = max(1, size // 1024)  # At least 1 KB for display
                    self.add_result(
                        f"Library: {lib}",
                        "OK",
                        f"Found ({size_kb} KB)",
                        f"Path: {lib_path}"
                    )
                else:
                    self.add_result(
                        f"Library: {lib}",
                        "FAIL",
                        "Not found",
                        f"Expected at: {lib_path}"
                    )
            except Exception as e:
                self.add_result(
                    f"Library: {lib}",
                    "FAIL",
                    f"Error checking library: {str(e)}",
                    None
                )
    
    # ==================== LLM Backends ====================
    
    def check_llm_backends(self):
        """Check available LLM inference backends"""
        self.section_header("LLM Backends")
        
        # Check for GGML directories
        ggml_variants = ["wocuda", "wcuda", "wvulkan"]
        
        for variant in ggml_variants:
            if self.platform == "Windows":
                ggml_path = Path(f"app/lib/ggml/{variant}")
            else:
                ggml_path = Path(f"app/lib/ggml/{variant}")
            
            if ggml_path.exists():
                dll_count = sum(1 for f in ggml_path.glob("*.dll" if self.platform == "Windows" else "*.so"))
                backend_type = {
                    "wocuda": "CPU (OpenBLAS)",
                    "wcuda": "CUDA (NVIDIA GPU)",
                    "wvulkan": "Vulkan (Cross-platform GPU)"
                }.get(variant, variant)
                
                self.add_result(
                    f"Backend: {backend_type}",
                    "OK",
                    f"Available ({dll_count} libraries)",
                    f"Path: {ggml_path.absolute()}"
                )
            else:
                optional = variant in ["wcuda", "wvulkan"]
                self.add_result(
                    f"Backend: {variant}",
                    "INFO" if optional else "WARNING",
                    "Not found (optional)" if optional else "Not found",
                    f"Expected at: {ggml_path.absolute()}"
                )
        
        # Check for precompiled llama libraries
        precompiled_dir = Path("app/lib/precompiled")
        if precompiled_dir.exists():
            variants = [d.name for d in precompiled_dir.iterdir() if d.is_dir()]
            self.add_result(
                "Precompiled Libraries",
                "OK",
                f"Found {len(variants)} variant(s): {', '.join(variants)}",
                f"Path: {precompiled_dir.absolute()}"
            )
        else:
            self.add_result(
                "Precompiled Libraries",
                "WARNING",
                "Directory not found",
                f"Expected at: {precompiled_dir.absolute()}"
            )
    
    # ==================== Database ====================
    
    def check_database(self):
        """Check database connectivity and structure"""
        self.section_header("Database")
        
        # Find database file
        if self.platform == "Windows":
            db_paths = [
                Path(os.path.expandvars("%APPDATA%/aifilesorter/aifilesorter.db")),
                Path("aifilesorter.db"),
            ]
        else:
            db_paths = [
                Path.home() / ".local/share/aifilesorter/aifilesorter.db",
                Path("aifilesorter.db"),
            ]
        
        db_path = None
        for path in db_paths:
            try:
                if path.exists():
                    db_path = path
                    break
            except Exception as e:
                self.add_result(
                    f"Database Path Check: {path}",
                    "WARNING",
                    f"Error checking path: {str(e)}",
                    None
                )
        
        if not db_path:
            self.add_result(
                "Database File",
                "INFO",
                "Not found (will be created on first run)",
                f"Expected locations: {', '.join(str(p) for p in db_paths)}"
            )
            return
        
        try:
            file_size = db_path.stat().st_size
            self.add_result(
                "Database File",
                "OK",
                f"Found ({file_size // 1024} KB)",
                f"Path: {db_path}"
            )
        except Exception as e:
            self.add_result(
                "Database File",
                "FAIL",
                f"Error reading database file: {str(e)}",
                f"Path: {db_path}"
            )
            return
        
        # Check database integrity
        try:
            conn = sqlite3.connect(db_path)
            cursor = conn.cursor()
            
            # Check tables
            try:
                cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
                tables = [row[0] for row in cursor.fetchall()]
                
                expected_tables = [
                    "categorization_cache",
                    "taxonomy",
                    "confidence_scores",
                    "content_analysis_cache",
                    "api_usage_tracking",
                    "user_profiles",
                    "user_corrections",
                    "categorization_sessions",
                    "undo_history",
                    "file_tinder_state"
                ]
                
                found_tables = [t for t in expected_tables if t in tables]
                missing_tables = [t for t in expected_tables if t not in tables]
                
                if len(found_tables) == len(expected_tables):
                    self.add_result(
                        "Database Tables",
                        "OK",
                        f"All {len(expected_tables)} tables found",
                        f"Tables: {', '.join(found_tables)}"
                    )
                else:
                    self.add_result(
                        "Database Tables",
                        "WARNING",
                        f"Found {len(found_tables)}/{len(expected_tables)} tables",
                        f"Missing: {', '.join(missing_tables)}"
                    )
            except sqlite3.Error as e:
                self.add_result(
                    "Database Tables",
                    "FAIL",
                    f"Error querying tables: {str(e)}",
                    None
                )
            
            # Check cache statistics
            try:
                cursor.execute("SELECT COUNT(*) FROM categorization_cache")
                cache_count = cursor.fetchone()[0]
                
                self.add_result(
                    "Cache Entries",
                    "INFO",
                    f"{cache_count} cached categorizations",
                    None
                )
            except sqlite3.Error as e:
                self.add_result(
                    "Cache Entries",
                    "WARNING",
                    f"Could not query cache: {str(e)}",
                    "Cache table may not exist yet"
                )
            
            # Check API usage tracking
            try:
                cursor.execute("SELECT COUNT(*) FROM api_usage_tracking")
                api_usage_count = cursor.fetchone()[0]
                
                self.add_result(
                    "API Usage Records",
                    "INFO",
                    f"{api_usage_count} API calls tracked",
                    None
                )
            except sqlite3.Error as e:
                self.add_result(
                    "API Usage Records",
                    "WARNING",
                    f"Could not query API usage: {str(e)}",
                    "API usage table may not exist yet"
                )
            
            conn.close()
            
        except sqlite3.Error as e:
            self.add_result(
                "Database Connection",
                "FAIL",
                f"Database error: {str(e)}",
                f"Could not connect to or query database at {db_path}"
            )
        except Exception as e:
            self.add_result(
                "Database Integrity",
                "FAIL",
                f"Unexpected error: {str(e)}",
                "Error during database checks"
            )
    
    # ==================== Configuration ====================
    
    def check_configuration(self):
        """Check configuration files"""
        self.section_header("Configuration")
        
        # Find config file
        if self.platform == "Windows":
            config_paths = [
                Path(os.path.expandvars("%APPDATA%/aifilesorter/config.ini")),
            ]
        else:
            config_paths = [
                Path.home() / ".config/aifilesorter/config.ini",
            ]
        
        config_path = None
        for path in config_paths:
            if path.exists():
                config_path = path
                break
        
        if not config_path:
            self.add_result(
                "Configuration File",
                "INFO",
                "Not found (will be created on first run)",
                f"Expected locations: {', '.join(str(p) for p in config_paths)}"
            )
            return
        
        self.add_result(
            "Configuration File",
            "OK",
            "Found",
            f"Path: {config_path}"
        )
        
        # Try to read config
        try:
            with open(config_path, 'r') as f:
                config_content = f.read()
                line_count = len(config_content.splitlines())
                
                self.add_result(
                    "Config Content",
                    "OK",
                    f"{line_count} lines",
                    None
                )
                
                # Check for API keys (without revealing them)
                has_openai = "openai" in config_content.lower()
                has_gemini = "gemini" in config_content.lower()
                
                if has_openai or has_gemini:
                    apis = []
                    if has_openai:
                        apis.append("OpenAI")
                    if has_gemini:
                        apis.append("Gemini")
                    
                    self.add_result(
                        "API Keys",
                        "INFO",
                        f"Configured: {', '.join(apis)}",
                        None
                    )
                else:
                    self.add_result(
                        "API Keys",
                        "INFO",
                        "No API keys configured (local LLM only)",
                        None
                    )
        
        except Exception as e:
            self.add_result(
                "Config Read",
                "WARNING",
                "Could not read config file",
                str(e)
            )
    
    # ==================== Logs ====================
    
    def check_logs(self):
        """Check log files"""
        self.section_header("Log Files")
        
        # Find log directory
        if self.platform == "Windows":
            log_dirs = [
                Path(os.path.expandvars("%APPDATA%/aifilesorter/logs")),
                Path("logs"),
            ]
        else:
            log_dirs = [
                Path.home() / ".local/share/aifilesorter/logs",
                Path("logs"),
            ]
        
        log_dir = None
        for path in log_dirs:
            if path.exists():
                log_dir = path
                break
        
        if not log_dir:
            self.add_result(
                "Log Directory",
                "INFO",
                "Not found (will be created on first run)",
                f"Expected locations: {', '.join(str(p) for p in log_dirs)}"
            )
            return
        
        # Count log files
        log_files = list(log_dir.glob("*.log")) + list(log_dir.glob("*.txt"))
        
        self.add_result(
            "Log Directory",
            "OK",
            f"Found ({len(log_files)} log files)",
            f"Path: {log_dir}"
        )
        
        # Check for error logs
        error_logs = [f for f in log_files if "error" in f.name.lower()]
        if error_logs:
            self.add_result(
                "Error Logs",
                "WARNING",
                f"{len(error_logs)} error log(s) found",
                f"Files: {', '.join(f.name for f in error_logs)}"
            )
            
            # Check the most recent error log
            if error_logs:
                latest_error = max(error_logs, key=lambda f: f.stat().st_mtime)
                try:
                    with open(latest_error, 'r') as f:
                        lines = f.readlines()
                        if lines:
                            last_lines = lines[-5:]  # Last 5 lines
                            self.add_result(
                                "Recent Errors",
                                "INFO",
                                f"Last {len(last_lines)} lines from {latest_error.name}",
                                '\n'.join(line.strip() for line in last_lines)
                            )
                except Exception as e:
                    self.add_result(
                        "Error Log Read",
                        "WARNING",
                        "Could not read error log",
                        str(e)
                    )
    
    # ==================== Feature Tests ====================
    
    def check_features(self):
        """Test key features (where possible without running the app)"""
        self.section_header("Feature Validation")
        
        # Check for feature-related files
        features = [
            ("File Tinder", "app/lib/FileTinderDialog.cpp"),
            ("Cache Manager", "app/lib/CacheManagerDialog.cpp"),
            ("User Profile", "app/lib/UserProfileManager.cpp"),
            ("Dry Run", "app/lib/DryRunPreviewDialog.cpp"),
            ("Undo Manager", "app/lib/UndoManager.cpp"),
            ("Gemini Client", "app/lib/GeminiClient.cpp"),
            ("Local LLM", "app/lib/LocalLLMClient.cpp"),
        ]
        
        for feature_name, file_path in features:
            file_path = Path(file_path)
            if file_path.exists():
                self.add_result(
                    f"Feature: {feature_name}",
                    "OK",
                    "Implementation found",
                    f"Source: {file_path}"
                )
            else:
                self.add_result(
                    f"Feature: {feature_name}",
                    "WARNING",
                    "Source file not found",
                    f"Expected: {file_path}"
                )
    
    # ==================== Performance Tests ====================
    
    def check_performance(self):
        """Run basic performance checks"""
        self.section_header("Performance Metrics")
        
        # Check available disk space
        try:
            if self.platform == "Windows":
                import shutil
                total, used, free = shutil.disk_usage("/")
            else:
                import shutil
                total, used, free = shutil.disk_usage(str(Path.home()))
            
            free_gb = free / (1024**3)
            total_gb = total / (1024**3)
            usage_percent = (used / total) * 100
            
            status = "OK" if free_gb > 5 else "WARNING"
            self.add_result(
                "Disk Space",
                status,
                f"{free_gb:.1f} GB free ({total_gb:.1f} GB total, {usage_percent:.1f}% used)",
                None
            )
        except Exception as e:
            self.add_result(
                "Disk Space",
                "WARNING",
                "Could not check disk space",
                str(e)
            )
        
        # Check memory (approximate)
        try:
            if self.platform == "Linux":
                # Linux-specific memory check
                with open('/proc/meminfo', 'r') as f:
                    meminfo = f.read()
                    for line in meminfo.splitlines():
                        if line.startswith('MemTotal:'):
                            total_mem = int(line.split()[1]) / 1024  # Convert to MB
                            status = "OK" if total_mem > 4096 else "WARNING"
                            self.add_result(
                                "System Memory",
                                status,
                                f"{total_mem / 1024:.1f} GB total",
                                None
                            )
                            break
            elif self.platform == "Darwin":
                # macOS memory check
                try:
                    import subprocess
                    result = subprocess.run(['sysctl', 'hw.memsize'], capture_output=True, text=True)
                    if result.returncode == 0:
                        mem_bytes = int(result.stdout.split()[1])
                        total_gb = mem_bytes / (1024**3)
                        status = "OK" if total_gb > 4 else "WARNING"
                        self.add_result(
                            "System Memory",
                            status,
                            f"{total_gb:.1f} GB total",
                            None
                        )
                    else:
                        raise Exception("sysctl failed")
                except Exception:
                    self.add_result(
                        "System Memory",
                        "INFO",
                        "Could not check memory on macOS",
                        None
                    )
            else:
                self.add_result(
                    "System Memory",
                    "INFO",
                    "Memory check not implemented for this platform",
                    None
                )
        except Exception as e:
            self.add_result(
                "System Memory",
                "INFO",
                "Could not check memory",
                str(e)
            )
    
    # ==================== Report Generation ====================
    
    def generate_report(self, output_file: Optional[str] = None) -> str:
        """Generate a comprehensive diagnostic report"""
        
        # Calculate statistics
        total = len(self.results)
        ok_count = sum(1 for r in self.results if r.status == "OK")
        warning_count = sum(1 for r in self.results if r.status == "WARNING")
        fail_count = sum(1 for r in self.results if r.status == "FAIL")
        info_count = sum(1 for r in self.results if r.status == "INFO")
        
        # Print summary
        self.section_header("Summary")
        self.log(f"Total Checks: {total}")
        self.log(f"  ✓ OK:       {ok_count}", Colors.OKGREEN)
        self.log(f"  ⚠ Warning:  {warning_count}", Colors.WARNING)
        self.log(f"  ✗ Failed:   {fail_count}", Colors.FAIL)
        self.log(f"  ℹ Info:     {info_count}", Colors.OKBLUE)
        
        duration = (datetime.datetime.now() - self.start_time).total_seconds()
        self.log(f"\nDuration: {duration:.2f} seconds")
        
        # Overall health status
        if fail_count > 0:
            health = "CRITICAL"
            health_color = Colors.FAIL
        elif warning_count > 3:
            health = "NEEDS ATTENTION"
            health_color = Colors.WARNING
        elif warning_count > 0:
            health = "GOOD"
            health_color = Colors.WARNING
        else:
            health = "EXCELLENT"
            health_color = Colors.OKGREEN
        
        self.log(f"\nOverall Health: {health}", health_color + Colors.BOLD)
        
        # Generate JSON report
        report = {
            "timestamp": self.start_time.isoformat(),
            "duration_seconds": duration,
            "platform": {
                "system": platform.system(),
                "release": platform.release(),
                "machine": platform.machine(),
            },
            "summary": {
                "total": total,
                "ok": ok_count,
                "warning": warning_count,
                "fail": fail_count,
                "info": info_count,
                "health": health
            },
            "results": [
                {
                    "name": r.name,
                    "status": r.status,
                    "message": r.message,
                    "details": r.details,
                    "timestamp": r.timestamp
                }
                for r in self.results
            ]
        }
        
        # Save to file if specified
        if output_file:
            try:
                with open(output_file, 'w') as f:
                    json.dump(report, f, indent=2)
                self.log(f"\n{Colors.OKGREEN}Report saved to: {output_file}{Colors.ENDC}")
            except Exception as e:
                self.log(f"\n{Colors.FAIL}Failed to save report: {e}{Colors.ENDC}")
        
        return json.dumps(report, indent=2)
    
    # ==================== Main Execution ====================
    
    def run_all_checks(self):
        """Run all diagnostic checks with comprehensive error handling"""
        self.log(f"{Colors.HEADER}{Colors.BOLD}")
        self.log("╔════════════════════════════════════════════════════════════════════════════╗")
        self.log("║                  AI FILE SORTER - DIAGNOSTIC TOOL                          ║")
        self.log("║                     Comprehensive System Check                             ║")
        self.log("╚════════════════════════════════════════════════════════════════════════════╝")
        self.log(Colors.ENDC)
        
        # List of all check methods
        check_methods = [
            ('System Information', self.check_system_info),
            ('File Structure', self.check_file_structure),
            ('Dependencies', self.check_dependencies),
            ('LLM Backends', self.check_llm_backends),
            ('Database', self.check_database),
            ('Configuration', self.check_configuration),
            ('Logs', self.check_logs),
            ('Features', self.check_features),
            ('Performance', self.check_performance),
        ]
        
        # Run all checks, logging errors but continuing
        for check_name, check_method in check_methods:
            try:
                check_method()
            except KeyboardInterrupt:
                self.log(f"\n{Colors.WARNING}Diagnostic interrupted by user{Colors.ENDC}")
                self.add_result(
                    f"{check_name} Check",
                    "FAIL",
                    "Interrupted by user",
                    "Diagnostic was cancelled before completion"
                )
                sys.exit(1)
            except Exception as e:
                self.log(f"\n{Colors.FAIL}Error in {check_name} check: {e}{Colors.ENDC}")
                
                # Log the error but continue with other checks
                import traceback
                error_details = traceback.format_exc()
                
                self.add_result(
                    f"{check_name} Check",
                    "FAIL",
                    f"Check failed with error: {str(e)}",
                    error_details
                )
                
                if self.verbose:
                    self.log(f"{Colors.FAIL}Traceback:{Colors.ENDC}", Colors.FAIL)
                    self.log(error_details)


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="AI File Sorter - Comprehensive Diagnostic Tool"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose output with detailed information"
    )
    parser.add_argument(
        "-o", "--output",
        type=str,
        help="Save diagnostic report to specified JSON file"
    )
    
    args = parser.parse_args()
    
    # Create and run diagnostic tool
    tool = DiagnosticTool(verbose=args.verbose)
    tool.run_all_checks()
    
    # Generate report
    default_output = f"diagnostic_report_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    output_file = args.output or default_output
    tool.generate_report(output_file)


if __name__ == "__main__":
    main()
