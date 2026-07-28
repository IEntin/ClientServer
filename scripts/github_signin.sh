#!/bin/bash

# Define your details
GH_USER="IEntin437"
GH_EMAIL="ilya.entin@gmail.com"
REPO_URL="https://github.com/IEntin/ClientServer"

echo "=========================================="
echo " Starting GitHub Sign-In Configuration"
echo "=========================================="

# 1. Prompt for your Personal Access Token
echo -n "Please paste your GitHub Personal Access Token (PAT): "
read -s PAT
echo -e "\nToken received.\n"

# 2. Configure global Git identity
echo "Setting global Git user and email..."
git config --global user.name "$GH_USER"
git config --global user.email "$GH_EMAIL"

# 3. Enable Git credential helper storage
echo "Enabling credential helper store..."
git config --global credential.helper store

# 4. Write credentials securely to the disk
echo "Saving credentials to ~/.git-credentials..."
echo "https://${GH_USER}:${PAT}@github.com" >> ~/.git-credentials
chmod 600 ~/.git-credentials

# 5. Execute the explicit "Sign-In" check
echo "Executing remote connection check to verify login..."
echo "----------------------------------------------------"
git ls-remote "$REPO_URL"

if [ $? -eq 0 ]; then
    echo "----------------------------------------------------"
    echo "SUCCESS: Your Linux Mint machine is now signed in!"
    echo "You can pull, push, and clone without password prompts."
else
    echo "----------------------------------------------------"
    echo "ERROR: Sign-in failed. Please verify your PAT validity and scopes."
fi
echo "=========================================="

# gh auth login
# git credential-manager clear
