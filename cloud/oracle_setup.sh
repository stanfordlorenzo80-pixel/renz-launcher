#!/bin/bash
#
# Renz WORM Proxy — Oracle Cloud Free Tier Setup
#
# Provisions a free ARM VM on Oracle Cloud (4 cores, 24GB RAM, always free).
# Installs the Python proxy + dependencies + runs as systemd service.
#
# Usage:
#   1. Sign up for Oracle Cloud free tier: https://cloud.oracle.com/
#   2. Create a VM.Standard.A1.Flex (4 OCPU, 24 GB RAM) — always free
#   3. SSH into the VM
#   4. Run this script: bash oracle_setup.sh
#   5. The proxy runs 24/7 on port 11435
#
# After setup, configure your clients to use:
#   http://<vm-public-ip>:11435/v1

set -e

echo "=== Renz WORM Proxy — Oracle Cloud Setup ==="
echo ""

# Update system
echo "[1/6] Updating system..."
if command -v apt-get &> /dev/null; then
    sudo apt-get update -y
    sudo apt-get install -y python3 python3-pip nginx
elif command -v yum &> /dev/null; then
    sudo yum update -y
    sudo yum install -y python3 python3-pip nginx
fi

# Install Python deps
echo "[2/6] Installing Python dependencies..."
pip3 install --user customtkinter || true

# Create user
echo "[3/6] Creating renz user..."
sudo useradd -m -s /bin/bash renz || echo "  user already exists"

# Create directory
echo "[4/6] Setting up /opt/renz..."
sudo mkdir -p /opt/renz
sudo chown renz:renz /opt/renz

# Copy files
echo "[5/6] Copying Renz files..."
sudo -u renz bash -c "
    cd /opt/renz
    cat > run_proxy.sh <<'EOF'
#!/bin/bash
cd /opt/renz
exec python3 proxy_server.py
EOF
    chmod +x run_proxy.sh
"

# Create systemd service
echo "[6/6] Creating systemd service..."
sudo tee /etc/systemd/system/renz-proxy.service > /dev/null <<'EOF'
[Unit]
Description=Renz WORM Universal Proxy
After=network.target

[Service]
Type=simple
User=renz
WorkingDirectory=/opt/renz
ExecStart=/opt/renz/run_proxy.sh
Restart=always
RestartSec=10
Environment="PYTHONUNBUFFERED=1"

[Install]
WantedBy=multi-user.target
EOF

# Reload systemd
sudo systemctl daemon-reload
sudo systemctl enable renz-proxy
sudo systemctl start renz-proxy

# Open firewall port 11435
echo ""
echo "=== Opening firewall port 11435 ==="
if command -v firewall-cmd &> /dev/null; then
    sudo firewall-cmd --permanent --add-port=11435/tcp
    sudo firewall-cmd --reload
elif command -v ufw &> /dev/null; then
    sudo ufw allow 11435/tcp
fi
# Oracle Cloud also has iptables-level firewall via Security Lists
echo "  -> also open port 11435 in Oracle Cloud Console → Networking → Security Lists"
echo "  -> and the VM's iptables (run: sudo iptables -I INPUT -p tcp --dport 11435 -j ACCEPT)"

# Print status
echo ""
echo "=== Status ==="
sudo systemctl status renz-proxy --no-pager
echo ""
echo "=== Proxy live at: ==="
echo "  http://$(curl -s ifconfig.me 2>/dev/null || echo '<vm-ip>'):11435/v1"
echo ""
echo "Test it:"
echo "  curl http://localhost:11435/v1/models"
echo ""
echo "Manage it:"
echo "  sudo systemctl status renz-proxy"
echo "  sudo systemctl restart renz-proxy"
echo "  sudo systemctl stop renz-proxy"
echo "  sudo journalctl -u renz-proxy -f"
