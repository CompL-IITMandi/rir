files1=`ls ./cTests/*.r 2>/dev/null`
files2=`ls ./cTests/*.R 2>/dev/null`

clear=0

while getopts u:r:c: flag
do
    case "${flag}" in
        c) clear=`rm -rf ./profile/*`;;
    esac
done


execute () {
  echo "processing: $2"
  norm=`time CONTEXT_LOGS=1 OUT_NAME="${2}" ./bin/Rscript $1`
  coo=`time CONTEXT_LOGS=1 OUT_NAME="${2}_COMPILE_ONLY_ONCE" COMPILE_ONLY_ONCE=1 ./bin/Rscript $1`
  sac=`time CONTEXT_LOGS=1 OUT_NAME="${2}_SKIP_ALL_COMPILATION" SKIP_ALL_COMPILATION=1 ./bin/Rscript $1`
}

for eachfile in $files1
do
    a=($(echo "$eachfile" | tr '/' '\n'))
    execute "$eachfile" "${a[2]}"
done

for eachfile in $files2
do
    a=($(echo "$eachfile" | tr '/' '\n'))
    execute "$eachfile" "${a[2]}"
done
