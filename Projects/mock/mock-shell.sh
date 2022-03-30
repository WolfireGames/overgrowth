arch=$1
shift
if [ -z "$arch" ]; then
	echo "$0 ARCH [-- command]"
	echo "Specify if 32 or 64 arch and an optional command to run in the shell"
	exit 1
fi

if [ $arch = "32" ]; then
ROOT=epel-6-i386
else
ROOT=epel-6-x86_64
fi

mock -r $ROOT --unpriv --shell $*
