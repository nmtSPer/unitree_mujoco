#!/usr/bin/env bash
set -euo pipefail

python3 -c "import numpy; print('numpy OK')"
python3 -c "import yaml; print('PyYAML OK')"
python3 -c "import unitree_sdk2py; print('unitree_sdk2py OK')"

echo "Python agent dependencies look OK"
