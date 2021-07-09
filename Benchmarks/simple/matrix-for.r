execute <- function(n) {
  x <- matrix(nrow=n, ncol=n)
  for (i in 1:n) {
    for (j in 1:n) {
      x[[i, j]] <- 5
    }
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
