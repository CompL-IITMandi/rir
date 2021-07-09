execute <- function(n) {
  a <- 1
  f <- function() a+a+a+a
  f()
  a <- 1L
  f()
  f()
  for (i in 1:n) f()

  body(execute)<<- body(execute)


}
run <- function(args) {
    numIterations <- strtoi(args[[1]])
    innerIterations <- strtoi(args[[2]])
    print(numIterations)
    print(innerIterations)
    for(i in 1:numIterations) {
        execute(innerIterations)
    }
}

run(commandArgs(trailingOnly=TRUE))
