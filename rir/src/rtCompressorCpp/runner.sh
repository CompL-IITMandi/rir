# rtCompressorCpp input.logg output.json

clear=1
op_dir="outputs/"
hpc=0
directory="input"
level=3
verbose=0
while getopts d:c:h:l:v: flag
do
	case "${flag}" in
		d) directory=${OPTARG};;        # directory
		c) clear=${OPTARG};;            # Clear previous outputs
		h) hpc=1;;                      # running on HPC ?
		l) level=${OPTARG};;            # level ( 0 -> [viz], 1 -> [viz + lists], 2 -> [viz + CallGraph], 3 -> [viz + Callgraph + lists] )
		v) verbose=${OPTARG};;          # verbose
	esac
done

files=`ls $directory/*.logg 2>/dev/null`

# set hpc directories
if [[ $hpc -eq 1 ]]
then
	printf "\n### Running on HPC ###\n"
	# create the directory if it does not exits
	[ ! -d "/tmp/s20012_outputs_compressor" ] && mkdir /tmp/s20012_outputs_compressor
	op_dir="/tmp/s20012_outputs_compressor/"
	# Remove previous outputs from the home directory
	rm -rf ~/s20012_outputs_compressor

	files=`ls $directory/scrO/*.logg 2>/dev/null`

fi

# clear the directory if -c was selected
if [[ $clear -eq 1 ]]
then
	if [[ $hpc -eq 1 ]]
	then
		rm -rf /home/s20012/s20012_outputs_compressor
		mkdir /home/s20012/s20012_outputs_compressor
		op_dir="/tmp/s20012_outputs_compressor/"
	fi
	printf "\n### Clearing Previous Outputs ###\n"
	rm -rf $op_dir
	mkdir $op_dir
fi

copyOutputs () {
	if [[ $hpc -eq 1 ]]
	then
		rsync -a $op_dir /home/s20012/s20012_outputs_compressor
	fi
}

for eachfile in $files
do
	a=($(echo "$eachfile" | tr '/' '\n'))
	file=${a[-1]}
	plain_name=${file%.*}
	mkdir "$op_dir${file%.*}"
	output_file="$op_dir${file%.*}/${file%.*}.json"
	printf "\n*** $eachfile --> $output_file ***\n"
	./rtCompressor $eachfile $output_file $level $verbose
	copyOutputs
done
