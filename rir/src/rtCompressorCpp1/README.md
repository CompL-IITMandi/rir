# rtCompressorCpp

This program is used to process .logg files and generate the JSON files that can be visualized using the rsh viz application.

### Installation and Usage
Requires cpp 14+
```sh
# build the project
make
# input_folder -> must contain .logg files generated from
# the logger
# ./runner.sh -d [input_folder]
#
./runner.sh -d input_folder
# Output json will be saved in the output folder
```
##### This program has been rewritten in cpp, a JS version exists and was deprecated due to memory limitations. 
