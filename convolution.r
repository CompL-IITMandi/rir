convolve <- function(a, b) # from the extending R manual
{
    a <- as.double(a)
    b <- as.double(b)
    na <- length(a)
    nb <- length(b)
    ab <- double(na + nb)
    for(i in 1 : na)
        for(j in 1 : nb)
            ab[i + j] <- ab[i + j] + a[i] * b[j]
    ab
}
n <- 100
checksum <- 0
a <- 1:n
b <- 1:n

# f.loadBitcode("convolve_9558.meta")
invisible(rir.compile(convolve))

start.time <- Sys.time()
# for (i in 1:10000) {
#   checksum <- checksum + convolve(a,b)[[n]]
# }
# checksum

convolve(a,b)[[n]]

end.time <- Sys.time()
time.taken <- end.time - start.time
print(paste("execution time: ",time.taken))
