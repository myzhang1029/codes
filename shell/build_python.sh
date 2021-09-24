#!/bin/sh -e

NPROC="${NPROC:-$(nproc)}"
PREFIX="${PREFIX:-"/opt/python3.9"}"
DOWNLOAD="${DOWNLOAD:-"wget"}"
DOWNFLAGS="${DOWNFLAGS:-"-c"}"
WORKDIR="${WORKDIR:-"./build_python"}"

# Versions to be used
MPDECIMAL_V="${MPDECIMAL_V:-"2.5.1"}"
READLINE_V="${READLINE_V:-"8.1"}"
SQLITE3_V="${SQLITE3_V:-"3360000"}"
TCL_V="${TCL_V:-"8.6.11"}"
LIBFFI_V="${LIBFFI_V:-"3.4.2"}"
NCURSES_V="${NCURSES:-"6.2"}"
ZLIB_V="${ZLIB_V:-"1.2.11"}"
GDBM_V="${GDBM_V:-"1.20"}"
XZ_V="${XZ_V:-"5.2.5"}"
OPENSSL_V="${OPENSSL_V:-"1.1.1l"}"
PYTHON_V="${PYTHON_V:-"3.9.7"}"

# Prepare build environment
mkdir -p "${PREFIX}"
mkdir -p "${WORKDIR}" || true
cd "${WORKDIR}"

# Download source code archives
if [ -f ".download_complete.marker" ]
then
    echo "WARNING: Assuming download has completed, if not, remove $PWD/.download_complete.marker and continue." >&2
else
    mkdir -p build_downloads || true
    (
        cd build_downloads
        # shellcheck disable=SC2086 # intended
        "$DOWNLOAD" $DOWNFLAGS \
            https://www.bytereef.org/software/mpdecimal/releases/mpdecimal-"$MPDECIMAL_V".tar.gz \
	        ftp://ftp.cwru.edu/pub/bash/readline-"$READLINE_V".tar.gz \
	        https://sqlite.org/2021/sqlite-autoconf-"$SQLITE3_V".tar.gz \
	        https://prdownloads.sourceforge.net/tcl/tcl"$TCL_V"-src.tar.gz \
	        https://prdownloads.sourceforge.net/tcl/tk"$TCL_V"-src.tar.gz \
            https://github.com/libffi/libffi/releases/download/v"$LIBFFI_V"/libffi-"$LIBFFI_V".tar.gz \
	        ftp://ftp.gnu.org/gnu/ncurses/ncurses-"$NCURSES_V".tar.gz \
	        https://www.zlib.net/zlib-"$ZLIB_V".tar.gz \
	        ftp://ftp.gnu.org/gnu/gdbm/gdbm-"$GDBM_V".tar.gz \
	        https://tukaani.org/xz/xz-"$XZ_V".tar.gz \
            https://www.openssl.org/source/openssl-"$OPENSSL_V".tar.gz \
	        https://www.python.org/ftp/python/"$PYTHON_V"/Python-"$PYTHON_V".tgz
    )
    touch .download_complete.marker
fi

# Unpack sources
mkdir -p build_sources || true
cd build_sources

if [ -f ".unpack_complete.marker" ]
then
    echo "WARNING: Assuming unpack has completed, if not, remove $PWD/.unpack_complete.marker and continue." >&2
else
    for file in ../build_downloads/*
    do
	    echo "Unpacking $file"
	    tar zxvf "$file"
    done
    touch .unpack_complete.marker
fi

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PREFIX/lib"

# $1: Package name/path
# $2: Path to configure relative to package root
# $3: Arguments to configure
build()
{
    pkg="$1"
    configure="$2"
    shift 2
    if [ -f "$pkg/.build_complete.marker" ]
    then
        echo "WARNING: Assuming $pkg has been built successfully, if not, remove $PWD/$pkg/.build_complete.marker and continue." >&2
    else
        echo Building "$pkg"
        (
	        cd "$pkg"
	        CFLAGS="-I$PREFIX/include $CFLAGS" \
	            CPPFLAGS="-I$PREFIX/include $CPPFLAGS" \
                LDFLAGS="-L$PREFIX/lib $LDFLAGS" \
                "$configure" --prefix="$PREFIX" "$@"
	        make -j"$NPROC"
	        make install
	        make distclean
            touch .build_complete.marker
        )
    fi
}


for package in tcl"$TCL_V" tk"$TCL_V"
do
    build "$package" "unix/configure"
done

# sqlite and gdbm depends on readline
for package in zlib-"$ZLIB_V" \
    mpdecimal-"$MPDECIMAL_V" \
    ncurses-"$NCURSES_V" \
    readline-"$READLINE_V" \
    gdbm-"$GDBM_V" \
    sqlite-autoconf-"$SQLITE3_V" \
    xz-"$XZ_V" \
    libffi-"$LIBFFI_V"
do
    build "$package" "./configure"
done

build openssl-"$OPENSSL_V" "./config"

build "Python-$PYTHON_V" "./configure" --enable-ipv6 --enable-optimizations --with-openssl="$PREFIX" --with-lto --with-system-libmpdec --with-system-ffi

echo "Your Python $PYTHON_V is available at $PREFIX"
echo "If you are using Linux, you may need to set LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:$PREFIX/lib\" apart from \$PATH"
exit 0
