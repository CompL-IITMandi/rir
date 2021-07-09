h <- function() { Sys.sleep(1) }
g <- function() { h(); }
f <- function() { g(); }

f();
f();
f();
f();
f();

# execute <- function(a) {
#   f()
# }

# run <- function(args) {
#     numIterations <- strtoi(args[[1]])
#     innerIterations <- strtoi(args[[2]])
#     for(i in 1:numIterations) {
#         print(i)
#         execute(innerIterations)
#     }
# }

# run(commandArgs(trailingOnly=TRUE))
