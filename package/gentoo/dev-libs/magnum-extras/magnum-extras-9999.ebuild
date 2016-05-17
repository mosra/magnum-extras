# Copyright 1999-2014 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=5

EGIT_REPO_URI="git://github.com/mosra/magnum-extras.git"

inherit cmake-utils git-r3

DESCRIPTION="Extras for Magnum OpenGL graphics engine"
HOMEPAGE="http://mosra.cz/blog/magnum.php"

LICENSE="MIT"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

RDEPEND="
	dev-libs/magnum
"
DEPEND="${RDEPEND}"

src_configure() {
	# general configuration
	local mycmakeargs=(
		-DCMAKE_INSTALL_PREFIX="${EPREFIX}/usr"
		-DCMAKE_BUILD_TYPE=Release
		-DWITH_UI=ON
		-DWITH_UI_GALLERY=ON
	)
	cmake-utils_src_configure
}

# kate: replace-tabs off;
