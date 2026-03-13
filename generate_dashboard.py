import os, sys, subprocess

def run_pipeline(project_name):
    print(f"🚀 Generating Dashboard for: {project_name}")
    
    project_path = os.path.join(os.getcwd(), project_name)
    reports_dir = os.path.join(project_path, "build_host", "Reports")
    os.makedirs(reports_dir, exist_ok=True)
    
    # 1. Convert GTest XML to HTML (JUnit2HTML must be installed)
    xml_file = os.path.join(reports_dir, "unit_test_results.xml")
    html_file = os.path.join(reports_dir, "test_report.html")
    if os.path.exists(xml_file):
        subprocess.run(["junit2html", xml_file, html_file])

    # 2. Generate the Index Dashboard
    dashboard_html = os.path.join(reports_dir, "index.html")
    with open(dashboard_html, "w") as f:
        f.write(f"""
        <html>
        <body style="font-family: sans-serif; padding: 20px;">
            <h2>{project_name} - Detailed Quality Reports</h2>
            <hr>
            <ul>
                <li><strong>Unit Tests:</strong> <a href="test_report.html" target="_blank">View GoogleTest Results</a></li>
                <li><strong>Code Coverage:</strong> <a href="coverage/index.html" target="_blank">View Line-by-Line Heat Map</a></li>
            </ul>
        </body>
        </html>
        """)
    print(f"✅ Dashboard generated at: {dashboard_html}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        run_pipeline(sys.argv[1])
    else:
        print("Please provide a project name. Example: python3 generate_dashboard.py 04_FreeRTOS_Cpp")