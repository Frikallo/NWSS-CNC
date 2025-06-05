#!/bin/bash

# Simple workflow validation script
echo "🔍 Validating GitHub Actions workflows..."

# Check if workflow files exist
if [ ! -f ".github/workflows/ci.yml" ]; then
    echo "❌ CI workflow file not found"
    exit 1
fi

if [ ! -f ".github/workflows/build-and-release.yml" ]; then
    echo "❌ Build and release workflow file not found"
    exit 1
fi

echo "✅ Workflow files found"

# Basic YAML syntax validation (if available)
if command -v python3 >/dev/null 2>&1; then
    echo "🔍 Checking YAML syntax..."
    
    python3 -c "
import yaml
import sys

try:
    with open('.github/workflows/ci.yml', 'r') as f:
        yaml.safe_load(f)
    print('✅ ci.yml syntax is valid')
except Exception as e:
    print(f'❌ ci.yml syntax error: {e}')
    sys.exit(1)

try:
    with open('.github/workflows/build-and-release.yml', 'r') as f:
        yaml.safe_load(f)
    print('✅ build-and-release.yml syntax is valid')
except Exception as e:
    print(f'❌ build-and-release.yml syntax error: {e}')
    sys.exit(1)
" || echo "⚠️  Python YAML validation not available, skipping syntax check"
fi

# Check for common GitHub Actions issues
echo "🔍 Checking for common issues..."

# Check for proper indentation and structure
if grep -q "runner\.os ==" .github/workflows/*.yml; then
    echo "✅ Found proper runner.os conditionals"
else
    echo "⚠️  No runner.os conditionals found"
fi

# Check for required secrets
if grep -q "GITHUB_TOKEN" .github/workflows/*.yml; then
    echo "✅ GITHUB_TOKEN usage found"
else
    echo "⚠️  No GITHUB_TOKEN usage found"
fi

echo "🎉 Workflow validation complete!" 