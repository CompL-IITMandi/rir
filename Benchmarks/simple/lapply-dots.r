execute <- function(n) {
  x <- 1:n
  f <- function(x, y) x + 1.5 * y
  a <- n / 2
  lapply(x, f, y=a)
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
