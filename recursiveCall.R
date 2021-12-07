f <- function(num) { # HAST: 3276
  if (num == 1) {
    return (1)
  } else {
    return ( num * f(num-1) )
  }
}

f.loadBitcode("f_3276.meta")

# invisible(rir.compile(f))
# invisible(rir.disassemble(f))

start.time <- Sys.time()
f(5)
f(5)
f(5)
f(5)
f(5)
end.time <- Sys.time()
time.taken <- end.time - start.time
print(paste("warmup time(first 5): ",time.taken))


start.time <- Sys.time()
f(5)
f(5)
f(5)
f(5)
f(5)
end.time <- Sys.time()
time.taken <- (end.time - start.time) / 5
print(paste("peak avg(last 5): ",time.taken))


f(c(1,5))
