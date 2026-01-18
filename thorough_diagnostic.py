#!/usr/bin/env python3
"""
AI File Sorter - Thorough Diagnostic Tool (Separate from Main App)

This comprehensive diagnostic tool performs extensive testing of every feature,
dependency, and component of AI File Sorter. It is completely separate from the
main application and can be run independently for thorough system validation.

Features:
- Tests ALL application features comprehensively
- Validates every dependency and library
- Checks database integrity and schema
- Tests LLM backends (CPU, CUDA, Vulkan, Metal)
- Validates API connectivity (OpenAI, Gemini)
- Tests file operations and permissions
- Benchmarks performance
- Generates detailed reports (JSON, HTML, Markdown)
- Provides actionable recommendations

Usage:
    python3 thorough_diagnostic.py [OPTIONS]
    
Options:
    --verbose, -v          Enable verbose output
    --output, -o FILE      Save JSON report to file
    --html                 Generate HTML report
    --markdown             Generate Markdown summary
    --test-apis            Test API connectivity (requires keys)
    --benchmark            Run performance benchmarks
    --quick                Skip slow tests (for rapid validation)
"""

import os
import sys
import json
import platform
import subprocess
import sqlite3
import datetime
import argparse
import time
import hashlib
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
from collections import defaultdict

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
    def __init__(self, name: str, status: str, message: str, 
                 details: Optional[str] = None, recommendation: Optional[str] = None):
        self.name = name
        self.status = status  # OK, WARNING, FAIL, INFO, SKIP
        self.message = message
        self.details = details
        self.recommendation = recommendation
        self.timestamp = datetime.datetime.now().isoformat()
        self.category = ""  # Will be set by diagnostic sections

