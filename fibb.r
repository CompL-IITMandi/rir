recurse_fibonacci <- function(n) {
  if(n <= 1) {
    return(n)
  } else {
    return(recurse_fibonacci(n-1) + recurse_fibonacci(n-2))
  }
}
nterms = 37
# for(i in 0:(nterms-1)) {
#   recurse_fibonacci(i)
# }
f.loadBitcode("recurse_fibonacci_6222.meta")
invisible(rir.compile(recurse_fibonacci))
start.time <- Sys.time()
recurse_fibonacci(20)
recurse_fibonacci(20)
recurse_fibonacci(20)
recurse_fibonacci(20)
recurse_fibonacci(20)
recurse_fibonacci(20)
recurse_fibonacci(20)
recurse_fibonacci(20)
recurse_fibonacci(20)
recurse_fibonacci(20)

end.time <- Sys.time()
time.taken <- end.time - start.time
print(paste("execution time: ",time.taken))
