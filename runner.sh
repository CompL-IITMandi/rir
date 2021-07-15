testing=`ls ./Benchmarks/Testing/*.r 2>/dev/null`
simpleBench=`ls ./Benchmarks/simple/*.r 2>/dev/null`
realThingBench=`ls ./Benchmarks/RealThing/*.r 2>/dev/null`
areWeFastBench=`ls ./Benchmarks/areWeFast/*.r 2>/dev/null`

# Benchmarks
# 0 -> Testing
# 1 -> simple
# 2 -> realThing
# 3 -> areWeFast

clear=0
profileBenchmark=-1
iter=100
inner=10
s=1
op_dir="./outputs/"
hpc=0

while getopts i:n:b:c:s:h: flag
do
    case "${flag}" in
        b) profileBenchmark=${OPTARG};; # Benchmark number
        c) clear=1;;                    # Clear previous outputs
        i) iter=${OPTARG};;             # iterations for the benchmark
        n) inner=${OPTARG};;            # inner iterations for the benchmark
        s) s=${OPTARG};;                # stabalization count
        h) hpc=1                        # running on HPC ?
    esac
done


# set hpc directories
if [[ $hpc -eq 1 ]]
then
    printf "\n### Running on HPC ###\n"
    # create the directory if it does not exits
    [ ! -d "/tmp/s20012_outputs" ] && mkdir /tmp/s20012_outputs
    op_dir="/tmp/s20012_outputs/"
    # Remove previous outputs from the home directory
    rm -rf ~/scrO
fi

# clear the directory if -c was selected
if [[ $clear -eq 1 ]]
then
    if [[ $hpc -eq 1 ]]
    then
        rm -rf /home/s20012/scrO
        mkdir /home/s20012/scrO
        op_dir="/tmp/s20012_outputs/"
    fi
    printf "\n### Clearing Previous Outputs ###\n"
    rm -rf $op_dir
    mkdir $op_dir   
    
fi



copyOutputs () {
    if [[ $hpc -eq 1 ]]
    then
        rsync -a $op_dir /home/s20012/scrO
    fi
}

executeBench () {
    for (( c=1; c<=$s; c++ ))
    do
        time RT_LOG="${op_dir}${1}" SAVE_BIN="${op_dir}${1}" LOAD_BIN="${op_dir}${1}" ./bin/Rscript $2 $iter $inner
    done
    copyOutputs
}

if [[ $profileBenchmark -eq 0 ]]
then
    printf "\n### Executing Testing ###\n"
    for eachfile in $testing
    do
        a=($(echo "$eachfile" | tr '/' '\n'))
        bench_file=${a[3]}
        bench_plain_name=${bench_file%.*}
        printf "\n*** $bench_plain_name ***\n"

        executeBench "$bench_plain_name" $eachfile
    done
elif [[ $profileBenchmark -eq 1 ]]
then
    printf "\n### Executing Simple ###\n"
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
    printf "\n### Executing RThing ###\n"
    for eachfile in $realThingBench
    do
        a=($(echo "$eachfile" | tr '/' '\n'))
        bench_file=${a[3]}
        bench_plain_name=${bench_file%.*}
        printf "\n*** $bench_plain_name ***\n"

        executeBench "$bench_plain_name" $eachfile
    done
elif [[ $profileBenchmark -eq 3 ]]
then
    printf "\n### Executing AreWeFast ###\n"
    for eachfile in $areWeFastBench
    do
        a=($(echo "$eachfile" | tr '/' '\n'))
        bench_file=${a[3]}
        bench_plain_name=${bench_file%.*}
        printf "\n*** $bench_plain_name ***\n"

        executeBench "$bench_plain_name" $eachfile
    done
else
    printf "\n*** ERR: Invalid Benchmark ***\n"
fi
