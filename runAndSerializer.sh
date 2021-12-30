folder=$1

if [ -z "$1" ];
then
  folder="toRun"
fi

# PIR_SERIALIZE_ALL=1  ./bin/Rscript $fileName > out
cases=`ls $folder/*.r 2>/dev/null`

rm out 2>/dev/null

for eachfile in $cases
do
  a=($(echo "$eachfile" | tr '/' '\n'))
  file=${a[-1]}
  plain_name=${file%.*}

  rm -rf bitcodes/$plain_name 2>/dev/null
  mkdir bitcodes/$plain_name

  echo "Running $eachfile" >> out

  bcPath="./bitcodes/$plain_name"

  PIR_SERIALIZE_ALL=1 PIR_SERIALIZE_PREFIX="$bcPath" ./bin/Rscript $eachfile >> out
done
