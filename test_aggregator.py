#!/usr/bin/env python3
"""
AI File Sorter - Test Result Aggregator

Aggregates and analyzes multiple diagnostic reports to track health over time.

Usage:
    python3 test_aggregator.py [report1.json report2.json ...]
    python3 test_aggregator.py --directory diagnostics/ --output summary.html
"""

import json
import sys
import argparse
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Any
from collections import defaultdict

class TestAggregator:
    """Aggregates and analyzes multiple test reports"""
    
    def __init__(self):
        self.reports: List[Dict[str, Any]] = []
        self.summary = defaultdict(list)
    
    def load_report(self, file_path: str) -> bool:
        """Load a diagnostic report JSON file"""
        try:
            with open(file_path, 'r') as f:
                report = json.load(f)
                self.reports.append({
                    'file': file_path,
                    'data': report
                })
                return True
        except Exception as e:
            print(f"Error loading {file_path}: {e}")
            return False
    
    def load_directory(self, directory: str) -> int:
        """Load all JSON reports from a directory"""
        count = 0
        for json_file in Path(directory).glob("*.json"):
            if self.load_report(str(json_file)):
                count += 1
        return count
    
    def analyze(self):
        """Analyze loaded reports"""
        if not self.reports:
            print("No reports loaded")
            return
        
        print("\n" + "="*80)
        print("TEST RESULT AGGREGATION AND ANALYSIS")
        print("="*80)
        
        # Sort by timestamp
        self.reports.sort(key=lambda r: r['data'].get('timestamp', ''))
        
        print(f"\nTotal Reports: {len(self.reports)}")
        print(f"Date Range: {self._get_date_range()}")
        
        self._analyze_health_trend()
        self._analyze_common_issues()
        self._analyze_performance()
        self._analyze_feature_status()
    
    def _get_date_range(self) -> str:
        """Get date range of reports"""
        if not self.reports:
            return "N/A"
        
        dates = [r['data'].get('timestamp', '') for r in self.reports if r['data'].get('timestamp')]
        if not dates:
            return "N/A"
        
        first = datetime.fromisoformat(dates[0]).strftime('%Y-%m-%d %H:%M')
        last = datetime.fromisoformat(dates[-1]).strftime('%Y-%m-%d %H:%M')
        
        return f"{first} to {last}"
    
    def _analyze_health_trend(self):
        """Analyze overall health trend"""
        print("\n" + "-"*80)
        print("HEALTH TREND")
        print("-"*80)
        
        for i, report in enumerate(self.reports, 1):
            data = report['data']
            summary = data.get('summary', {})
            timestamp = data.get('timestamp', 'Unknown')
            health = summary.get('health', 'Unknown')
            
            ok = summary.get('ok', 0)
            warn = summary.get('warning', 0)
            fail = summary.get('fail', 0)
            total = summary.get('total', 0)
            
            # Health indicator
            if health == 'EXCELLENT':
                indicator = '🟢'
            elif health == 'GOOD':
                indicator = '🟡'
            elif health == 'NEEDS ATTENTION':
                indicator = '🟠'
            else:
                indicator = '🔴'
            
            print(f"{i}. {timestamp[:19]} {indicator} {health}")
            print(f"   OK: {ok}/{total}, Warnings: {warn}, Failures: {fail}")
        
        # Trend analysis
        if len(self.reports) > 1:
            first_health = self.reports[0]['data'].get('summary', {}).get('health', '')
            last_health = self.reports[-1]['data'].get('summary', {}).get('health', '')
            
            print(f"\nTrend: {first_health} → {last_health}")
            
            if last_health == first_health:
                print("Status: Stable")
            elif self._health_score(last_health) > self._health_score(first_health):
                print("Status: Improving ✅")
            else:
                print("Status: Degrading ⚠️")
    
    def _health_score(self, health: str) -> int:
        """Convert health status to numeric score"""
        scores = {
            'EXCELLENT': 4,
            'GOOD': 3,
            'NEEDS ATTENTION': 2,
            'CRITICAL': 1
        }
        return scores.get(health, 0)
    
    def _analyze_common_issues(self):
        """Analyze common issues across reports"""
        print("\n" + "-"*80)
        print("COMMON ISSUES")
        print("-"*80)
        
        # Count warnings and failures by name
        warnings = defaultdict(int)
        failures = defaultdict(int)
        
        for report in self.reports:
            results = report['data'].get('results', [])
            for result in results:
                if result.get('status') == 'WARNING':
                    warnings[result.get('name', 'Unknown')] += 1
                elif result.get('status') == 'FAIL':
                    failures[result.get('name', 'Unknown')] += 1
        
        if failures:
            print("\nMost Common Failures:")
            for issue, count in sorted(failures.items(), key=lambda x: -x[1])[:5]:
                pct = (count / len(self.reports)) * 100
                print(f"  • {issue}: {count}/{len(self.reports)} reports ({pct:.1f}%)")
        else:
            print("\n✅ No recurring failures")
        
        if warnings:
            print("\nMost Common Warnings:")
            for issue, count in sorted(warnings.items(), key=lambda x: -x[1])[:5]:
                pct = (count / len(self.reports)) * 100
                print(f"  • {issue}: {count}/{len(self.reports)} reports ({pct:.1f}%)")
        else:
            print("\n✅ No recurring warnings")
    
    def _analyze_performance(self):
        """Analyze performance metrics"""
        print("\n" + "-"*80)
        print("PERFORMANCE METRICS")
        print("-"*80)
        
        durations = [r['data'].get('duration_seconds', 0) for r in self.reports]
        
        if durations:
            avg_duration = sum(durations) / len(durations)
            min_duration = min(durations)
            max_duration = max(durations)
            
            print(f"\nDiagnostic Run Time:")
            print(f"  Average: {avg_duration:.2f}s")
            print(f"  Min: {min_duration:.2f}s")
            print(f"  Max: {max_duration:.2f}s")
        
        # Check for performance trends
        if len(durations) > 1:
            first_half = durations[:len(durations)//2]
            second_half = durations[len(durations)//2:]
            
            avg_first = sum(first_half) / len(first_half)
            avg_second = sum(second_half) / len(second_half)
            
            if avg_second < avg_first * 0.9:
                print("\n✅ Performance improving over time")
            elif avg_second > avg_first * 1.1:
                print("\n⚠️ Performance degrading over time")
            else:
                print("\n→ Performance stable")
    
    def _analyze_feature_status(self):
        """Analyze feature status across reports"""
        print("\n" + "-"*80)
        print("FEATURE STATUS")
        print("-"*80)
        
        # Find features that were recently fixed or broken
        if len(self.reports) < 2:
            print("\nNeed at least 2 reports for trend analysis")
            return
        
        latest = self.reports[-1]['data'].get('results', [])
        previous = self.reports[-2]['data'].get('results', [])
        
        # Create lookup by name
        latest_lookup = {r['name']: r['status'] for r in latest}
        previous_lookup = {r['name']: r['status'] for r in previous}
        
        improved = []
        degraded = []
        
        for name in latest_lookup:
            if name not in previous_lookup:
                continue
            
            prev_status = previous_lookup[name]
            curr_status = latest_lookup[name]
            
            # Check if status changed
            if prev_status != curr_status:
                if self._status_score(curr_status) > self._status_score(prev_status):
                    improved.append(name)
                else:
                    degraded.append(name)
        
        if improved:
            print("\n✅ Recently Improved:")
            for name in improved:
                print(f"  • {name}: {previous_lookup[name]} → {latest_lookup[name]}")
        
        if degraded:
            print("\n⚠️ Recently Degraded:")
            for name in degraded:
                print(f"  • {name}: {previous_lookup[name]} → {latest_lookup[name]}")
        
        if not improved and not degraded:
            print("\n→ No status changes in latest report")
    
    def _status_score(self, status: str) -> int:
        """Convert status to numeric score"""
        scores = {
            'OK': 4,
            'INFO': 3,
            'WARNING': 2,
            'FAIL': 1
        }
        return scores.get(status, 0)
    
    def generate_html_report(self, output_file: str):
        """Generate HTML summary report"""
        html = f"""
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>AI File Sorter - Test Report Summary</title>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
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
        .summary {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }}
        .card {{
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .card h3 {{
            margin-top: 0;
            color: #333;
        }}
        .status {{
            display: inline-block;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 12px;
            font-weight: bold;
        }}
        .status-ok {{ background: #4caf50; color: white; }}
        .status-warning {{ background: #ff9800; color: white; }}
        .status-fail {{ background: #f44336; color: white; }}
        .status-info {{ background: #2196f3; color: white; }}
        .status-excellent {{ background: #4caf50; color: white; }}
        .status-good {{ background: #8bc34a; color: white; }}
        .status-needs-attention {{ background: #ff9800; color: white; }}
        .status-critical {{ background: #f44336; color: white; }}
        table {{
            width: 100%;
            border-collapse: collapse;
            background: white;
            margin-bottom: 20px;
        }}
        th, td {{
            text-align: left;
            padding: 12px;
            border-bottom: 1px solid #ddd;
        }}
        th {{
            background: #667eea;
            color: white;
        }}
        .trend-up {{ color: #4caf50; }}
        .trend-down {{ color: #f44336; }}
        .trend-stable {{ color: #757575; }}
    </style>
</head>
<body>
    <div class="header">
        <h1>AI File Sorter - Test Report Summary</h1>
        <p>Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        <p>Reports Analyzed: {len(self.reports)}</p>
        <p>Date Range: {self._get_date_range()}</p>
    </div>
"""
        
        # Summary cards
        if self.reports:
            latest = self.reports[-1]['data'].get('summary', {})
            html += """
    <div class="summary">
        <div class="card">
            <h3>Overall Health</h3>
            <h2>{}</h2>
        </div>
        <div class="card">
            <h3>Passed Checks</h3>
            <h2>{}/{}</h2>
        </div>
        <div class="card">
            <h3>Warnings</h3>
            <h2>{}</h2>
        </div>
        <div class="card">
            <h3>Failures</h3>
            <h2>{}</h2>
        </div>
    </div>
""".format(
                latest.get('health', 'Unknown'),
                latest.get('ok', 0),
                latest.get('total', 0),
                latest.get('warning', 0),
                latest.get('fail', 0)
            )
        
        # Health trend table
        html += """
    <div class="card">
        <h3>Health Trend</h3>
        <table>
            <tr>
                <th>Date</th>
                <th>Health</th>
                <th>OK</th>
                <th>Warnings</th>
                <th>Failures</th>
                <th>Total</th>
            </tr>
"""
        
        for report in self.reports:
            data = report['data']
            summary = data.get('summary', {})
            timestamp = data.get('timestamp', 'Unknown')[:19]
            health = summary.get('health', 'Unknown')
            
            html += f"""
            <tr>
                <td>{timestamp}</td>
                <td><span class="status status-{health.lower().replace(' ', '-')}">{health}</span></td>
                <td>{summary.get('ok', 0)}</td>
                <td>{summary.get('warning', 0)}</td>
                <td>{summary.get('fail', 0)}</td>
                <td>{summary.get('total', 0)}</td>
            </tr>
"""
        
        html += """
        </table>
    </div>
"""
        
        html += """
</body>
</html>
"""
        
        with open(output_file, 'w') as f:
            f.write(html)
        
        print(f"\n✅ HTML report saved to: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description="AI File Sorter - Test Result Aggregator"
    )
    parser.add_argument(
        'reports',
        nargs='*',
        help='JSON report files to analyze'
    )
    parser.add_argument(
        '-d', '--directory',
        help='Directory containing JSON reports'
    )
    parser.add_argument(
        '-o', '--output',
        help='Output HTML report file'
    )
    
    args = parser.parse_args()
    
    aggregator = TestAggregator()
    
    # Load reports
    if args.directory:
        count = aggregator.load_directory(args.directory)
        print(f"Loaded {count} reports from {args.directory}")
    
    for report_file in args.reports:
        aggregator.load_report(report_file)
    
    if not aggregator.reports:
        print("No reports loaded. Specify report files or use --directory")
        sys.exit(1)
    
    # Analyze
    aggregator.analyze()
    
    # Generate HTML report
    if args.output:
        aggregator.generate_html_report(args.output)


if __name__ == "__main__":
    main()
