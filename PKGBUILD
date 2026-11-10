pkgname=kalamake
pkgver=2.0.3
pkgrel=1
pkgdesc="KalaMake build system"
arch=('x86_64')
url="https://github.com/kalakit/kalamake"
license=('ZLIB')
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('90ffd20adb05878378d333bc7252a3846c6bafa1549f93330794ef4f12bb4ba7')
depends=('glibc' 'gcc-libs')
makedepends=('clang')

build() {
    cd "$srcdir/KalaMake-$pkgver"

    mkdir -p build/release-linux
    cd build/release-linux

    clang++ -DLIB_STATIC -std=c++20 \
        ../../src/*.cpp \
        ../../src/*/*.cpp \
        -I../../include \
        -I../../external/kalaheaders/include \
        -I../../external/kalacli/include \
        ../../external/kalacli/libkalacli.a \
        -o kalamake
}
package() {
    cd "$srcdir/KalaMake-$pkgver"

    install -Dm755 build/release-linux/kalamake \
        "$pkgdir/usr/bin/kalamake"

    install -Dm644 LICENSE.md \
        "$pkgdir/usr/share/licenses/$pkgname/LICENSE"

    install -Dm644 README.md \
        "$pkgdir/usr/share/doc/$pkgname/README.md"

    cp -a docs \
        "$pkgdir/usr/share/doc/$pkgname/"
}
