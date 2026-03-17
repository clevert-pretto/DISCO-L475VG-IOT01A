#!/bin/bash

# 1. Define required tools
REQUIRED_PYTHON_TOOLS=("pre-commit" "lizard" "junit2html" "plotly")

echo "🔧 Starting Workspace Bootstrap..."

# 2. Check for Python & Create Virtual Environment
if ! command -v python3 &> /dev/null; then
    echo "❌ Error: python3 is not installed."
    exit 1
fi

if [ ! -d ".venv" ]; then
    echo "📂 Creating .venv..."
    python3 -m venv .venv
fi

echo "📦 Syncing Python dependencies..."
./.venv/bin/python3 -m pip install --upgrade pip
./.venv/bin/python3 -m pip install "${REQUIRED_PYTHON_TOOLS[@]}"

# 3. Install Jekyll Locally (Ruby-based)
echo "💎 Checking Ruby environment..."
if ! command -v gem &> /dev/null; then
    echo "📥 Installing Ruby and build essentials..."
    sudo apt update && sudo apt install -y ruby-full build-essential zlib1g-dev
fi

# Robust PATH check: Only add to .bashrc if not already present in the file
if ! grep -q "GEM_HOME" "$HOME/.bashrc"; then
    echo "📝 Updating .bashrc with Ruby paths..."
    echo 'export GEM_HOME="$HOME/gems"' >> ~/.bashrc
    echo 'export PATH="$HOME/gems/bin:$PATH"' >> ~/.bashrc
fi

# Export for the current session so the rest of the script works immediately
export GEM_HOME="$HOME/gems"
export PATH="$HOME/gems/bin:$PATH"

echo "🚀 Installing Jekyll and Bundler..."
gem install jekyll bundler --no-document

# 4. Initialize Jekyll Project
if [ -d "docs" ]; then
    echo "📂 Syncing Jekyll gems in /docs..."
    cd docs
    # Use 'bundle config' to ensure it installs locally for the new user
    bundle config set --local path 'vendor/bundle'
    bundle install
    cd ..
else
    echo "⚠️ Warning: 'docs' folder not found."
fi

# 5. Initialize pre-commit with Custom Config Path
# We use the explicit path in build_utils/ so the root stays clean
if [ -f "build_utils/.pre-commit-config.yaml" ]; then
    echo "⚓ Initializing pre-commit hooks..."
    # Explicitly call the venv binary to ensure installation works without activation
    ./.venv/bin/pre-commit install --config build_utils/.pre-commit-config.yaml
    
    # Optional: Run an initial check to ensure everything is linked correctly
    echo "🔍 Running initial pre-commit check..."
    ./.venv/bin/pre-commit run --config build_utils/.pre-commit-config.yaml --all-files
else
    echo "⚠️ Skipping pre-commit: Config not found at build_utils/.pre-commit-config.yaml"
fi

echo "🎉 Workspace ready! Run 'source ~/.bashrc' to refresh your terminal."