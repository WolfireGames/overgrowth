arch=$1
shift
if [ -z "$arch" ]; then
	echo "Specify if 32 or 64 arch sync and the app"
	exit 1
fi

if [ $arch = "32" ]; then
ROOT=epel-6-i386
CMAKE=cmake
elif [ $arch = "64" ]; then
ROOT=epel-6-x86_64
CMAKE=cmake
else
	echo "Pick a correct sync arch (32 or 64)"
	exit
fi


if [ "$1" == "init" ]; then
	mock -r $ROOT --init
	shift
fi

mock -r $ROOT --install ccache dos2unix \
    pkgconfig mesa-libGLU-devel \
    libX11-devel libXrandr-devel libXmu-devel \
    libXi-devel libXext-devel libXft-devel \
    alsa-lib-devel freeglut-devel \
    libtiff-devel curl-devel \
    pulseaudio-libs-devel freetype-devel $CMAKE \
    libpng-devel libjpeg-devel zlib-devel fltk \
    vim-enhanced libXinerama-devel libXpm-devel \
    gtk2 dbus-glib GConf2 \
    $*
