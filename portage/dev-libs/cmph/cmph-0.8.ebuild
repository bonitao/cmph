# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /cvsroot/cmph/cmph/portage/dev-libs/cmph/cmph-0.8.ebuild,v 1.1 2008/04/28 01:27:31 davi Exp $

inherit eutils
DESCRIPTION="Library for building large minimal perfect hashes"
HOMEPAGE="http://cmph.sf.net/"
SRC_URI="http://unc.dl.sourceforge.net/sourceforge/cmph/${P}.tar.gz"
LICENSE="LGPL-2"
SLOT="0"
KEYWORDS="~x86"
IUSE=""
DEPEND="gcc libtool libc"
RDEPEND="libc"

S=${WORKDIR}/${P}

src_compile() {
	econf || die "econf failed"
	emake || die "emake failed"
}

src_install() {
	make DESTDIR=${D} install || die
}
