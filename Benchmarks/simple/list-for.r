execute <- function(n) {
  x <- list()
  for (i in 1:n) {
    x[[i]] <- i
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
