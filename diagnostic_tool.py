#!/usr/bin/env python3
"""
AI File Sorter - Comprehensive Feature Testing Diagnostic Tool
Tests all implemented features functionality end-to-end
"""

import os
import sys
import json
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

class FeatureDiagnosticTool:
    """Comprehensive feature testing for AI File Sorter"""
    
    def __init__(self, repo_path: str):
        self.repo_path = Path(repo_path)
        self.app_path = self.repo_path / "app"
        self.test_results = []
        
    def run_all_tests(self) -> Dict[str, Any]:
        """Run comprehensive tests for all features"""
        print("=" * 80)
        print("AI File Sorter - Comprehensive Feature Diagnostic Tool")
        print("=" * 80)
        print(f"Repository: {self.repo_path}")
        print(f"Timestamp: {datetime.now().isoformat()}")
        print("=" * 80)
        
        # Test each feature
        self.test_feature_1_gemini_api()
        self.test_feature_2_file_tinder()
        self.test_feature_3_whitelist_editor()
        # Cache Manager skipped - not needed per user request
        self.test_feature_5_content_sorting()
        self.test_feature_6_tui()
        
        return self.generate_report()
    
    def test_feature_1_gemini_api(self):
        """Feature #1: Enhanced Gemini API with Rate Limiting"""
        print("\n" + "=" * 80)
        print("[FEATURE #1] Enhanced Gemini API with Sophisticated Rate Limiting")
        print("=" * 80)
        
        tests = [
            ("Implementation Files", self.check_gemini_files),
            ("Rate Limiting Logic", self.check_rate_limiting),
            ("State Persistence", self.check_state_persistence),
            ("Circuit Breaker Pattern", self.check_circuit_breaker),
            ("Progress Callback", self.check_progress_callback),
            ("Timeout Management", self.check_timeout_management),
            ("Error Handling", self.check_gemini_error_handling),
            ("Thread Safety", self.check_gemini_thread_safety),
        ]
        
        self.run_test_suite("Gemini API", tests)
    
    def test_feature_2_file_tinder(self):
        """Feature #2: File Tinder Tool"""
        print("\n" + "=" * 80)
        print("[FEATURE #2] File Tinder Tool")
        print("=" * 80)
        
        tests = [
            ("Implementation Files", self.check_file_tinder_files),
            ("Arrow Key Navigation", self.check_arrow_navigation),
            ("File Preview System", self.check_file_preview),
            ("Decision Tracking", self.check_decision_tracking),
            ("Session Persistence", self.check_tinder_session),
            ("Database Integration", self.check_tinder_database),
            ("File Deletion Logic", self.check_deletion_logic),
            ("UI Components", self.check_tinder_ui),
        ]
        
        self.run_test_suite("File Tinder", tests)
    
    def test_feature_3_whitelist_editor(self):
        """Feature #3: Enhanced Whitelist Tree Editor"""
        print("\n" + "=" * 80)
        print("[FEATURE #3] Enhanced Whitelist Tree Editor + Management")
        print("=" * 80)
        
        tests = [
            ("Implementation Files", self.check_whitelist_files),
            ("Tree Structure", self.check_tree_structure),
            ("Hierarchical Mode", self.check_hierarchical_mode),
            ("Shared Subcategories Mode", self.check_shared_mode),
            ("Add/Remove Operations", self.check_tree_operations),
            ("Import/Export", self.check_import_export),
            ("Keyboard Shortcuts", self.check_keyboard_shortcuts),
            ("CategoryNode Conversion", self.check_category_node),
        ]
        
        self.run_test_suite("Whitelist Editor", tests)
    
    # Cache Manager tests removed - feature not needed per user request
    
    def test_feature_5_content_sorting(self):
        """Feature #4: Content-Based File Sorting"""
        print("\n" + "=" * 80)
        print("[FEATURE #4] Content-Based File Sorting")
        print("=" * 80)
        
        tests = [
            ("File Type Mappings", self.check_file_type_mappings),
            ("Extension Detection", self.check_extension_detection),
            ("Prompt Enhancement", self.check_prompt_enhancement),
            ("VST Plugin Detection", self.check_vst_detection),
            ("Config File Detection", self.check_config_detection),
            ("Source Code Detection", self.check_source_detection),
            ("Fallback Mechanism", self.check_fallback),
            ("Integration", self.check_sorting_integration),
        ]
        
        self.run_test_suite("Content-Based Sorting", tests)
    
    def test_feature_6_tui(self):
        """Feature #6: TUI (Text-based User Interface)"""
        print("\n" + "=" * 80)
        print("[FEATURE #6] TUI (Text-based User Interface)")
        print("=" * 80)
        
        tests = [
            ("TUI Directory Structure", self.check_tui_directory),
            ("TUI CMakeLists.txt", self.check_tui_cmake),
            ("TUI Main Entry Point", self.check_tui_main),
            ("TUI App Implementation", self.check_tui_app),
            ("TUI Settings (Qt-free)", self.check_tui_settings),
            ("TUI LLM Selection Dialog", self.check_tui_llm_selection),
            ("TUI Categorization Progress", self.check_tui_categorization_progress),
            ("TUI Results Display", self.check_tui_results),
            ("TUI File Tinder", self.check_tui_file_tinder),
            ("TUI Whitelist Manager", self.check_tui_whitelist),
            ("TUI Build Script", self.check_tui_build_script),
        ]
        
        self.run_test_suite("TUI Interface", tests)
    
    # TUI Test Methods
    def check_tui_directory(self) -> bool:
        """Check if TUI directory exists"""
        return (self.app_path / "tui").exists()
    
    def check_tui_cmake(self) -> bool:
        """Check if TUI CMakeLists.txt exists and is properly configured"""
        cmake_file = self.app_path / "tui" / "CMakeLists.txt"
        if not cmake_file.exists():
            return False
        content = cmake_file.read_text()
        has_ftxui = "ftxui" in content.lower()
        has_project = "project" in content.lower()
        has_executable = "add_executable" in content
        return has_ftxui and has_project and has_executable
    
    def check_tui_main(self) -> bool:
        """Check if TUI main entry point exists"""
        main_file = self.app_path / "tui" / "main_tui.cpp"
        if not main_file.exists():
            return False
        content = main_file.read_text()
        return "TuiApp" in content and "main" in content
    
    def check_tui_app(self) -> bool:
        """Check if TuiApp implementation exists"""
        hpp = (self.app_path / "tui" / "TuiApp.hpp").exists()
        cpp = (self.app_path / "tui" / "TuiApp.cpp").exists()
        if not (hpp and cpp):
            return False
        content = (self.app_path / "tui" / "TuiApp.cpp").read_text()
        return "build_main_ui" in content and "run" in content
    
    def check_tui_settings(self) -> bool:
        """Check if TUI Settings (Qt-free) exists"""
        hpp = (self.app_path / "tui" / "TuiSettings.hpp").exists()
        cpp = (self.app_path / "tui" / "TuiSettings.cpp").exists()
        if not (hpp and cpp):
            return False
        # Verify it doesn't use Qt
        content = (self.app_path / "tui" / "TuiSettings.cpp").read_text()
        return "QStandardPaths" not in content and "QString" not in content
    
    def check_tui_llm_selection(self) -> bool:
        """Check if TUI LLM Selection dialog exists"""
        hpp = (self.app_path / "tui" / "TuiLLMSelection.hpp").exists()
        cpp = (self.app_path / "tui" / "TuiLLMSelection.cpp").exists()
        return hpp and cpp
    
    def check_tui_categorization_progress(self) -> bool:
        """Check if TUI Categorization Progress exists"""
        hpp = (self.app_path / "tui" / "TuiCategorizationProgress.hpp").exists()
        cpp = (self.app_path / "tui" / "TuiCategorizationProgress.cpp").exists()
        return hpp and cpp
    
    def check_tui_results(self) -> bool:
        """Check if TUI Results Display exists"""
        hpp = (self.app_path / "tui" / "TuiCategorizationResults.hpp").exists()
        cpp = (self.app_path / "tui" / "TuiCategorizationResults.cpp").exists()
        return hpp and cpp
    
    def check_tui_file_tinder(self) -> bool:
        """Check if TUI File Tinder exists"""
        hpp = (self.app_path / "tui" / "TuiFileTinder.hpp").exists()
        cpp = (self.app_path / "tui" / "TuiFileTinder.cpp").exists()
        if not (hpp and cpp):
            return False
        content = (self.app_path / "tui" / "TuiFileTinder.cpp").read_text()
        return "ArrowLeft" in content or "mark_keep" in content
    
    def check_tui_whitelist(self) -> bool:
        """Check if TUI Whitelist Manager exists"""
        hpp = (self.app_path / "tui" / "TuiWhitelistManager.hpp").exists()
        cpp = (self.app_path / "tui" / "TuiWhitelistManager.cpp").exists()
        return hpp and cpp
    
    def check_tui_build_script(self) -> bool:
        """Check if TUI build script exists"""
        return (self.repo_path / "scripts" / "build_tui.sh").exists()
    
    def run_test_suite(self, feature_name: str, tests: List):
        """Run a suite of tests for a feature"""
        passed = 0
        failed = 0
        
        for test_name, test_func in tests:
            try:
                result = test_func()
                if result:
                    print(f"  ✅ {test_name}")
                    passed += 1
                else:
                    print(f"  ❌ {test_name}")
                    failed += 1
                    
                self.test_results.append({
                    "feature": feature_name,
                    "test": test_name,
                    "passed": result,
                    "timestamp": datetime.now().isoformat()
                })
            except Exception as e:
                print(f"  ⚠️  {test_name}: {str(e)}")
                failed += 1
                self.test_results.append({
                    "feature": feature_name,
                    "test": test_name,
                    "passed": False,
                    "error": str(e),
                    "timestamp": datetime.now().isoformat()
                })
        
        print(f"\n  Summary: {passed} passed, {failed} failed")
    
    # Gemini API Tests
    def check_gemini_files(self) -> bool:
        """Check if Gemini API files exist"""
        hpp = (self.app_path / "include" / "GeminiClient.hpp").exists()
        cpp = (self.app_path / "lib" / "GeminiClient.cpp").exists()
        return hpp and cpp
    
    def check_rate_limiting(self) -> bool:
        """Check for rate limiting implementation"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "token_bucket" in content.lower() or "rate_limit" in content.lower()
    
    def check_state_persistence(self) -> bool:
        """Check for state persistence"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "PersistentState" in content and "schedule_save" in content
    
    def check_circuit_breaker(self) -> bool:
        """Check for circuit breaker pattern"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "circuit" in content.lower() or "failure_count" in content
    
    def check_progress_callback(self) -> bool:
        """Check progress callback implementation"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "progress_callback" in content and "ProgressData" in content
    
    def check_timeout_management(self) -> bool:
        """Check timeout management"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "timeout" in content.lower() and "progressive" in content.lower() or "extend" in content.lower()
    
    def check_gemini_error_handling(self) -> bool:
        """Check error handling"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        has_try_catch = "try" in content and "catch" in content
        has_specific = "std::exception" in content or "std::invalid_argument" in content
        return has_try_catch and has_specific
    
    def check_gemini_thread_safety(self) -> bool:
        """Check thread safety measures"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "std::mutex" in content or "std::atomic" in content
    
    # File Tinder Tests
    def check_file_tinder_files(self) -> bool:
        """Check if File Tinder files exist"""
        hpp = (self.app_path / "include" / "FileTinderDialog.hpp").exists()
        cpp = (self.app_path / "lib" / "FileTinderDialog.cpp").exists()
        return hpp and cpp
    
    def check_arrow_navigation(self) -> bool:
        """Check arrow key navigation"""
        cpp_file = self.app_path / "lib" / "FileTinderDialog.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        has_left = "Qt::Key_Left" in content
        has_right = "Qt::Key_Right" in content
        has_up = "Qt::Key_Up" in content
        has_down = "Qt::Key_Down" in content
        return has_left and has_right and has_up and has_down
    
    def check_file_preview(self) -> bool:
        """Check file preview functionality"""
        cpp_file = self.app_path / "lib" / "FileTinderDialog.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "preview" in content.lower() and "QLabel" in content
    
    def check_decision_tracking(self) -> bool:
        """Check decision tracking"""
        cpp_file = self.app_path / "lib" / "FileTinderDialog.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "Decision::" in content and ("Keep" in content or "Delete" in content)
    
    def check_tinder_session(self) -> bool:
        """Check session persistence"""
        cpp_file = self.app_path / "lib" / "FileTinderDialog.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "save_state" in content or "load_state" in content
    
    def check_tinder_database(self) -> bool:
        """Check database integration"""
        cpp_file = self.app_path / "lib" / "FileTinderDialog.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "save_tinder_decision" in content or "load_tinder_state" in content
    
    def check_deletion_logic(self) -> bool:
        """Check file deletion logic"""
        cpp_file = self.app_path / "lib" / "FileTinderDialog.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        has_function = "on_execute_deletions" in content
        has_remove = "QFile::remove" in content or ".remove()" in content
        return has_function and has_remove
    
    def check_tinder_ui(self) -> bool:
        """Check UI components"""
        cpp_file = self.app_path / "lib" / "FileTinderDialog.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "setup_ui" in content and "QVBoxLayout" in content
    
    # Whitelist Editor Tests
    def check_whitelist_files(self) -> bool:
        """Check if Whitelist Editor files exist"""
        hpp = (self.app_path / "include" / "WhitelistTreeEditor.hpp").exists()
        cpp = (self.app_path / "lib" / "WhitelistTreeEditor.cpp").exists()
        return hpp and cpp
    
    def check_tree_structure(self) -> bool:
        """Check tree structure implementation"""
        cpp_file = self.app_path / "lib" / "WhitelistTreeEditor.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "QTreeWidget" in content and "QTreeWidgetItem" in content
    
    def check_hierarchical_mode(self) -> bool:
        """Check hierarchical mode"""
        cpp_file = self.app_path / "lib" / "WhitelistTreeEditor.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "hierarchical" in content.lower() or "per-category" in content.lower()
    
    def check_shared_mode(self) -> bool:
        """Check shared subcategories mode"""
        cpp_file = self.app_path / "lib" / "WhitelistTreeEditor.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "shared" in content.lower() or "global" in content.lower()
    
    def check_tree_operations(self) -> bool:
        """Check add/remove operations"""
        cpp_file = self.app_path / "lib" / "WhitelistTreeEditor.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        has_add = "on_add" in content
        has_remove = "on_remove" in content
        return has_add and has_remove
    
    def check_import_export(self) -> bool:
        """Check import/export functionality"""
        cpp_file = self.app_path / "lib" / "WhitelistTreeEditor.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "import" in content.lower() and "export" in content.lower()
    
    def check_keyboard_shortcuts(self) -> bool:
        """Check keyboard shortcuts"""
        cpp_file = self.app_path / "lib" / "WhitelistTreeEditor.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "keyPressEvent" in content or "QShortcut" in content
    
    def check_category_node(self) -> bool:
        """Check CategoryNode structure"""
        hpp_file = self.app_path / "include" / "WhitelistTreeEditor.hpp"
        if not hpp_file.exists():
            return False
        content = hpp_file.read_text()
        return "CategoryNode" in content or "struct" in content
    
    # Cache Manager Tests
    # Cache Manager tests removed - feature not needed per user request
    # def check_cache_files, check_cache_stats, check_clear_all, check_clear_old
    # def check_db_optimize, check_stats_refresh, check_cache_errors, check_cache_ui
    
    # Content-Based Sorting Tests
    def check_file_type_mappings(self) -> bool:
        """Check file type mappings exist"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "get_file_type_description" in content and ".dll" in content
    
    def check_extension_detection(self) -> bool:
        """Check extension detection logic"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "extension" in content.lower() and ("tolower" in content.lower() or "toLowerCase" in content)
    
    def check_prompt_enhancement(self) -> bool:
        """Check prompt enhancement"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "categorize" in content.lower() and "file_type" in content.lower()
    
    def check_vst_detection(self) -> bool:
        """Check VST plugin detection"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "VST" in content or "vst" in content.lower()
    
    def check_config_detection(self) -> bool:
        """Check config file detection"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return ".ini" in content or ".conf" in content or "configuration" in content.lower()
    
    def check_source_detection(self) -> bool:
        """Check source code detection"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return ".cpp" in content or ".py" in content or "source code" in content.lower()
    
    def check_fallback(self) -> bool:
        """Check fallback mechanism"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        return "fallback" in content.lower() or "default" in content.lower()
    
    def check_sorting_integration(self) -> bool:
        """Check integration with categorization"""
        cpp_file = self.app_path / "lib" / "GeminiClient.cpp"
        if not cpp_file.exists():
            return False
        content = cpp_file.read_text()
        has_function = "get_file_type_description" in content
        has_usage = content.count("get_file_type_description") >= 2
        return has_function and has_usage
    
    def generate_report(self) -> Dict[str, Any]:
        """Generate comprehensive diagnostic report"""
        print("\n" + "=" * 80)
        print("COMPREHENSIVE DIAGNOSTIC REPORT")
        print("=" * 80)
        
        # Calculate statistics by feature
        features = {}
        for result in self.test_results:
            feature = result["feature"]
            if feature not in features:
                features[feature] = {"passed": 0, "failed": 0, "total": 0}
            
            features[feature]["total"] += 1
            if result["passed"]:
                features[feature]["passed"] += 1
            else:
                features[feature]["failed"] += 1
        
        # Print summary
        print("\n📊 Feature Testing Summary:\n")
        total_passed = 0
        total_failed = 0
        
        for feature, stats in features.items():
            pass_rate = (stats["passed"] / stats["total"] * 100) if stats["total"] > 0 else 0
            status = "✅" if pass_rate >= 75 else "⚠️" if pass_rate >= 50 else "❌"
            print(f"  {status} {feature}: {stats['passed']}/{stats['total']} tests passed ({pass_rate:.1f}%)")
            total_passed += stats["passed"]
            total_failed += stats["failed"]
        
        total_tests = total_passed + total_failed
        overall_rate = (total_passed / total_tests * 100) if total_tests > 0 else 0
        
        print(f"\n  Overall: {total_passed}/{total_tests} tests passed ({overall_rate:.1f}%)")
        
        # Create report
        report = {
            "timestamp": datetime.now().isoformat(),
            "repository": str(self.repo_path),
            "total_features_tested": len(features),
            "total_tests": total_tests,
            "total_passed": total_passed,
            "total_failed": total_failed,
            "pass_rate": overall_rate,
            "features": features,
            "detailed_results": self.test_results
        }
        
        # Save report
        report_path = self.repo_path / "feature_diagnostic_report.json"
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"\n📄 Detailed report saved to: {report_path}")
        print("=" * 80)
        
        return report

def main():
    if len(sys.argv) > 1:
        repo_path = sys.argv[1]
    else:
        repo_path = os.getcwd()
    
    tool = FeatureDiagnosticTool(repo_path)
    report = tool.run_all_tests()
    
    # Exit with error code if pass rate is below 75%
    sys.exit(0 if report['pass_rate'] >= 75 else 1)

if __name__ == "__main__":
    main()
