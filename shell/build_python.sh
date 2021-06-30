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
LIBRESSL_V="${LIBRESSL_V:-"3.3.3"}"
PYTHON_V="${PYTHON_V:-"3.9.6"}"

# Prepare build environment
mkdir -p "${PREFIX}"
mkdir -p "${WORKDIR}" || true
cd "${WORKDIR}"

# Download source code archives
if [ -f ".download_complete.marker" ]
then
    echo "WARNING: Assuming download has completed, if not, remove $PWD/.download_complete.marker and continue." >&2
else
    mkdir -p .downloads || true
    cd .downloads
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
	    https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-"$LIBRESSL_V".tar.gz \
	    https://www.python.org/ftp/python/"$PYTHON_V"/Python-"$PYTHON_V".tgz
    cd ..
    touch .download_complete.marker
fi

# Unpack sources
mkdir -p .sources || true
cd .sources

if [ -f ".unpack_complete.marker" ]
then
    echo "WARNING: Assuming unpack has completed, if not, remove $PWD/.download_complete.marker and continue." >&2
else
    for file in ../.downloads/*
    do
	    echo "Unpacking $file"
	    tar zxvf "$file"
    done
    touch .unpack_complete.marker
fi

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"

# $1: Package name/path
# $2: Path to configure relative to package root
# $3: Arguments to configure
build()
{
    if [ -f "$1/.build_complete.marker" ]
    then
        echo "WARNING: Assuming $1 has been built successfully, if not, remove $PWD/$1/.build_complete.marker and continue." >&2
    else
        echo Building "$1"
	    cd "$1"
	    CFLAGS="-I$PREFIX/include $CFLAGS" LDFLAGS="-L$PREFIX/lib $LDFLAGS" "$2" --prefix="$PREFIX" $3
	    make -j"$NPROC" install
	    make distclean
        touch .build_complete.marker
        cd ..
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
    libressl-"$LIBRESSL_V" \
    xz-"$XZ_V" \
    libffi-"$LIBFFI_V"
do
    build "$package" "./configure"
done

build "Python-$PYTHON_V" "./configure" --enable-optimizations

echo "Your Python $PYTHON_V is available at $PREFIX"
exit 0
