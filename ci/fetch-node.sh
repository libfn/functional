#!/bin/bash
set -ex
set -o pipefail
: "${NODE_RELEASE:?NODE_RELEASE is not set}"

mkdir -p /etc/apt/keyrings
curl --proto '=https' --tlsv1.2 -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key \
  | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg
printf "%s\n%s\n" \
  "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_${NODE_RELEASE}.x nodistro main" \
  "deb-src [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_${NODE_RELEASE}.x nodistro main" \
  | tee /etc/apt/sources.list.d/nodesource.list
printf "%s\n%s\n%s\n" \
  "Package: nodejs" \
  "Pin: origin deb.nodesource.com" \
  "Pin-Priority: 600" \
  | tee /etc/apt/preferences.d/nodejs
apt-get update
apt-get install -y --no-install-recommends nodejs
