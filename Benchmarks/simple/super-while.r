execute <- function(n) {
  x <- 0
  f <- function() {
    while (x < n) {
      x <<- x + 1
    }
  }
  f()
}
run <- function(args) {
    iter <- args[[1]]
    numIterations <- strtoi(args[[1]])
    innerIterations <- strtoi(args[[2]])
    for(i in 1:numIterations) {
        execute(innerIterations)
    }
}

run(commandArgs(trailingOnly=TRUE))
