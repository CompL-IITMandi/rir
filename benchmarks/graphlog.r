
require(graphics)
library(ggplot2)
library(scales)

graphlog <- function(medNR, graphDir="benchGraph"){

	#graphing medNR
	#graphName <- paste(pathName, testName, sep="/")
	pdf(paste(graphDir, "pdf", sep="."))
	print(paste(graphDir, "pdf", sep="."))

	graphn <- ggplot() + geom_pointrange(data=medNR, mapping=aes(x=name, y=median_time, ymin=top_quantile, ymax=bottom_quantile), ordered=TRUE, width=0.8, size=0.8, color="blue", fill="white", shape=20) + 
						 theme(axis.text.x = element_text(angle = 90, hjust = 1)) + 
						 geom_hline(yintercept=1, size=0.5, width=0.5) + 
						 scale_y_continuous(labels=percent) + 
						 ylab("% of slowdown") + 
						 #annotate("text", x=8, y = 1.1, label = "normalised gnur", size=3) + 
						 annotate("text", x=medNR$graph_CT, y=max(medNR$compilation_time, na.rm=TRUE), label=medNR$large_CT, angle=60, size=2) + 
						 annotate("point", x=medNR$graph_CT, y=max(medNR$compilation_time, na.rm=TRUE)+0.13, label=medNR$large_CT, shape=94, size=5, color="red") + 
						 annotate("point", x=medNR$name, y=medNR$compilation_time, color="red") +
						 expand_limits(y=0) + 
						 ggtitle("% slowdown of rjit against gnur for \n the programming language game benchmark")
	print(graphn)
	dev.off()
	print("done")
}
