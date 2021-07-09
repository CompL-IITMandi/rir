files1=`ls ./Benchmarks/Custom/*.r 2>/dev/null`
simpleBench=`ls ./Benchmarks/simple/*.r 2>/dev/null`
RealThingBench=`ls ./Benchmarks/RealThing/*.r 2>/dev/null`
AreWeFastBench=`ls ./Benchmarks/areWeFast/*.r 2>/dev/null`
testing=`ls ./Benchmarks/Testing/*.r 2>/dev/null`

clear=0
profileBenchmark=-1
iter=10
inner=20
s=1
while getopts i:n:b:c:s: flag
do
    case "${flag}" in
        b) profileBenchmark=${OPTARG};;
        c) clear=`rm -rf ./outputs/* && rm ./*.csv`;;
        i) iter=${OPTARG};;
        n) inner=${OPTARG};;
        s) s=${OPTARG};;
    esac
done


execute () {
    printf "\n*** $2 ***\n"
    for (( c=1; c<=$s; c++ ))
    do
        time RT_LOG="./outputs/${1}" SAVE_BIN="./outputs/${1}" LOAD_BIN="./outputs/${1}" ./bin/Rscript ./Benchmarks/custom-harness.r $1 $iter $inner $2
    done
}

executeBench () {
    for (( c=1; c<=$s; c++ ))
    do
        time RT_LOG="./outputs/${1}" SAVE_BIN="./outputs/${1}" LOAD_BIN="./outputs/${1}" ./bin/Rscript $2 $iter $inner
    done
}

if [[ $profileBenchmark -eq 0 ]]
then
    echo "Executing Testing"
    for eachfile in $testing
    do
        a=($(echo "$eachfile" | tr '/' '\n'))
        bench_file=${a[3]}
        bench_plain_name=${bench_file%.*}
        printf "\n*** $bench_plain_name ***\n"

        time RT_LOG="./outputs/$bench_plain_name" SAVE_BIN="./outputs/$bench_plain_name" LOAD_BIN="./outputs/$bench_plain_name" ./bin/Rscript $eachfile $iter $inner
    done
elif [[ $profileBenchmark -eq 1 ]]
then
    echo "Executing Simple Bench"
    for eachfile in $simpleBench
    do
        a=($(echo "$eachfile" | tr '/' '\n'))
        bench_file=${a[3]}
        bench_plain_name=${bench_file%.*}
        printf "\n*** $bench_plain_name ***\n"

        executeBench "$bench_plain_name" $eachfile
    done
elif [[ $profileBenchmark -eq 2 ]]
then
    echo "Executing RThing Bench"
    for eachfile in $RealThingBench
    do
        a=($(echo "$eachfile" | tr '/' '\n'))
        bench_file=${a[3]}
        bench_plain_name=${bench_file%.*}
        printf "\n*** $bench_plain_name ***\n"

        executeBench "$bench_plain_name" $eachfile
    done
elif [[ $profileBenchmark -eq 3 ]]
then
    echo "Executing AreWeFast Bench"
    for eachfile in $AreWeFastBench
    do
        a=($(echo "$eachfile" | tr '/' '\n'))
        bench_file=${a[3]}
        bench_plain_name=${bench_file%.*}
        printf "\n*** $bench_plain_name ***\n"

        executeBench "$bench_plain_name" $eachfile
    done
else
    for eachfile in $files1
    do
        printf "\n*** ERR ***\n"
    done

fi
