execute <- function(n) {
  x <- list()
  i <- 1
  while (i < n) {
    x[[i]] <- 5
    i <- i + 1
  }
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
