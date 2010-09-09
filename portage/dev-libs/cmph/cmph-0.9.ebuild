DESCRIPTION="Library for building large minimal perfect hashes"
HOMEPAGE="http://cmph.sf.net/"
SRC_URI="mirror://sourceforge/cmph/${PN}/${P}/${P}.tar.gz"
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
	emake DESTDIR=${D} install || die
}
