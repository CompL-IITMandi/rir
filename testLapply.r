x <- list(a = 1:10, beta = exp(-3:3), logic = c(TRUE,FALSE,FALSE,TRUE))

f.loadBitcode("lapply_6959.meta")
invisible(rir.compile(lapply))

invisible(rir.compile(match.fun))
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)

# lapply(x, quantile, probs = 1:3/4)
# lapply(x, quantile, probs = 1:3/4)
# lapply(x, quantile, probs = 1:3/4)
# lapply(x, quantile, probs = 1:3/4)

# lapply(x, mean)
# lapply(x, mean)
# lapply(x, mean)
# lapply(x, mean)


# f.serialize(lapply)
