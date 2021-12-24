metaFiles=`ls ~/compL/rir_serializer/rir/*.meta 2>/dev/null`
bcFiles=`ls ~/compL/rir_serializer/rir/*.bc 2>/dev/null`
poolFiles=`ls ~/compL/rir_serializer/rir/*.pool 2>/dev/null`

./clean.sh 2>/dev/null

for eachfile in $bcFiles
do
    a=($(echo "$eachfile" | tr '/' '\n'))
    file=${a[-1]}
    plain_name=${file%.*}
    cp $eachfile ./bitcodes
done
for eachfile in $poolFiles
do
    a=($(echo "$eachfile" | tr '/' '\n'))
    file=${a[-1]}
    plain_name=${file%.*}
    cp $eachfile ./bitcodes
done

for eachfile in $metaFiles
do
    a=($(echo "$eachfile" | tr '/' '\n'))
    file=${a[-1]}
    plain_name=${file%.*}
    cp $eachfile ./bitcodes
    echo "f.loadBitcode(\"./bitcodes/$file\")"
done
