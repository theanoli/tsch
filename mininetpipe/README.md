## Synopsis

Stuff for transport protocol development and experiments.

## Installation

To install mTCP on a machine, run `sudo bash mtcp_setup.sh N`, where `N` is an integer that will become the last octet of that machine's IP address. Do not use the same number for two different machines at the same time!

This will configure/build/install both DPDK and mTCP.

For DPDK only, run `sudo bash dpdk-setup.sh`.

## Branches

The **master** branch is generally stable. The working branch is **devel**.
