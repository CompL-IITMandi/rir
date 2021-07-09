
verifyResult <- function(x, ...) {
    UseMethod("verifyResult", x)
}

verifyResult.default <- function(result, benchmarkParameter) {
    TRUE
}

innerBenchmarkLoop <- function(x, ...) {
    UseMethod("innerBenchmarkLoop", x)
}

innerBenchmarkLoop.default <- function(class, benchmarkParameter) {
    verifyResult(execute(benchmarkParameter), benchmarkParameter)
}

results <- c()

doRuns <- function(name, iterations, benchmarkParameter) {
    total <- 0
    class(name) <- tolower(name)
    for (i in 1:iterations) {
        startTime <- Sys.time()
        if (!innerBenchmarkLoop(name, benchmarkParameter)) {
            stop("Benchmark failed with incorrect result")
        }
        endTime <- Sys.time()
        runTime <- (as.numeric(endTime) - as.numeric(startTime)) * 1000000

        cat(name, ": iterations=1 runtime: ", round(runTime), "us\n", sep = "")
        results <<- c(results, runTime)
        total <- total + runTime
    }
    total
}

run <- function(args) {
    if (length(args) != 4)
        stop(printUsage())
    
    name <- args[[1]]
    numIterations <- strtoi(args[[2]])
    benchmarkParameter <- strtoi(args[[3]])

    source(file.path("./Benchmarks/RealThing", paste(name, ".r", sep="")))
    
    total <- as.numeric(doRuns(name, numIterations, benchmarkParameter));
    cat(name, ": ",
        "iterations=", numIterations, "; ",
        "average: ", round(total / numIterations), " us; ",
        "total: ", round(total), "us\n\n", sep="")
    #cat("Total runtime: ", total, "us\n\n", sep="")
    destroyX = function(es) {
        f = es
        for (col in c(1:ncol(f))){ #for each column in dataframe
            if (startsWith(colnames(f)[col], "X") == TRUE)  { #if starts with 'X' ..
            colnames(f)[col] <- substr(colnames(f)[col], 2, 100) #get rid of it
            }
        }
        assign(deparse(substitute(es)), f, inherits = TRUE) #assign corrected data to original name
    }

    tryCatch(
        expr = {
            l <- read.csv(paste(name,".csv", sep=""))
            runner <- args[[4]]
            # df <- data.frame(dataName=results)            
            l$newdata <- results
            names(l)[names(l) == 'newdata'] <- paste(runner, sep="")
            destroyX(l)
            write.csv(l, file = paste(name,".csv", sep=""),row.names=FALSE)
        },
        error = function(e){
            print("ERROR")
            runner <- args[[4]]
            df <- data.frame(dataName=results)
            colnames(df) <- c(paste(runner, sep=""))
            write.csv(df, file = paste(name,".csv", sep=""),row.names=FALSE)
        },
        warning = function(w){
            print("WARNING")
            runner <- args[[4]]
            df <- data.frame(dataName=results)
            colnames(df) <- c(paste(runner, sep=""))
            write.csv(df, file = paste(name,".csv", sep=""),row.names=FALSE)
        },
        finally = {
            message('All done, quitting.')
        }
    )
}

printUsage <- function() {
    cat("harness.r benchmark num-iterations benchmark-parameter [inner-iter]\n")
    cat("\n")
    cat("  benchmark           - benchmark class name\n")
    cat("  num-iterations      - number of times to execute benchmark\n")
    cat("  benchmark-parameter - size of the benchmark problem\n")
    cat("  inner-iter          - number of times the benchmark is executed in an inner loop,\n")
    cat("                        which is measured in total, default: 1\n")
}

run(commandArgs(trailingOnly=TRUE))
