#!/bin/bash
# ======================================================
# Hypervisor Config Generation Script
# ------------------------------------------------------
# This script generates the hypervisor.config file
# required by the cernvm-webapi to pick the appropriate
# install targets of VirtualBox and VirtualBox extension
# packs.
# ======================================================

# Which version to install if no hypervisor is found
VERSION_ACTIVE="4.3.30"

# Make sure you keep the following list of versions up-to-date
VERSION_ALL="
5.0.0 5.0.2 5.0.4
4.3.30 4.3.28 4.3.26 4.3.24 4.3.22 4.3.20 4.3.18 4.3.16 4.3.14 4.3.12 4.3.10 4.3.8 4.3.6 4.3.4 4.3.2 4.3.0
4.2.26 4.2.24 4.2.22 4.2.20 4.2.18 4.2.16 4.2.14 4.2.12 4.2.10 4.2.8 4.2.6 4.2.4 4.2.2 4.2.0
4.1.34 4.1.32 4.1.30 4.1.28 4.1.26 4.1.24 4.1.22 4.1.20 4.1.18 4.1.16 4.1.14 4.1.12 4.1.10 4.1.8 4.1.6 4.1.4 4.1.2 4.1.0
4.0.26 4.0.24 4.0.22 4.0.20 4.0.18 4.0.16 4.0.14 4.0.12 4.0.10 4.0.8 4.0.6 4.0.4 4.0.2 4.0.0
"

# Mapping for different linux configurations not found
# on the checksum list, and specifically for RPM-based OSes
RPM_MAPPING_VERSIONS="
el6=scientificcernslc-carbon
el6=redhat-carbon
fedora18=fedora-spherical
fedora18=fedora-schrödiger's
fedora18=fedora-heisenbug
fedora17=fedora-beefy
el6=scientificcernslc-carbon)
el6=redhat-carbon)
fedora18=fedora-spherical)
fedora18=fedora-schrödiger's)
fedora18=fedora-heisenbug)
fedora17=fedora-beefy)
"

DEB_MAPPING_VERSIONS="
wheezy=debian-jessie
raring=ubuntu-saucy
trusty=ubuntu-trusty
trusty=ubuntu-utopic
trusty=ubuntu-vivid
"

# Configuration for VirtualBox
DOWNLOAD_URL="http://download.virtualbox.org/virtualbox"
HASH_URL="https://www.virtualbox.org/download/hashes"
HASH_FILE="SHA256SUMS"

# -------------------------
# Parse the checksums file and generate the vbox-extpack
# specific information for the hypervisor config
# -------------------------
function parse_extpack {
	local VER="$1"
	local LINE=$(cat | grep .vbox-extpack | head -n1)
	local P_CHECKSUM=$(echo "${LINE}" | awk -F' ' '{print $1}')
	local P_FILE=$(echo "${LINE}" | awk -F' ' '{print $2}' | tr -d '*')

	echo "vbox-${VER}-extpack=${DOWNLOAD_URL}/${VER}/${P_FILE}"
	echo "vbox-${VER}-extpackChecksum=${P_CHECKSUM}"

}

# -------------------------
# Parse the checksums file and generate the operating system-specific
# information for the hypervisor installer.
# -------------------------
function parse_os_versions {
	local VER="$1"
	while read LINE; do
		local P_CHECKSUM=$(echo "${LINE}" | awk -F' ' '{print $1}')
		local P_FILE=$(echo "${LINE}" | awk -F' ' '{print $2}' | tr -d '*')

		case $P_FILE in

			*.dmg)
				# OSX Installer
				echo "osx=${DOWNLOAD_URL}/${VER}/${P_FILE}"
				echo "osx-sha256=${P_CHECKSUM}"
				echo "osx-installer=VirtualBox.pkg"
				;;

			*.exe)
				# Windows Installer
				echo "win32=${DOWNLOAD_URL}/${VER}/${P_FILE}"
				echo "win32-sha256=${P_CHECKSUM}"
				echo "win32-installer=${P_FILE}"
				;;

			*.deb)
				# DEB-Based linux installer (Debian/Ubuntu)
				local LINUX_ARCH="linux32"
				if [[ "$P_FILE" =~ "amd64" ]] || [[ "$P_FILE" =~ "x86_64" ]]; then
					LINUX_ARCH="linux64"
				fi
				local LINUX_PLATF=$(echo "$P_FILE" | awk -F'~' '{print $2}' | tr '[:upper:]' '[:lower:]')
				local LINUX_FLAVOR=$(echo "$P_FILE" | awk -F'~' '{print $3}' | awk -F'_' '{print $1}' | tr '[:upper:]' '[:lower:]')

				echo "${LINUX_ARCH}-${LINUX_PLATF}-${LINUX_FLAVOR}=${DOWNLOAD_URL}/${VER}/${P_FILE}"
				echo "${LINUX_ARCH}-${LINUX_PLATF}-${LINUX_FLAVOR}-sha256=${P_CHECKSUM}"
				echo "${LINUX_ARCH}-${LINUX_PLATF}-${LINUX_FLAVOR}-installer=dpkg"

				# Process hand-mapped versions
				IFS=$'\n'
				for map_line in $DEB_MAPPING_VERSIONS; do
					FIND=$(echo "$map_line" | awk -F'=' '{print $1}')
					REPLACE=$(echo "$map_line" | awk -F'=' '{print $2}')
					if [[ "$P_FILE" =~ $FIND ]]; then
						echo "${LINUX_ARCH}-${REPLACE}=${DOWNLOAD_URL}/${VER}/${P_FILE}"
						echo "${LINUX_ARCH}-${REPLACE}-sha256=${P_CHECKSUM}"
						echo "${LINUX_ARCH}-${REPLACE}-installer=${P_FILE}"
					fi
				done
				unset IFS

				;;

			*.rpm)
				# RPM-Based linux installer (RHEL)
				local LINUX_ARCH="linux32"
				if [[ "$P_FILE" =~ "amd64" ]] || [[ "$P_FILE" =~ "x86_64" ]]; then
					LINUX_ARCH="linux64"
				fi

				# Process hand-mapped versions
				IFS=$'\n'
				for map_line in $RPM_MAPPING_VERSIONS; do
					FIND=$(echo "$map_line" | awk -F'=' '{print $1}')
					REPLACE=$(echo "$map_line" | awk -F'=' '{print $2}')
					if [[ "$P_FILE" =~ $FIND ]]; then
						echo "${LINUX_ARCH}-${REPLACE}=${DOWNLOAD_URL}/${VER}/${P_FILE}"
						echo "${LINUX_ARCH}-${REPLACE}-sha256=${P_CHECKSUM}"
						echo "${LINUX_ARCH}-${REPLACE}-installer=${P_FILE}"
					fi
				done
				unset IFS
				;;

		esac
	done
}

# -------------------------
# Download the checksum file for the given version
# -------------------------
function fetch {
	local VER="$1"
	wget -O - -q "${HASH_URL}/${VER}/${HASH_FILE}"
}

# Generate the installer files
fetch ${VERSION_ACTIVE} | parse_os_versions ${VERSION_ACTIVE}

# Generate the guest additions files
for VER in ${VERSION_ALL}; do
	fetch $VER | parse_extpack $VER
done
