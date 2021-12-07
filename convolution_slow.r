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

execute(1)
execute(1)
execute(10)
execute(1)
execute(10)

f.serialize(chkDots)
f.serialize(onvolveSlow)
f.serialize(execute)

execute(1)
execute(10)
execute(1)
execute(10)
execute(1)
