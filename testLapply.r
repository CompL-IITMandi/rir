x <- list(a = 1:1000, beta = exp(-30:30), logic = c(TRUE,FALSE,FALSE,TRUE))
invisible(rir.compile(lapply))

# invisible(lapply(x, mean))
# invisible(lapply(x, mean))
# invisible(lapply(x, mean))
# invisible(lapply(x, mean))
# invisible(lapply(x, mean))

lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)

f.serialize(lapply)
