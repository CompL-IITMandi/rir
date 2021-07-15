# h <- function() { Sys.sleep(1) }
# g <- function() { h(); }
# f <- function() { g(); }

# f();
# f();
# f();
# f();
# f();

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

f <- function() {
	options(useFancyQuotes=c)
	require("Matrix")
	a <- c("", "", "tsp")
	b <- function(d) {
	    setClass("foo" )
	}
	e <- sapply(a, b)
	str(e) #
	setOldClass(c("foo", "numeric"))
	setClass("A", representation(slot1="numeric", slot2="logical"))
	setClass("D1", "A" )
}

execute <- function(n) {
	for(i in 1:n) {
		f()
	}
}

execute(1)
