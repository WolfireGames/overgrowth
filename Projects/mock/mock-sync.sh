dir=$PWD
arch=$1
app=$2

if [ -z "$arch" -o -z "$app" ]; then
	echo "Specify if 32 or 64 arch sync and the app"
	exit 1
fi

shift 1

if [ "$1" = "-n" ]; then
	rsync_args=-vvn
	shift 1
fi

root=$dir/mock-root${arch}

if [ ! -d $root ]; then
	echo "Pick a correct sync arch (32 or 64)"
	exit 1
fi

for app in $*; do
	if [ ! -e $dir/$app/mock-sync.txt -a ! -e $dir/$app/mock-rsync.txt ]; then
		echo "App $app has no mock-sync.txt or mock-rsync.txt"
		continue
	fi

    mkdir -p $root/$app/build

    if [ -e $dir/$app/mock-sync.txt ]; then
        pushd ./$app
            srcpaths=`cat $dir/$app/mock-sync.txt`
            for srcpath in $srcpaths; do
                if [ -d $srcpath ]; then
                    mkdir -p $root/$app/$srcpath/ 
                    rsync -a  $srcpath/ $root/$app/$srcpath/
                else
                    mkdir -p `dirname $root/$app/$srcpath `
                    rsync -a  $srcpath $root/$app/$srcpath
                fi
            done
        popd
    fi

    if [ -e $dir/$app/mock-rsync.txt ]; then
        rsync -a -f ". $dir/$app/mock-rsync.txt" $app/ $root/$app/ $rsync_args
    fi


done
