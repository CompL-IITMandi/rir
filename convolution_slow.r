onvolveSlow <- function(x,y) {
  nx <- length(x)
  ny <- length(y)
  z <- numeric(nx + ny - 1)
  for(i in seq(length = nx)) {
    xi <- x[[i]]
    for(j in seq(length = ny)) {
      ij <- i + j - 1
      z[[ij]] <- z[[ij]] + xi * y[[j]]
    }
  }
  z
}

execute <- function(n) {
  a <- 1:n
  b <- 1:n
  checksum <- 0
  for (i in 1:10) {
    checksum <- checksum + onvolveSlow(a,b)[[n]]
  }
  cat("Convolution ", n, " " , checksum, ": ")
  checksum
}

f.loadBitcode("execute_8162.meta")
f.loadBitcode("chkDots_9623.meta")
# f.loadBitcode("onvolveSlow_9513.meta")

rir.compile(execute)
rir.compile(chkDots)

start.time <- Sys.time()
execute(10)
execute(1)
execute(1)
execute(1)
execute(1)
execute(1)
execute(10)
execute(1)
execute(1)
execute(1)
execute(1)
execute(1)
execute(1)
execute(1)
execute(1)
execute(1)
end.time <- Sys.time()
time.taken <- end.time - start.time
print(paste("execution time: ",time.taken))