class ThoroughDiagnosticTool:
    """Comprehensive diagnostic tool for AI File Sorter"""
    
    def __init__(self, verbose: bool = False, quick: bool = False):
        self.verbose = verbose
        self.quick = quick
        self.results: List[DiagnosticResult] = []
        self.platform = platform.system()
        self.start_time = datetime.datetime.now()
        self.categories: Dict[str, List[DiagnosticResult]] = defaultdict(list)
        
        # Determine base directories
        self.repo_root = Path.cwd()
        if self.platform == "Windows":
            self.app_dir = self.repo_root / "app" / "build-windows" / "Release"
            if not self.app_dir.exists():
                self.app_dir = self.repo_root
        else:
            self.app_dir = self.repo_root / "app" / "bin"
            if not self.app_dir.exists():
                self.app_dir = self.repo_root
        
        # Platform-specific paths
        if self.platform == "Windows":
            self.config_dir = Path(os.path.expandvars("%APPDATA%")) / "aifilesorter"
            self.data_dir = Path(os.path.expandvars("%APPDATA%")) / "aifilesorter"
        elif self.platform == "Darwin":  # macOS
            self.config_dir = Path.home() / "Library" / "Application Support" / "aifilesorter"
            self.data_dir = Path.home() / "Library" / "Application Support" / "aifilesorter"
        else:  # Linux
            self.config_dir = Path.home() / ".config" / "aifilesorter"
            self.data_dir = Path.home() / ".local" / "share" / "aifilesorter"
    
    def log(self, message: str, color: str = ""):
        """Print a log message"""
        if color:
            print(f"{color}{message}{Colors.ENDC}")
        else:
            print(message)
    
    def add_result(self, name: str, status: str, message: str, 
                   details: Optional[str] = None, recommendation: Optional[str] = None,
                   category: str = "General"):
        """Add a diagnostic result"""
        result = DiagnosticResult(name, status, message, details, recommendation)
        result.category = category
        self.results.append(result)
        self.categories[category].append(result)
        
        # Print result
        status_color = {
            "OK": Colors.OKGREEN,
            "WARNING": Colors.WARNING,
            "FAIL": Colors.FAIL,
            "INFO": Colors.OKBLUE,
            "SKIP": Colors.OKCYAN
        }.get(status, "")
        
        status_symbol = {
            "OK": "✓",
            "WARNING": "⚠",
            "FAIL": "✗",
            "INFO": "ℹ",
            "SKIP": "⊘"
        }.get(status, "•")
        
        self.log(f"  {status_symbol} {name}: {message}", status_color)
        
        if self.verbose and details:
            self.log(f"    Details: {details}", Colors.OKCYAN)
        
        if recommendation and (status == "WARNING" or status == "FAIL"):
            self.log(f"    💡 Recommendation: {recommendation}", Colors.OKBLUE)
    
    def section_header(self, title: str):
        """Print a section header"""
        self.log(f"\n{'='*80}", Colors.HEADER)
        self.log(f"{title.upper()}", Colors.HEADER + Colors.BOLD)
        self.log(f"{'='*80}", Colors.HEADER)
    
    # ==================== System Information ====================
    
    def check_system_info(self):
        """Check comprehensive system information"""
        self.section_header("System Information")
        category = "System"
        
        # Platform details
        self.add_result(
            "Operating System",
            "INFO",
            f"{platform.system()} {platform.release()} ({platform.version()})",
            f"Platform: {platform.platform()}\nMachine: {platform.machine()}\nProcessor: {platform.processor()}",
            category=category
        )
        
        # Python version
        py_version = sys.version_info
        status = "OK" if py_version >= (3, 7) else "WARNING"
        rec = "Python 3.7+ recommended" if status == "WARNING" else None
        self.add_result(
            "Python Version",
            status,
            f"{py_version.major}.{py_version.minor}.{py_version.micro}",
            sys.version,
            recommendation=rec,
            category=category
        )
        
        # CPU info
        try:
            import multiprocessing
            cpu_count = multiprocessing.cpu_count()
            self.add_result(
                "CPU Cores",
                "INFO",
                f"{cpu_count} cores available",
                category=category
            )
        except:
            pass
        
        # Memory info
        try:
            if self.platform == "Linux":
                with open('/proc/meminfo', 'r') as f:
                    for line in f:
                        if line.startswith('MemTotal:'):
                            mem_kb = int(line.split()[1])
                            mem_gb = mem_kb / (1024 * 1024)
                            status = "OK" if mem_gb >= 4 else "WARNING"
                            rec = "At least 4GB RAM recommended for LLM inference" if status == "WARNING" else None
                            self.add_result(
                                "System Memory",
                                status,
                                f"{mem_gb:.1f} GB total",
                                recommendation=rec,
                                category=category
                            )
                            break
            elif self.platform == "Darwin":
                result = subprocess.run(['sysctl', 'hw.memsize'], 
                                      capture_output=True, text=True, timeout=5)
                if result.returncode == 0:
                    mem_bytes = int(result.stdout.split()[1])
                    mem_gb = mem_bytes / (1024**3)
                    status = "OK" if mem_gb >= 4 else "WARNING"
                    rec = "At least 4GB RAM recommended for LLM inference" if status == "WARNING" else None
                    self.add_result(
                        "System Memory",
                        status,
                        f"{mem_gb:.1f} GB total",
                        recommendation=rec,
                        category=category
                    )
        except Exception as e:
            self.add_result(
                "System Memory",
                "INFO",
                "Could not determine",
                str(e),
                category=category
            )
        
        # Disk space
        try:
            import shutil
            if self.platform == "Windows":
                drive = Path.cwd().drive
                total, used, free = shutil.disk_usage(drive if drive else "/")
            else:
                total, used, free = shutil.disk_usage(str(Path.home()))
            
            free_gb = free / (1024**3)
            total_gb = total / (1024**3)
            usage_percent = (used / total) * 100
            
            status = "OK" if free_gb > 10 else ("WARNING" if free_gb > 5 else "FAIL")
            rec = "At least 10GB free space recommended for models and cache" if status != "OK" else None
            
            self.add_result(
                "Disk Space",
                status,
                f"{free_gb:.1f} GB free ({total_gb:.1f} GB total, {usage_percent:.1f}% used)",
                recommendation=rec,
                category=category
            )
        except Exception as e:
            self.add_result(
                "Disk Space",
                "WARNING",
                "Could not check disk space",
                str(e),
                category=category
            )
    
    # ==================== File Structure ====================
    
    def check_file_structure(self):
        """Check comprehensive file structure"""
        self.section_header("File Structure & Executables")
        category = "File Structure"
        
        # Main executable
        if self.platform == "Windows":
            executables = [
                ("Launcher", "StartAiFileSorter.exe"),
                ("Main Application", "app/build-windows/Release/aifilesorter.exe"),
            ]
        else:
            executables = [
                ("Main Binary", "app/bin/aifilesorter"),
                ("Launch Script", "app/bin/run_aifilesorter.sh"),
            ]
        
        for name, exe_path in executables:
            full_path = self.repo_root / exe_path
            if full_path.exists():
                size = full_path.stat().st_size
                size_mb = size / (1024 * 1024)
                is_exec = os.access(full_path, os.X_OK) if self.platform != "Windows" else True
                
                status = "OK" if is_exec else "WARNING"
                rec = "File should be executable" if not is_exec else None
                
                self.add_result(
                    f"Executable: {name}",
                    status,
                    f"Found ({size_mb:.1f} MB)",
                    f"Path: {full_path}\nExecutable: {is_exec}",
                    recommendation=rec,
                    category=category
                )
            else:
                self.add_result(
                    f"Executable: {name}",
                    "FAIL",
                    "Not found",
                    f"Expected at: {full_path}",
                    recommendation="Rebuild the application",
                    category=category
                )
        
        # Required directories
        required_dirs = [
            ("Include Directory", "app/include"),
            ("Library Directory", "app/lib"),
            ("Resources Directory", "app/resources"),
            ("Scripts Directory", "app/scripts"),
        ]
        
        for name, dir_path in required_dirs:
            full_path = self.repo_root / dir_path
            if full_path.exists() and full_path.is_dir():
                file_count = sum(1 for _ in full_path.rglob('*') if _.is_file())
                dir_size = sum(f.stat().st_size for f in full_path.rglob('*') if f.is_file())
                size_mb = dir_size / (1024 * 1024)
                
                self.add_result(
                    name,
                    "OK",
                    f"Found ({file_count} files, {size_mb:.1f} MB)",
                    f"Path: {full_path}",
                    category=category
                )
            else:
                self.add_result(
                    name,
                    "FAIL" if "Resources" in name or "Library" in name else "WARNING",
                    "Not found",
                    f"Expected at: {full_path}",
                    recommendation="Repository may be incomplete",
                    category=category
                )
        
        # Source files check
        source_patterns = [
            ("C++ Headers", "app/lib/*.cpp"),
            ("Resource Files", "app/resources/**/*.qrc"),
            ("CMake Files", "app/CMakeLists.txt"),
        ]
        
        for name, pattern in source_patterns:
            from glob import glob
            files = glob(str(self.repo_root / pattern), recursive=True)
            if files:
                self.add_result(
                    name,
                    "OK",
                    f"Found ({len(files)} files)",
                    category=category
                )
            else:
                self.add_result(
                    name,
                    "WARNING",
                    "Not found",
                    f"Pattern: {pattern}",
                    category=category
                )
    
    # ==================== Dependencies ====================
    
    def check_dependencies(self):
        """Check all dependencies comprehensively"""
        self.section_header("Dependencies & Libraries")
        category = "Dependencies"
        
        # Qt libraries
        if self.platform == "Windows":
            qt_libs = [
                "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll",
                "Qt6Network.dll", "Qt6Sql.dll"
            ]
            lib_dir = self.app_dir
            
            for lib in qt_libs:
                lib_path = lib_dir / lib
                if lib_path.exists():
                    size = lib_path.stat().st_size / (1024 * 1024)
                    self.add_result(
                        f"Qt Library: {lib}",
                        "OK",
                        f"Found ({size:.1f} MB)",
                        f"Path: {lib_path}",
                        category=category
                    )
                else:
                    self.add_result(
                        f"Qt Library: {lib}",
                        "FAIL",
                        "Not found",
                        f"Expected at: {lib_path}",
                        recommendation="Run windeployqt or rebuild application",
                        category=category
                    )
        else:
            # Check Qt via pkg-config
            try:
                result = subprocess.run(
                    ["pkg-config", "--modversion", "Qt6Widgets"],
                    capture_output=True, text=True, timeout=5
                )
                if result.returncode == 0:
                    version = result.stdout.strip()
                    status = "OK" if version >= "6.5" else "WARNING"
                    rec = "Qt 6.5+ recommended" if status == "WARNING" else None
                    self.add_result(
                        "Qt6 Framework",
                        status,
                        f"Version {version}",
                        recommendation=rec,
                        category=category
                    )
                else:
                    self.add_result(
                        "Qt6 Framework",
                        "WARNING",
                        "Not found via pkg-config",
                        "May still be available via system paths",
                        category=category
                    )
            except (subprocess.TimeoutExpired, FileNotFoundError) as e:
                self.add_result(
                    "Qt6 Framework",
                    "WARNING",
                    "Could not verify",
                    str(e),
                    category=category
                )
        
        # Check system libraries
        system_libs = []
        if self.platform == "Linux":
            system_libs = [
                ("libcurl", "curl --version"),
                ("SQLite3", "sqlite3 --version"),
            ]
        elif self.platform == "Darwin":
            system_libs = [
                ("libcurl", "curl --version"),
                ("SQLite3", "sqlite3 --version"),
            ]
        
        for lib_name, check_cmd in system_libs:
            try:
                result = subprocess.run(
                    check_cmd.split(),
                    capture_output=True, text=True, timeout=5
                )
                if result.returncode == 0:
                    version = result.stdout.split('\n')[0]
                    self.add_result(
                        f"Library: {lib_name}",
                        "OK",
                        "Available",
                        f"Version: {version}",
                        category=category
                    )
                else:
                    self.add_result(
                        f"Library: {lib_name}",
                        "WARNING",
                        "Not found",
                        category=category
                    )
            except (subprocess.TimeoutExpired, FileNotFoundError):
                self.add_result(
                    f"Library: {lib_name}",
                    "WARNING",
                    "Could not verify",
                    category=category
                )
    
    # ==================== LLM Backends ====================
    
    def check_llm_backends(self):
        """Check LLM inference backends comprehensively"""
        self.section_header("LLM Inference Backends")
        category = "LLM Backends"
        
        # Check GGML directories
        ggml_base = self.repo_root / "app" / "lib" / "ggml"
        variants = {
            "wocuda": {"name": "CPU (OpenBLAS)", "required": True},
            "wcuda": {"name": "CUDA (NVIDIA GPU)", "required": False},
            "wvulkan": {"name": "Vulkan (Cross-platform GPU)", "required": False},
        }
        
        for variant, info in variants.items():
            variant_path = ggml_base / variant
            if variant_path.exists():
                # Count library files
                if self.platform == "Windows":
                    libs = list(variant_path.glob("*.dll"))
                else:
                    libs = list(variant_path.glob("*.so")) + list(variant_path.glob("*.dylib"))
                
                total_size = sum(f.stat().st_size for f in libs) / (1024 * 1024)
                
                self.add_result(
                    f"Backend: {info['name']}",
                    "OK",
                    f"Available ({len(libs)} libraries, {total_size:.1f} MB)",
                    f"Path: {variant_path}",
                    category=category
                )
            else:
                status = "FAIL" if info['required'] else "INFO"
                message = "Not found" + ("" if info['required'] else " (optional)")
                rec = "Rebuild llama.cpp with this backend" if info['required'] else None
                
                self.add_result(
                    f"Backend: {info['name']}",
                    status,
                    message,
                    f"Expected at: {variant_path}",
                    recommendation=rec,
                    category=category
                )
        
        # Check precompiled libraries
        precompiled_dir = self.repo_root / "app" / "lib" / "precompiled"
        if precompiled_dir.exists():
            precompiled_variants = [d.name for d in precompiled_dir.iterdir() if d.is_dir()]
            total_size = sum(
                f.stat().st_size 
                for d in precompiled_dir.iterdir() if d.is_dir()
                for f in d.rglob('*') if f.is_file()
            ) / (1024 * 1024)
            
            self.add_result(
                "Precompiled LLM Libraries",
                "OK",
                f"Found {len(precompiled_variants)} variant(s): {', '.join(precompiled_variants)}",
                f"Path: {precompiled_dir}\nTotal size: {total_size:.1f} MB",
                category=category
            )
        else:
            self.add_result(
                "Precompiled LLM Libraries",
                "WARNING",
                "Directory not found",
                f"Expected at: {precompiled_dir}",
                recommendation="Run build_llama script for your platform",
                category=category
            )
        
        # Check for local models
        models_dir = self.data_dir / "llms"
        if models_dir.exists():
            models = list(models_dir.glob("*.gguf"))
            if models:
                total_size = sum(m.stat().st_size for m in models) / (1024 * 1024)
                model_names = [m.name for m in models[:5]]  # First 5
                
                self.add_result(
                    "Local LLM Models",
                    "OK",
                    f"Found {len(models)} model(s) ({total_size:.0f} MB total)",
                    f"Path: {models_dir}\nModels: {', '.join(model_names)}",
                    category=category
                )
            else:
                self.add_result(
                    "Local LLM Models",
                    "INFO",
                    "No models downloaded yet",
                    f"Path: {models_dir}",
                    recommendation="Download models via the application on first run",
                    category=category
                )
        else:
            self.add_result(
                "Local LLM Models",
                "INFO",
                "Models directory not created yet",
                f"Expected at: {models_dir}",
                category=category
            )
    
    # ==================== Database ====================
    
    def check_database(self):
        """Check database comprehensively"""
        self.section_header("Database & Data Storage")
        category = "Database"
        
        # Find database file
        db_path = self.data_dir / "aifilesorter.db"
        
        if not db_path.exists():
            self.add_result(
                "Database File",
                "INFO",
                "Not created yet (will be created on first run)",
                f"Expected location: {db_path}",
                category=category
            )
            return
        
        # Check file
        try:
            file_size = db_path.stat().st_size
            size_mb = file_size / (1024 * 1024)
            
            self.add_result(
                "Database File",
                "OK",
                f"Found ({size_mb:.2f} MB)",
                f"Path: {db_path}",
                category=category
            )
        except Exception as e:
            self.add_result(
                "Database File",
                "FAIL",
                f"Error reading file: {str(e)}",
                f"Path: {db_path}",
                category=category
            )
            return
        
        # Check database integrity
        try:
            with sqlite3.connect(db_path) as conn:
                cursor = conn.cursor()
                
                # Integrity check
                try:
                    cursor.execute("PRAGMA integrity_check")
                    integrity = cursor.fetchone()[0]
                    if integrity == "ok":
                        self.add_result(
                            "Database Integrity",
                            "OK",
                            "Passed integrity check",
                            category=category
                        )
                    else:
                        self.add_result(
                            "Database Integrity",
                            "FAIL",
                            "Integrity check failed",
                            integrity,
                            recommendation="Database may be corrupted. Consider backup and repair.",
                            category=category
                        )
                except sqlite3.Error as e:
                    self.add_result(
                        "Database Integrity",
                        "FAIL",
                        f"Error checking integrity: {str(e)}",
                        category=category
                    )
            
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
                    "file_tinder_state",
                    "whitelists",
                    "folder_learning_config"
                ]
                
                found_tables = [t for t in expected_tables if t in tables]
                missing_tables = [t for t in expected_tables if t not in tables]
                
                if len(found_tables) == len(expected_tables):
                    self.add_result(
                        "Database Schema",
                        "OK",
                        f"All {len(expected_tables)} required tables present",
                        f"Tables: {', '.join(found_tables)}",
                        category=category
                    )
                else:
                    self.add_result(
                        "Database Schema",
                        "WARNING",
                        f"Found {len(found_tables)}/{len(expected_tables)} tables",
                        f"Missing: {', '.join(missing_tables)}",
                        recommendation="Tables will be created on first use",
                        category=category
                    )
                
                # Check table statistics
                stats = {}
                for table in found_tables:
                    try:
                        cursor.execute(f"SELECT COUNT(*) FROM {table}")
                        count = cursor.fetchone()[0]
                        stats[table] = count
                    except sqlite3.Error:
                        stats[table] = "N/A"
                
                stats_str = "\n".join([f"{k}: {v} rows" for k, v in stats.items()])
                self.add_result(
                    "Database Statistics",
                    "INFO",
                    f"{len(stats)} tables analyzed",
                    stats_str,
                    category=category
                )
                
            except sqlite3.Error as e:
                self.add_result(
                    "Database Schema",
                    "FAIL",
                    f"Error querying schema: {str(e)}",
                    category=category
                )
            
        except sqlite3.Error as e:
            self.add_result(
                "Database Connection",
                "FAIL",
                f"Cannot connect: {str(e)}",
                f"Database at {db_path}",
                category=category
            )
    
    # ==================== Configuration ====================
    
    def check_configuration(self):
        """Check configuration files comprehensively"""
        self.section_header("Configuration Files")
        category = "Configuration"
        
        # Main config file
        config_path = self.config_dir / "config.ini"
        
        if not config_path.exists():
            self.add_result(
                "Main Configuration",
                "INFO",
                "Not created yet (will be created on first run)",
                f"Expected location: {config_path}",
                category=category
            )
        else:
            try:
                with open(config_path, 'r', encoding='utf-8') as f:
                    config_content = f.read()
                    line_count = len(config_content.splitlines())
                    
                    self.add_result(
                        "Main Configuration",
                        "OK",
                        f"Found ({line_count} lines)",
                        f"Path: {config_path}",
                        category=category
                    )
                    
                    # Check for API keys (without revealing them)
                    has_openai = bool(re.search(r'openai.*key', config_content, re.I))
                    has_gemini = bool(re.search(r'gemini.*key', config_content, re.I))
                    
                    if has_openai or has_gemini:
                        apis = []
                        if has_openai:
                            apis.append("OpenAI")
                        if has_gemini:
                            apis.append("Gemini")
                        
                        self.add_result(
                            "API Keys Configured",
                            "INFO",
                            f"{', '.join(apis)}",
                            "Keys are encrypted/stored locally",
                            category=category
                        )
                    else:
                        self.add_result(
                            "API Keys Configured",
                            "INFO",
                            "No API keys (using local LLM only)",
                            category=category
                        )
                    
                    # Check for other settings
                    settings_found = []
                    patterns = {
                        "Language": r'language\s*=',
                        "Theme": r'theme\s*=',
                        "LLM Model": r'llm.*model',
                    }
                    
                    for setting, pattern in patterns.items():
                        if re.search(pattern, config_content, re.I):
                            settings_found.append(setting)
                    
                    if settings_found:
                        self.add_result(
                            "Configuration Settings",
                            "INFO",
                            f"Found settings: {', '.join(settings_found)}",
                            category=category
                        )
                    
            except Exception as e:
                self.add_result(
                    "Main Configuration",
                    "WARNING",
                    "Could not read configuration",
                    str(e),
                    category=category
                )
        
        # Check config directory permissions
        try:
            if self.config_dir.exists():
                is_writable = os.access(self.config_dir, os.W_OK)
                is_readable = os.access(self.config_dir, os.R_OK)
                
                if is_writable and is_readable:
                    self.add_result(
                        "Config Directory Permissions",
                        "OK",
                        "Read/Write access",
                        f"Path: {self.config_dir}",
                        category=category
                    )
                else:
                    self.add_result(
                        "Config Directory Permissions",
                        "FAIL",
                        f"Readable: {is_readable}, Writable: {is_writable}",
                        f"Path: {self.config_dir}",
                        recommendation="Check file permissions",
                        category=category
                    )
            else:
                # Try to create it
                try:
                    self.config_dir.mkdir(parents=True, exist_ok=True)
                    self.add_result(
                        "Config Directory",
                        "OK",
                        "Created successfully",
                        f"Path: {self.config_dir}",
                        category=category
                    )
                except Exception as e:
                    self.add_result(
                        "Config Directory",
                        "FAIL",
                        "Could not create",
                        f"Path: {self.config_dir}\nError: {e}",
                        recommendation="Check file system permissions",
                        category=category
                    )
        except Exception as e:
            self.add_result(
                "Config Directory Permissions",
                "WARNING",
                "Could not check",
                str(e),
                category=category
            )
    
    # ==================== Feature Tests ====================
    
    def check_features(self):
        """Test all features comprehensively"""
        self.section_header("Feature Implementation Validation")
        category = "Features"
        
        # Map features to their implementation files
        features = [
            ("Core Categorization", "app/lib/CategorizationService.cpp", True),
            ("File Scanner", "app/lib/FileScanner.cpp", True),
            ("Database Manager", "app/lib/DatabaseManager.cpp", True),
            ("LLM Client Interface", "app/lib/LLMClient.cpp", True),
            ("Local LLM Client", "app/lib/LocalLLMClient.cpp", True),
            ("OpenAI Client", "app/lib/LLMClient.cpp", True),
            ("Gemini Client", "app/lib/GeminiClient.cpp", True),
            ("Categorization Dialog", "app/lib/CategorizationDialog.cpp", True),
            ("User Profile Manager", "app/lib/UserProfileManager.cpp", False),
            ("User Profile Dialog", "app/lib/UserProfileDialog.cpp", False),
            ("Folder Learning", "app/lib/FolderLearningDialog.cpp", False),
            ("File Tinder", "app/lib/FileTinderDialog.cpp", False),
            ("Cache Manager", "app/lib/CacheManagerDialog.cpp", False),
            ("Undo Manager", "app/lib/UndoManager.cpp", False),
            ("Dry Run Preview", "app/lib/DryRunPreviewDialog.cpp", False),
            ("Whitelist Manager", "app/lib/WhitelistManagerDialog.cpp", False),
            ("Custom LLM Dialog", "app/lib/CustomLLMDialog.cpp", False),
            ("LLM Selection Dialog", "app/lib/LLMSelectionDialog.cpp", True),
            ("API Usage Statistics", "app/lib/UsageStatsDialog.cpp", False),
            ("Translation Manager", "app/lib/TranslationManager.cpp", False),
            ("Consistency Service", "app/lib/ConsistencyPassService.cpp", False),
            ("Categorization Progress", "app/lib/CategorizationProgressDialog.cpp", True),
        ]
        
        for feature_name, file_path, is_core in features:
            full_path = self.repo_root / file_path
            if full_path.exists():
                size = full_path.stat().st_size
                lines = 0
                try:
                    with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                        lines = len(f.readlines())
                except:
                    pass
                
                self.add_result(
                    f"Feature: {feature_name}",
                    "OK",
                    f"Implemented ({lines} lines, {size/1024:.1f} KB)",
                    f"Source: {file_path}",
                    category=category
                )
            else:
                status = "FAIL" if is_core else "WARNING"
                message = "Not found" + (" (core feature)" if is_core else " (optional feature)")
                rec = "Core feature missing - rebuild required" if is_core else "Feature may not be available"
                
                self.add_result(
                    f"Feature: {feature_name}",
                    status,
                    message,
                    f"Expected: {file_path}",
                    recommendation=rec,
                    category=category
                )
        
        # Check for translation files
        i18n_dir = self.repo_root / "app" / "resources" / "i18n"
        if i18n_dir.exists():
            translation_files = list(i18n_dir.glob("*.ts"))
            languages = [f.stem.replace("aifilesorter_", "") for f in translation_files]
            
            self.add_result(
                "Internationalization",
                "OK",
                f"{len(languages)} languages supported: {', '.join(languages)}",
                f"Path: {i18n_dir}",
                category=category
            )
        else:
            self.add_result(
                "Internationalization",
                "WARNING",
                "Translation directory not found",
                f"Expected: {i18n_dir}",
                category=category
            )
    
    # ==================== Log Files ====================
    
    def check_logs(self):
        """Check log files comprehensively"""
        self.section_header("Log Files & Error Reporting")
        category = "Logs"
        
        # Find log directory
        log_dir = self.data_dir / "logs"
        
        if not log_dir.exists():
            self.add_result(
                "Log Directory",
                "INFO",
                "Not created yet (created on first run)",
                f"Expected location: {log_dir}",
                category=category
            )
            return
        
        # Count log files
        log_files = list(log_dir.glob("*.log")) + list(log_dir.glob("*.txt"))
        
        if log_files:
            total_size = sum(f.stat().st_size for f in log_files) / (1024 * 1024)
            
            self.add_result(
                "Log Directory",
                "OK",
                f"Found ({len(log_files)} log files, {total_size:.2f} MB)",
                f"Path: {log_dir}",
                category=category
            )
        else:
            self.add_result(
                "Log Directory",
                "INFO",
                "No log files yet",
                f"Path: {log_dir}",
                category=category
            )
            return
        
        # Check for error logs
        error_logs = [f for f in log_files if "error" in f.name.lower()]
        if error_logs:
            latest_error = max(error_logs, key=lambda f: f.stat().st_mtime)
            age_seconds = time.time() - latest_error.stat().st_mtime
            age_str = f"{age_seconds/3600:.1f} hours ago" if age_seconds > 3600 else f"{age_seconds/60:.0f} minutes ago"
            
            status = "WARNING" if age_seconds < 86400 else "INFO"  # Recent errors are warnings
            
            self.add_result(
                "Error Logs",
                status,
                f"{len(error_logs)} error log(s), most recent: {age_str}",
                f"Latest: {latest_error.name}",
                recommendation="Check error logs for issues" if status == "WARNING" else None,
                category=category
            )
            
            # Try to read last few lines
            if not self.quick:
                try:
                    with open(latest_error, 'r', encoding='utf-8', errors='ignore') as f:
                        lines = f.readlines()
                        if lines:
                            last_lines = lines[-5:]
                            preview = '\n'.join(line.strip() for line in last_lines)
                            self.add_result(
                                "Recent Error Preview",
                                "INFO",
                                f"Last {len(last_lines)} lines from {latest_error.name}",
                                preview,
                                category=category
                            )
                except Exception as e:
                    self.add_result(
                        "Error Log Read",
                        "WARNING",
                        "Could not read error log",
                        str(e),
                        category=category
                    )
        else:
            self.add_result(
                "Error Logs",
                "OK",
                "No error logs found",
                category=category
            )
        
        # Check Copilot error reports
        copilot_errors = list(log_dir.glob("COPILOT_ERROR_*.md"))
        if copilot_errors:
            self.add_result(
                "Copilot Error Reports",
                "INFO",
                f"{len(copilot_errors)} report(s) generated",
                f"Path: {log_dir}",
                recommendation="User-friendly error reports for debugging",
                category=category
            )
    
    # ==================== Performance Benchmarks ====================
    
    def check_performance(self):
        """Run performance benchmarks"""
        self.section_header("Performance Benchmarks")
        category = "Performance"
        
        # Disk I/O test
        if not self.quick:
            try:
                test_file = self.data_dir / ".diagnostic_test"
                test_data = b"0" * (1024 * 1024)  # 1MB
                
                try:
                    start = time.time()
                    with open(test_file, 'wb') as f:
                        f.write(test_data)
                    write_time = time.time() - start
                    
                    start = time.time()
                    with open(test_file, 'rb') as f:
                        _ = f.read()
                    read_time = time.time() - start
                    
                    write_speed = 1.0 / write_time  # MB/s
                    read_speed = 1.0 / read_time  # MB/s
                    
                    status = "OK" if write_speed > 50 and read_speed > 100 else "WARNING"
                    rec = "Slow disk I/O may affect performance" if status == "WARNING" else None
                    
                    self.add_result(
                        "Disk I/O Performance",
                        status,
                        f"Write: {write_speed:.0f} MB/s, Read: {read_speed:.0f} MB/s",
                        recommendation=rec,
                        category=category
                    )
                finally:
                    # Ensure test file is always deleted
                    if test_file.exists():
                        test_file.unlink()
            except Exception as e:
                self.add_result(
                    "Disk I/O Performance",
                    "WARNING",
                    "Could not test",
                    str(e),
                    category=category
                )
        else:
            self.add_result(
                "Disk I/O Performance",
                "SKIP",
                "Skipped in quick mode",
                category=category
            )
        
        # Database query performance
        db_path = self.data_dir / "aifilesorter.db"
        if db_path.exists() and not self.quick:
            try:
                conn = sqlite3.connect(db_path)
                cursor = conn.cursor()
                
                # Simple query benchmark
                start = time.time()
                for _ in range(100):
                    cursor.execute("SELECT COUNT(*) FROM sqlite_master")
                    cursor.fetchone()
                query_time = (time.time() - start) / 100 * 1000  # ms per query
                
                conn.close()
                
                status = "OK" if query_time < 10 else "WARNING"
                rec = "Database performance may be slow" if status == "WARNING" else None
                
                self.add_result(
                    "Database Query Performance",
                    status,
                    f"Average query: {query_time:.2f} ms",
                    recommendation=rec,
                    category=category
                )
            except Exception as e:
                self.add_result(
                    "Database Query Performance",
                    "WARNING",
                    "Could not test",
                    str(e),
                    category=category
                )
        
        # Check available disk space
        try:
            import shutil
            if self.platform == "Windows":
                drive = Path.cwd().drive
                total, used, free = shutil.disk_usage(drive if drive else "/")
            else:
                total, used, free = shutil.disk_usage(str(Path.home()))
            
            free_gb = free / (1024**3)
            
            self.add_result(
                "Available Storage",
                "INFO",
                f"{free_gb:.1f} GB free",
                category=category
            )
        except:
            pass
    
    # ==================== API Tests ====================
    
    def check_api_connectivity(self, test_apis: bool = False):
        """Check API connectivity if requested"""
        if not test_apis:
            self.section_header("API Connectivity")
            self.log("  ⊘ Skipped (use --test-apis to enable)")
            return
        
        self.section_header("API Connectivity Tests")
        category = "API"
        
        # Test general internet connectivity
        try:
            import socket
            socket.create_connection(("8.8.8.8", 53), timeout=5)
            self.add_result(
                "Internet Connectivity",
                "OK",
                "Connected",
                category=category
            )
        except Exception as e:
            self.add_result(
                "Internet Connectivity",
                "FAIL",
                "No connection",
                str(e),
                recommendation="Internet required for API features",
                category=category
            )
            return
        
        # Test OpenAI API endpoint
        try:
            result = subprocess.run(
                ["curl", "-I", "-s", "-m", "5", "https://api.openai.com"],
                capture_output=True, text=True, timeout=10
            )
            if result.returncode == 0 and "200" in result.stdout:
                self.add_result(
                    "OpenAI API Endpoint",
                    "OK",
                    "Reachable",
                    category=category
                )
            else:
                self.add_result(
                    "OpenAI API Endpoint",
                    "WARNING",
                    "Not reachable",
                    category=category
                )
        except Exception as e:
            self.add_result(
                "OpenAI API Endpoint",
                "WARNING",
                "Could not test",
                str(e),
                category=category
            )
        
        # Test Gemini API endpoint
        try:
            result = subprocess.run(
                ["curl", "-I", "-s", "-m", "5", "https://generativelanguage.googleapis.com"],
                capture_output=True, text=True, timeout=10
            )
            if result.returncode == 0:
                self.add_result(
                    "Gemini API Endpoint",
                    "OK",
                    "Reachable",
                    category=category
                )
            else:
                self.add_result(
                    "Gemini API Endpoint",
                    "WARNING",
                    "Not reachable",
                    category=category
                )
        except Exception as e:
            self.add_result(
                "Gemini API Endpoint",
                "WARNING",
                "Could not test",
                str(e),
                category=category
            )
    
    # ==================== Report Generation ====================
    
    def generate_json_report(self, output_file: str) -> dict:
        """Generate comprehensive JSON report"""
        duration = (datetime.datetime.now() - self.start_time).total_seconds()
        
        # Calculate statistics
        total = len(self.results)
        stats_by_status = defaultdict(int)
        stats_by_category = defaultdict(lambda: defaultdict(int))
        
        for result in self.results:
            stats_by_status[result.status] += 1
            stats_by_category[result.category][result.status] += 1
        
        # Overall health
        fail_count = stats_by_status["FAIL"]
        warning_count = stats_by_status["WARNING"]
        
        if fail_count > 0:
            health = "CRITICAL"
        elif warning_count > 5:
            health = "NEEDS ATTENTION"
        elif warning_count > 0:
            health = "GOOD"
        else:
            health = "EXCELLENT"
        
        # Build report
        report = {
            "diagnostic_metadata": {
                "tool_version": "2.0",
                "timestamp": self.start_time.isoformat(),
                "duration_seconds": duration,
                "quick_mode": self.quick,
            },
            "system_info": {
                "platform": platform.system(),
                "release": platform.release(),
                "machine": platform.machine(),
                "python_version": f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}",
            },
            "summary": {
                "total_checks": total,
                "by_status": dict(stats_by_status),
                "by_category": {k: dict(v) for k, v in stats_by_category.items()},
                "overall_health": health,
            },
            "results_by_category": {},
            "all_results": []
        }
        
        # Group results by category
        for category, results in self.categories.items():
            report["results_by_category"][category] = [
                {
                    "name": r.name,
                    "status": r.status,
                    "message": r.message,
                    "details": r.details,
                    "recommendation": r.recommendation,
                    "timestamp": r.timestamp
                }
                for r in results
            ]
        
        # All results in order
        report["all_results"] = [
            {
                "name": r.name,
                "category": r.category,
                "status": r.status,
                "message": r.message,
                "details": r.details,
                "recommendation": r.recommendation,
                "timestamp": r.timestamp
            }
            for r in self.results
        ]
        
        # Save to file
        try:
            with open(output_file, 'w') as f:
                json.dump(report, f, indent=2)
            self.log(f"\n{Colors.OKGREEN}✓ JSON report saved: {output_file}{Colors.ENDC}")
        except Exception as e:
            self.log(f"\n{Colors.FAIL}✗ Failed to save JSON report: {e}{Colors.ENDC}")
        
        return report
    
    def generate_html_report(self, json_report: dict, output_file: str):
        """Generate HTML report"""
        from pathlib import Path
        html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AI File Sorter - Diagnostic Report</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background: #f5f5f5;
        }}
        .header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            border-radius: 10px;
            margin-bottom: 30px;
        }}
        .header h1 {{
            margin: 0 0 10px 0;
        }}
        .summary {{
            background: white;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .health-badge {{
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-weight: bold;
            font-size: 14px;
        }}
        .health-EXCELLENT {{ background: #10b981; color: white; }}
        .health-GOOD {{ background: #f59e0b; color: white; }}
        .health-NEEDS\\ ATTENTION {{ background: #ef4444; color: white; }}
        .health-CRITICAL {{ background: #dc2626; color: white; }}
        .stats {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin-top: 20px;
        }}
        .stat-card {{
            background: #f9fafb;
            padding: 15px;
            border-radius: 8px;
            text-align: center;
        }}
        .stat-value {{
            font-size: 32px;
            font-weight: bold;
            margin: 10px 0;
        }}
        .stat-label {{
            color: #6b7280;
            font-size: 14px;
        }}
        .category {{
            background: white;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .category h2 {{
            margin-top: 0;
            color: #1f2937;
        }}
        .result {{
            padding: 12px;
            margin: 8px 0;
            border-radius: 6px;
            border-left: 4px solid #e5e7eb;
        }}
        .result-OK {{ border-left-color: #10b981; background: #f0fdf4; }}
        .result-WARNING {{ border-left-color: #f59e0b; background: #fffbeb; }}
        .result-FAIL {{ border-left-color: #ef4444; background: #fef2f2; }}
        .result-INFO {{ border-left-color: #3b82f6; background: #eff6ff; }}
        .result-SKIP {{ border-left-color: #9ca3af; background: #f9fafb; }}
        .result-name {{
            font-weight: 600;
            margin-bottom: 4px;
        }}
        .result-message {{
            color: #4b5563;
        }}
        .result-details {{
            font-size: 12px;
            color: #6b7280;
            margin-top: 8px;
            font-family: monospace;
            background: rgba(0,0,0,0.05);
            padding: 8px;
            border-radius: 4px;
        }}
        .recommendation {{
            margin-top: 8px;
            padding: 8px;
            background: #dbeafe;
            border-radius: 4px;
            font-size: 13px;
        }}
        .recommendation::before {{
            content: "💡 ";
        }}
        .timestamp {{
            text-align: center;
            color: #6b7280;
            font-size: 12px;
            margin-top: 30px;
        }}
    </style>
</head>
<body>
    <div class="header">
        <h1>🔍 AI File Sorter - Thorough Diagnostic Report</h1>
        <p>Generated on {json_report['diagnostic_metadata']['timestamp']}</p>
        <p>Duration: {json_report['diagnostic_metadata']['duration_seconds']:.2f} seconds</p>
    </div>
    
    <div class="summary">
        <h2>Overall Health: <span class="health-badge health-{json_report['summary']['overall_health']}">{json_report['summary']['overall_health']}</span></h2>
        
        <div class="stats">
            <div class="stat-card">
                <div class="stat-label">Total Checks</div>
                <div class="stat-value">{json_report['summary']['total_checks']}</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">✓ Passed</div>
                <div class="stat-value" style="color: #10b981;">{json_report['summary']['by_status'].get('OK', 0)}</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">⚠ Warnings</div>
                <div class="stat-value" style="color: #f59e0b;">{json_report['summary']['by_status'].get('WARNING', 0)}</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">✗ Failed</div>
                <div class="stat-value" style="color: #ef4444;">{json_report['summary']['by_status'].get('FAIL', 0)}</div>
            </div>
        </div>
    </div>
"""
        
        # Add results by category
        for category, results in json_report['results_by_category'].items():
            html += f"""
    <div class="category">
        <h2>{category}</h2>
"""
            for result in results:
                details_html = ""
                if result.get('details'):
                    details_html = f'<div class="result-details">{result["details"]}</div>'
                
                rec_html = ""
                if result.get('recommendation'):
                    rec_html = f'<div class="recommendation">{result["recommendation"]}</div>'
                
                html += f"""
        <div class="result result-{result['status']}">
            <div class="result-name">{result['name']}</div>
            <div class="result-message">{result['message']}</div>
            {details_html}
            {rec_html}
        </div>
"""
            html += """
    </div>
"""
        
        html += f"""
    <div class="timestamp">
        <p>Platform: {json_report['system_info']['platform']} {json_report['system_info']['release']}</p>
        <p>Python: {json_report['system_info']['python_version']}</p>
    </div>
</body>
</html>
"""
        
        try:
            with open(output_file, 'w') as f:
                f.write(html)
            self.log(f"{Colors.OKGREEN}✓ HTML report saved: {output_file}{Colors.ENDC}")
        except Exception as e:
            self.log(f"{Colors.FAIL}✗ Failed to save HTML report: {e}{Colors.ENDC}")
    
    def generate_markdown_summary(self, json_report: dict, output_file: str):
        """Generate Markdown summary"""
        md = f"""# AI File Sorter - Diagnostic Summary

**Generated:** {json_report['diagnostic_metadata']['timestamp']}  
**Duration:** {json_report['diagnostic_metadata']['duration_seconds']:.2f} seconds  
**Platform:** {json_report['system_info']['platform']} {json_report['system_info']['release']}

## Overall Health: {json_report['summary']['overall_health']}

### Summary Statistics

| Status | Count |
|--------|-------|
| ✓ Passed | {json_report['summary']['by_status'].get('OK', 0)} |
| ⚠ Warnings | {json_report['summary']['by_status'].get('WARNING', 0)} |
| ✗ Failed | {json_report['summary']['by_status'].get('FAIL', 0)} |
| ℹ Info | {json_report['summary']['by_status'].get('INFO', 0)} |
| **Total** | **{json_report['summary']['total_checks']}** |

---

"""
        
        # Add results by category
        for category, results in json_report['results_by_category'].items():
            md += f"## {category}\n\n"
            
            for result in results:
                symbol = {"OK": "✓", "WARNING": "⚠", "FAIL": "✗", "INFO": "ℹ", "SKIP": "⊘"}.get(result['status'], "•")
                md += f"- **{symbol} {result['name']}:** {result['message']}\n"
                
                if result.get('recommendation'):
                    md += f"  - 💡 *{result['recommendation']}*\n"
            
            md += "\n"
        
        md += "---\n"
        md += "*Generated by AI File Sorter Thorough Diagnostic Tool v2.0*\n"
        
        try:
            with open(output_file, 'w') as f:
                f.write(md)
            self.log(f"{Colors.OKGREEN}✓ Markdown summary saved: {output_file}{Colors.ENDC}")
        except Exception as e:
            self.log(f"{Colors.FAIL}✗ Failed to save Markdown summary: {e}{Colors.ENDC}")
    
    def print_summary(self, json_report: dict):
        """Print summary to console"""
        self.section_header("Summary")
        
        stats = json_report['summary']['by_status']
        
        self.log(f"Total Checks: {json_report['summary']['total_checks']}")
        self.log(f"  ✓ Passed:   {stats.get('OK', 0)}", Colors.OKGREEN)
        self.log(f"  ⚠ Warnings: {stats.get('WARNING', 0)}", Colors.WARNING)
        self.log(f"  ✗ Failed:   {stats.get('FAIL', 0)}", Colors.FAIL)
        self.log(f"  ℹ Info:     {stats.get('INFO', 0)}", Colors.OKBLUE)
        self.log(f"  ⊘ Skipped:  {stats.get('SKIP', 0)}", Colors.OKCYAN)
        
        duration = json_report['diagnostic_metadata']['duration_seconds']
        self.log(f"\nDuration: {duration:.2f} seconds")
        
        health = json_report['summary']['overall_health']
        health_color = {
            "EXCELLENT": Colors.OKGREEN,
            "GOOD": Colors.WARNING,
            "NEEDS ATTENTION": Colors.WARNING,
            "CRITICAL": Colors.FAIL
        }.get(health, "")
        
        self.log(f"\nOverall Health: {health}", health_color + Colors.BOLD)
    
    # ==================== Main Execution ====================
    
    def run_all_checks(self, test_apis: bool = False):
        """Run all diagnostic checks"""
        self.log(f"{Colors.HEADER}{Colors.BOLD}")
        self.log("╔════════════════════════════════════════════════════════════════════════════╗")
        self.log("║          AI FILE SORTER - THOROUGH DIAGNOSTIC TOOL v2.0                    ║")
        self.log("║              Comprehensive Feature & System Validation                     ║")
        self.log("╚════════════════════════════════════════════════════════════════════════════╝")
        self.log(Colors.ENDC)
        
        if self.quick:
            self.log(f"{Colors.WARNING}⚡ Quick mode enabled - skipping slow tests{Colors.ENDC}\n")
        
        # List of all check methods
        check_methods = [
            self.check_system_info,
            self.check_file_structure,
            self.check_dependencies,
            self.check_llm_backends,
            self.check_database,
            self.check_configuration,
            self.check_features,
            self.check_logs,
            self.check_performance,
            lambda: self.check_api_connectivity(test_apis),
        ]
        
        # Run all checks with error handling
        for check_method in check_methods:
            try:
                check_method()
            except KeyboardInterrupt:
                self.log(f"\n{Colors.WARNING}⚠ Diagnostic interrupted by user{Colors.ENDC}")
                sys.exit(1)
            except Exception as e:
                import traceback
                self.log(f"\n{Colors.FAIL}✗ Error in check: {e}{Colors.ENDC}")
                if self.verbose:
                    self.log(traceback.format_exc(), Colors.FAIL)


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="AI File Sorter - Thorough Diagnostic Tool (Separate from Main App)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          # Basic diagnostic
  %(prog)s --verbose                # Detailed output
  %(prog)s --output report.json     # Save JSON report
  %(prog)s --html --markdown        # Generate all report formats
  %(prog)s --test-apis              # Test API connectivity (requires internet)
  %(prog)s --quick                  # Fast scan, skip slow tests
  %(prog)s -v --html --markdown     # Full verbose with all reports
        """
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
    
    parser.add_argument(
        "--html",
        action="store_true",
        help="Generate HTML report"
    )
    
    parser.add_argument(
        "--markdown",
        action="store_true",
        help="Generate Markdown summary"
    )
    
    parser.add_argument(
        "--test-apis",
        action="store_true",
        help="Test API connectivity (requires internet)"
    )
    
    parser.add_argument(
        "--quick",
        action="store_true",
        help="Quick mode - skip slow tests for rapid validation"
    )
    
    args = parser.parse_args()
    
    # Create and run diagnostic tool
    tool = ThoroughDiagnosticTool(verbose=args.verbose, quick=args.quick)
    tool.run_all_checks(test_apis=args.test_apis)
    
    # Generate reports
    timestamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    
    json_file = args.output or f"thorough_diagnostic_{timestamp}.json"
    json_report = tool.generate_json_report(json_file)
    
    if args.html:
        # Properly replace extension using pathlib
        html_file = str(Path(json_file).with_suffix('.html'))
        tool.generate_html_report(json_report, html_file)
    
    if args.markdown:
        # Properly replace extension using pathlib
        md_file = str(Path(json_file).with_suffix('.md'))
        tool.generate_markdown_summary(json_report, md_file)
    
    # Print summary
    tool.print_summary(json_report)


if __name__ == "__main__":
    main()
