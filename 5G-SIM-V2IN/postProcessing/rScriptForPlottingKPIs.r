.rs.restartR()
require(omnetpp)
setwd("TODO")

####################### Plotting the Packet Delay as Boxplot

#############################################
##########MW 4APL###########################################################
###Voip
#
datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayVoip:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayVoipMWDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayVoip:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayVoipMWUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL
###Video

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayVideo:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayVideoMWDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayVideo:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayVideoMWUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL
###V2x

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayV2X:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayV2xMWDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayV2X:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayV2xMWUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL
###Data

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayData:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayDataMWDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayData:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayDataMWUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL

##########4AL ################################################################
##########UR
###Voip
#
datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayVoip:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayVoipURDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayVoip:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayVoipURUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL
###Video

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayVideo:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayVideoURDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayVideo:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayVideoURUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL
###V2x

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayV2X:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayV2xURDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayV2X:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayV2xURUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL
###Data

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayData:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayDataURDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/vector-*.vec'),
                         add('vector', 'name("delayData:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayDataURUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL

##############################################################################

##########################################################
#pdf
pdf("Packet Delay 4 Applications Downlink and Uplink.pdf", width=11.0, height=7.5)
par(mfrow=c(2,2))

########################################################
theData <- list(delayV2xMWDL4APPL$y,delayV2xMWUL4APPL$y)
MW25 <- boxplot(theData,
                ylab = "Packet Delay in s",
                names = c("Downlink","Uplink"),
                main="V2X",
                cex.axis = 0.9,
                cex.lab = 1,
                las=1,
                ylim=c(0,0.2),
                outline = FALSE,
                range=1,
                boxwex=0.6,
                staplewex=0.6)
abline(h=0.05,lty=3,lwd=0.5)
abline(h=0.1,lty=3,lwd=0.5)
abline(h=0.15,lty=3,lwd=0.5)
abline(h=0.2,lty=3,lwd=0.5)
abline(h=0.025,lty=3,lwd=0.5)
abline(h=0.075,lty=3,lwd=0.5)
abline(h=0.125,lty=3,lwd=0.5)
abline(h=0.175,lty=3,lwd=0.5)

median <- matrix(c(MW25$stats[3,]),nrow=1,byrow=TRUE)
#text(x = col(median)+0.45, y = median, labels = round(median,digits=3), cex = 0.7, col="black")

meanvalDL <- mean(delayV2xMWDL4APPL$y)
meanvalUL <- mean(delayV2xMWUL4APPL$y)

meansDL <- matrix(c(meanvalDL),nrow=1,byrow=TRUE)
points(x = col(meansDL), y = meansDL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansDL)+0.45, y = meansDL+0.004, labels = round(meansDL,digits=3),cex = 0.7, col="red")

meansUL <- matrix(c(meanvalUL),nrow=1,byrow=TRUE)
points(x = col(meansUL)+1, y = meansUL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansUL)+1.45, y = meansUL, labels = round(meansUL,digits=3),cex = 0.7, col="red")

#clean
meansDL <- NULL
meansUL <- NULL
meanvalDL <- NULL
meanvalUL <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(delayVoipMWDL4APPL$y,delayVoipMWUL4APPL$y)
MW50 <- boxplot(theData,
                ylab = "Packet Delay in s",
                names = c("Downlink","Uplink"),
                main="VoIP",
                #col=c("cornflowerblue","yellow"),
                #at=c(1,2),
                #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                cex.axis = 0.9,
                cex.lab = 1,
                las=1,
                ylim=c(0,0.25),
                outline = FALSE,
                range=1,
                boxwex=0.6,
                staplewex=0.6)
abline(h=0.05,lty=3,lwd=0.5)
abline(h=0.1,lty=3,lwd=0.5)
abline(h=0.15,lty=3,lwd=0.5)
abline(h=0.2,lty=3,lwd=0.5)
abline(h=0.25,lty=3,lwd=0.5)


median <- matrix(c(MW50$stats[3,]),nrow=1,byrow=TRUE)
#text(x = col(median)+0.45, y = median, labels = round(median,digits=3), cex = 0.7, col="black")

meanvalDL <- mean(delayVoipMWDL4APPL$y)
meanvalUL <- mean(delayVoipMWUL4APPL$y)
meansDL <- matrix(c(meanvalDL),nrow=1,byrow=TRUE)
points(x = col(meansDL), y = meansDL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansDL)+0.45, y = meansDL+0.005, labels = round(meansDL,digits=3),cex = 0.7, col="red")

meansUL <- matrix(c(meanvalUL),nrow=1,byrow=TRUE)
points(x = col(meansUL)+1, y = meansUL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansUL)+1.45, y = meansUL, labels = round(meansUL,digits=3),cex = 0.7, col="red")

#clean
meansDL <- NULL
meansUL <- NULL
meanvalDL <- NULL
meanvalUL <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(delayVideoMWDL4APPL$y,delayVideoMWUL4APPL$y)
MW100 <- boxplot(theData,
                 ylab = "Packet Delay in s",
                 names = c("Downlink","Uplink"),
                 main="Video",
                 cex.axis = 0.9,
                 cex.lab = 1,
                 las=1,
                 ylim=c(0,0.8),
                 outline = FALSE,
                 range=1,
                 boxwex=0.6,
                 staplewex=0.6)

abline(h=0.2,lty=3,lwd=0.5)
abline(h=0.4,lty=3,lwd=0.5)
abline(h=0.6,lty=3,lwd=0.5)
abline(h=0.8,lty=3,lwd=0.5)
abline(h=0.1,lty=3,lwd=0.5)
abline(h=0.3,lty=3,lwd=0.5)
abline(h=0.5,lty=3,lwd=0.5)
abline(h=0.7,lty=3,lwd=0.5)


median <- matrix(c(MW100$stats[3,]),nrow=1,byrow=TRUE)
#text(x = col(median)+0.45, y = median, labels = round(median,digits=3), cex = 0.7, col="black")

meanvalDL <- mean(delayVideoMWDL4APPL$y)
meanvalUL <- mean(delayVideoMWUL4APPL$y)
meansDL <- matrix(c(meanvalDL),nrow=1,byrow=TRUE)
points(x = col(meansDL), y = meansDL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansDL)+0.45, y = meansDL+0.012, labels = round(meansDL,digits=3),cex = 0.7, col="red")

meansUL <- matrix(c(meanvalUL),nrow=1,byrow=TRUE)
points(x = col(meansUL)+1, y = meansUL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansUL)+1.45, y = meansUL, labels = round(meansUL,digits=3),cex = 0.7, col="red")

#clean
meansDL <- NULL
meansUL <- NULL
meanvalDL <- NULL
meanvalUL <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(delayDataMWDL4APPL$y,delayDataMWUL4APPL$y)
MW270 <- boxplot(theData,
                 ylab = "Packet Delay in s",
                 names = c("Downlink","Uplink"),
                 main="Data",
                 #col=c("cornflowerblue","yellow"),
                 #at=c(1,2),
                 #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                 cex.axis = 0.9,
                 cex.lab = 1,
                 las=1,
                 ylim=c(0,1.5),
                 outline = FALSE,
                 range=1,
                 boxwex=0.6,
                 staplewex=0.6)

abline(h=0.5,lty=3,lwd=0.5)
abline(h=1,lty=3,lwd=0.5)
abline(h=0.25,lty=3,lwd=0.5)
abline(h=0.75,lty=3,lwd=0.5)
abline(h=1.25,lty=3,lwd=0.5)
abline(h=1.5,lty=3,lwd=0.5)

median <- matrix(c(MW270$stats[3,]),nrow=1,byrow=TRUE)
#text(x = col(median)+0.45, y = median, labels = round(median,digits=3), cex = 0.7, col="black")

meanvalDL <- mean(delayDataMWDL4APPL$y)
meanvalUL <- mean(delayDataMWUL4APPL$y)
meansDL <- matrix(c(meanvalDL),nrow=1,byrow=TRUE)
points(x = col(meansDL), y = meansDL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansDL)+0.45, y = meansDL+0.005, labels = round(meansDL,digits=3),cex = 0.7, col="red")

meansUL <- matrix(c(meanvalUL),nrow=1,byrow=TRUE)
points(x = col(meansUL)+1, y = meansUL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansUL)+1.45, y = meansUL, labels = round(meansUL,digits=3),cex = 0.7, col="red")

#clean
meansDL <- NULL
meansUL <- NULL
meanvalDL <- NULL
meanvalUL <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################
dev.off()


#########################################################################################################################################################
#########################################################################################################################################################
#################### Plotting the Reliability


datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo10msDL*")'))
relVideo4APPDL10MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo20msDL*")'))
relVideo4APPDL20MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo50msDL*")'))
relVideo4APPDL50MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo100msDL*")'))
relVideo4APPDL100MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo200msDL*")'))
relVideo4APPDL200MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo500msDL*")'))
relVideo4APPDL500MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo1sDL*")'))
relVideo4APPDL1MW <- datasetDL$scalars$value

datasetDL <- NULL


####################################################################################################
##UL

#delay Video
###
datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo10msUL*")'))
relVideo4APPUL10MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo20msUL*")'))
relVideo4APPUL20MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo50msUL*")'))
relVideo4APPUL50MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo100msUL*")'))
relVideo4APPUL100MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo200msUL*")'))
relVideo4APPUL200MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo500msUL*")'))
relVideo4APPUL500MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVideo1sUL*")'))
relVideo4APPUL1MW <- datasetUL$scalars$value

datasetUL <- NULL


####################################################################################################
#voip
###
datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip10msDL*")'))
relVoip4APPDL10MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip20msDL*")'))
relVoip4APPDL20MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip50msDL*")'))
relVoip4APPDL50MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip100msDL*")'))
relVoip4APPDL100MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip200msDL*")'))
relVoip4APPDL200MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip500msDL*")'))
relVoip4APPDL500MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip1sDL*")'))
relVoip4APPDL1MW <- datasetDL$scalars$value

datasetDL <- NULL


####################################################################################################
##UL

###
datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip10msUL*")'))
relVoip4APPUL10MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip20msUL*")'))
relVoip4APPUL20MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip50msUL*")'))
relVoip4APPUL50MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip100msUL*")'))
relVoip4APPUL100MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip200msUL*")'))
relVoip4APPUL200MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip500msUL*")'))
relVoip4APPUL500MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityVoip1sUL*")'))
relVoip4APPUL1MW <- datasetUL$scalars$value

datasetUL <- NULL


####################################################################################################
#data
###
datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData10msDL*")'))
relData4APPDL10MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData20msDL*")'))
relData4APPDL20MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData50msDL*")'))
relData4APPDL50MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData100msDL*")'))
relData4APPDL100MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData200msDL*")'))
relData4APPDL200MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData500msDL*")'))
relData4APPDL500MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData1sDL*")'))
relData4APPDL1MW <- datasetDL$scalars$value

datasetDL <- NULL


####################################################################################################
##UL
datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData10msUL*")'))
relData4APPUL10MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData20msUL*")'))
relData4APPUL20MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData50msUL*")'))
relData4APPUL50MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData100msUL*")'))
relData4APPUL100MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData200msUL*")'))
relData4APPUL200MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData500msUL*")'))
relData4APPUL500MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityData1sUL*")'))
relData4APPUL1MW <- datasetUL$scalars$value

datasetUL <- NULL


####################################################################################################
#V2X
###
datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X10msDL*")'))
relV2X4APPDL10MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X20msDL*")'))
relV2X4APPDL20MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X50msDL*")'))
relV2X4APPDL50MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X100msDL*")'))
relV2X4APPDL100MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X200msDL*")'))
relV2X4APPDL200MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X500msDL*")'))
relV2X4APPDL500MW <- datasetDL$scalars$value

datasetDL <- NULL

datasetDL <- loadDataset(c('TODO-INSERT-PATH-TO-DOWNLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X1sDL*")'))
relV2X4APPDL1MW <- datasetDL$scalars$value

datasetDL <- NULL



####################################################################################################
##UL

###
datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X10msUL*")'))
relV2X4APPUL10MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X20msUL*")'))
relV2X4APPUL20MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X50msUL*")'))
relV2X4APPUL50MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X100msUL*")'))
relV2X4APPUL100MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X200msUL*")'))
relV2X4APPUL200MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X500msUL*")'))
relV2X4APPUL500MW <- datasetUL$scalars$value

datasetUL <- NULL

datasetUL <- loadDataset(c('TODO-INSERT-PATH-TO-UPLINK-FOLDER/scalar-*.sca'),
                         add('scalar', 'name("reliabilityV2X1sUL*")'))
relV2X4APPUL1MW <- datasetUL$scalars$value

datasetUL <- NULL






##################################################################
#pdf
pdf("Reliability 4 Applications.pdf", width=11.0, height=7.5)
par(mfrow=c(2,2))

########################################################
theData <- list(relV2X4APPDL10MW,relV2X4APPDL20MW,relV2X4APPDL50MW,relV2X4APPDL100MW,relV2X4APPDL200MW,relV2X4APPDL500MW,relV2X4APPDL1MW,
                relV2X4APPUL10MW,relV2X4APPUL20MW,relV2X4APPUL50MW,relV2X4APPUL100MW,relV2X4APPUL200MW,relV2X4APPUL500MW,relV2X4APPUL1MW)
MW25 <- boxplot(theData,
                ylab = "Reliability",
                names = c("DL 10 ms","DL 20 ms","DL 50 ms","DL 100 ms","DL 200 ms", "DL 500 ms","DL 1 s",
                          "UL 10 ms","UL 20 ms","UL 50 ms","UL 100 ms","UL 200 ms", "UL 500 ms","UL 1 s"),
                main="V2X",
                #col=c("cornflowerblue","yellow"),
                at=c(1,2,3,4,5,6,7,8,9,10,11,12,13,14),
                #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                cex.axis = 0.9,
                cex.lab = 1,
                las=2,
                ylim=c(0,1),
                outline = FALSE,
                range=0,
                boxwex=0.6,
                staplewex=0.6)
abline(v=7.5,lty=1,lwd=1.5)

meanvalDL10 <- mean(relV2X4APPDL10MW)
meansDL10 <- matrix(c(meanvalDL10),nrow=1,byrow=TRUE)
points(x = col(meansDL10), y = meansDL10, pch = 3, lwd = 2, col = "red")

meanvalDL20 <- mean(relV2X4APPDL20MW)
meansDL20 <- matrix(c(meanvalDL20),nrow=1,byrow=TRUE)
points(x = col(meansDL20)+1, y = meansDL20, pch = 3, lwd = 2, col = "red")

meanvalDL50 <- mean(relV2X4APPDL50MW)
meansDL50 <- matrix(c(meanvalDL50),nrow=1,byrow=TRUE)
points(x = col(meansDL50)+2, y = meansDL50, pch = 3, lwd = 2, col = "red")

meanvalDL100 <- mean(relV2X4APPDL100MW)
meansDL100 <- matrix(c(meanvalDL100),nrow=1,byrow=TRUE)
points(x = col(meansDL100)+3, y = meansDL100, pch = 3, lwd = 2, col = "red")

meanvalDL200 <- mean(relV2X4APPDL200MW)
meansDL200 <- matrix(c(meanvalDL200),nrow=1,byrow=TRUE)
points(x = col(meansDL200)+4, y = meansDL200, pch = 3, lwd = 2, col = "red")

meanvalDL500 <- mean(relV2X4APPDL500MW)
meansDL500 <- matrix(c(meanvalDL500),nrow=1,byrow=TRUE)
points(x = col(meansDL500)+5, y = meansDL500, pch = 3, lwd = 2, col = "red")

meanvalDL1 <- mean(relV2X4APPDL1MW)
meansDL1 <- matrix(c(meanvalDL1),nrow=1,byrow=TRUE)
points(x = col(meansDL1)+6, y = meansDL1, pch = 3, lwd = 2, col = "red")



meanvalUL10 <- mean(relV2X4APPUL10MW)
meansUL10 <- matrix(c(meanvalUL10),nrow=1,byrow=TRUE)
points(x = col(meansUL10)+7, y = meansUL10, pch = 3, lwd = 2, col = "red")

meanvalUL20 <- mean(relV2X4APPUL20MW)
meansUL20 <- matrix(c(meanvalUL20),nrow=1,byrow=TRUE)
points(x = col(meansUL20)+8, y = meansUL20, pch = 3, lwd = 2, col = "red")

meanvalUL50 <- mean(relV2X4APPUL50MW)
meansUL50 <- matrix(c(meanvalUL50),nrow=1,byrow=TRUE)
points(x = col(meansUL50)+9, y = meansUL50, pch = 3, lwd = 2, col = "red")

meanvalUL100 <- mean(relV2X4APPUL100MW)
meansUL100 <- matrix(c(meanvalUL100),nrow=1,byrow=TRUE)
points(x = col(meansUL100)+10, y = meansUL100, pch = 3, lwd = 2, col = "red")

meanvalUL200 <- mean(relV2X4APPUL200MW)
meansUL200 <- matrix(c(meanvalUL200),nrow=1,byrow=TRUE)
points(x = col(meansUL200)+11, y = meansUL200, pch = 3, lwd = 2, col = "red")

meanvalUL500 <- mean(relV2X4APPUL500MW)
meansUL500 <- matrix(c(meanvalUL500),nrow=1,byrow=TRUE)
points(x = col(meansUL500)+12, y = meansUL500, pch = 3, lwd = 2, col = "red")

meanvalUL1 <- mean(relV2X4APPUL1MW)
meansUL1 <- matrix(c(meanvalUL1),nrow=1,byrow=TRUE)
points(x = col(meansUL1)+13, y = meansUL1, pch = 3, lwd = 2, col = "red")

#clean
meansDL10 <- NULL
meansUL10 <- NULL
meanvalDL10 <- NULL
meanvalUL10 <- NULL
meansDL20 <- NULL
meansUL20 <- NULL
meanvalDL20 <- NULL
meanvalUL20 <- NULL
meansDL50 <- NULL
meansUL50 <- NULL
meanvalDL50 <- NULL
meanvalUL50 <- NULL
meansDL100 <- NULL
meansUL100 <- NULL
meanvalDL100 <- NULL
meanvalUL100 <- NULL
meansDL200 <- NULL
meansUL200 <- NULL
meanvalDL200 <- NULL
meanvalUL200 <- NULL
meansDL500 <- NULL
meansUL500 <- NULL
meanvalDL500 <- NULL
meanvalUL500 <- NULL
meansDL1 <- NULL
meansUL1 <- NULL
meanvalDL1 <- NULL
meanvalUL1 <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(relVoip4APPDL10MW,relVoip4APPDL20MW,relVoip4APPDL50MW,relVoip4APPDL100MW,relVoip4APPDL200MW,relVoip4APPDL500MW,relVoip4APPDL1MW,
                relVoip4APPUL10MW,relVoip4APPUL20MW,relVoip4APPUL50MW,relVoip4APPUL100MW,relVoip4APPUL200MW,relVoip4APPUL500MW,relVoip4APPUL1MW)
MW50 <- boxplot(theData,
                ylab = "Reliability",
                names = c("DL 10 ms","DL 20 ms","DL 50 ms","DL 100 ms","DL 200 ms", "DL 500 ms","DL 1 s",
                          "UL 10 ms","UL 20 ms","UL 50 ms","UL 100 ms","UL 200 ms", "UL 500 ms","UL 1 s"),
                main="VoIP",
                #col=c("cornflowerblue","yellow"),
                at=c(1,2,3,4,5,6,7,8,9,10,11,12,13,14),
                #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                cex.axis = 0.9,
                cex.lab = 1,
                las=2,
                ylim=c(0,1),
                outline = FALSE,
                range=0,
                boxwex=0.6,
                staplewex=0.6)
abline(v=7.5,lty=1,lwd=1.5)

meanvalDL10 <- mean(relVoip4APPDL10MW)
meansDL10 <- matrix(c(meanvalDL10),nrow=1,byrow=TRUE)
points(x = col(meansDL10), y = meansDL10, pch = 3, lwd = 2, col = "red")

meanvalDL20 <- mean(relVoip4APPDL20MW)
meansDL20 <- matrix(c(meanvalDL20),nrow=1,byrow=TRUE)
points(x = col(meansDL20)+1, y = meansDL20, pch = 3, lwd = 2, col = "red")

meanvalDL50 <- mean(relVoip4APPDL50MW)
meansDL50 <- matrix(c(meanvalDL50),nrow=1,byrow=TRUE)
points(x = col(meansDL50)+2, y = meansDL50, pch = 3, lwd = 2, col = "red")

meanvalDL100 <- mean(relVoip4APPDL100MW)
meansDL100 <- matrix(c(meanvalDL100),nrow=1,byrow=TRUE)
points(x = col(meansDL100)+3, y = meansDL100, pch = 3, lwd = 2, col = "red")

meanvalDL200 <- mean(relVoip4APPDL200MW)
meansDL200 <- matrix(c(meanvalDL200),nrow=1,byrow=TRUE)
points(x = col(meansDL200)+4, y = meansDL200, pch = 3, lwd = 2, col = "red")

meanvalDL500 <- mean(relVoip4APPDL500MW)
meansDL500 <- matrix(c(meanvalDL500),nrow=1,byrow=TRUE)
points(x = col(meansDL500)+5, y = meansDL500, pch = 3, lwd = 2, col = "red")

meanvalDL1 <- mean(relVoip4APPDL1MW)
meansDL1 <- matrix(c(meanvalDL1),nrow=1,byrow=TRUE)
points(x = col(meansDL1)+6, y = meansDL1, pch = 3, lwd = 2, col = "red")



meanvalUL10 <- mean(relVoip4APPUL10MW)
meansUL10 <- matrix(c(meanvalUL10),nrow=1,byrow=TRUE)
points(x = col(meansUL10)+7, y = meansUL10, pch = 3, lwd = 2, col = "red")

meanvalUL20 <- mean(relVoip4APPUL20MW)
meansUL20 <- matrix(c(meanvalUL20),nrow=1,byrow=TRUE)
points(x = col(meansUL20)+8, y = meansUL20, pch = 3, lwd = 2, col = "red")

meanvalUL50 <- mean(relVoip4APPUL50MW)
meansUL50 <- matrix(c(meanvalUL50),nrow=1,byrow=TRUE)
points(x = col(meansUL50)+9, y = meansUL50, pch = 3, lwd = 2, col = "red")

meanvalUL100 <- mean(relVoip4APPUL100MW)
meansUL100 <- matrix(c(meanvalUL100),nrow=1,byrow=TRUE)
points(x = col(meansUL100)+10, y = meansUL100, pch = 3, lwd = 2, col = "red")

meanvalUL200 <- mean(relVoip4APPUL200MW)
meansUL200 <- matrix(c(meanvalUL200),nrow=1,byrow=TRUE)
points(x = col(meansUL200)+11, y = meansUL200, pch = 3, lwd = 2, col = "red")

meanvalUL500 <- mean(relVoip4APPUL500MW)
meansUL500 <- matrix(c(meanvalUL500),nrow=1,byrow=TRUE)
points(x = col(meansUL500)+12, y = meansUL500, pch = 3, lwd = 2, col = "red")

meanvalUL1 <- mean(relVoip4APPUL1MW)
meansUL1 <- matrix(c(meanvalUL1),nrow=1,byrow=TRUE)
points(x = col(meansUL1)+13, y = meansUL1, pch = 3, lwd = 2, col = "red")

#clean
meansDL10 <- NULL
meansUL10 <- NULL
meanvalDL10 <- NULL
meanvalUL10 <- NULL
meansDL20 <- NULL
meansUL20 <- NULL
meanvalDL20 <- NULL
meanvalUL20 <- NULL
meansDL50 <- NULL
meansUL50 <- NULL
meanvalDL50 <- NULL
meanvalUL50 <- NULL
meansDL100 <- NULL
meansUL100 <- NULL
meanvalDL100 <- NULL
meanvalUL100 <- NULL
meansDL200 <- NULL
meansUL200 <- NULL
meanvalDL200 <- NULL
meanvalUL200 <- NULL
meansDL500 <- NULL
meansUL500 <- NULL
meanvalDL500 <- NULL
meanvalUL500 <- NULL
meansDL1 <- NULL
meansUL1 <- NULL
meanvalDL1 <- NULL
meanvalUL1 <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(relVideo4APPDL10MW,relVideo4APPDL20MW,relVideo4APPDL50MW,relVideo4APPDL100MW,relVideo4APPDL200MW,relVideo4APPDL500MW,relVideo4APPDL1MW,
                relVideo4APPUL10MW,relVideo4APPUL20MW,relVideo4APPUL50MW,relVideo4APPUL100MW,relVideo4APPUL200MW,relVideo4APPUL500MW,relVideo4APPUL1MW)
MW100 <- boxplot(theData,
                 ylab = "Reliability",
                 names = c("DL 10 ms","DL 20 ms","DL 50 ms","DL 100 ms","DL 200 ms", "DL 500 ms","DL 1 s",
                           "UL 10 ms","UL 20 ms","UL 50 ms","UL 100 ms","UL 200 ms", "UL 500 ms","UL 1 s"),
                 main="Video",
                 #col=c("cornflowerblue","yellow"),
                 at=c(1,2,3,4,5,6,7,8,9,10,11,12,13,14),
                 #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                 cex.axis = 0.9,
                 cex.lab = 1,
                 las=2,
                 ylim=c(0,1),
                 outline = FALSE,
                 range=0,
                 boxwex=0.6,
                 staplewex=0.6)
abline(v=7.5,lty=1,lwd=1.5)

meanvalDL10 <- mean(relVideo4APPDL10MW)
meansDL10 <- matrix(c(meanvalDL10),nrow=1,byrow=TRUE)
points(x = col(meansDL10), y = meansDL10, pch = 3, lwd = 2, col = "red")

meanvalDL20 <- mean(relVideo4APPDL20MW)
meansDL20 <- matrix(c(meanvalDL20),nrow=1,byrow=TRUE)
points(x = col(meansDL20)+1, y = meansDL20, pch = 3, lwd = 2, col = "red")

meanvalDL50 <- mean(relVideo4APPDL50MW)
meansDL50 <- matrix(c(meanvalDL50),nrow=1,byrow=TRUE)
points(x = col(meansDL50)+2, y = meansDL50, pch = 3, lwd = 2, col = "red")

meanvalDL100 <- mean(relVideo4APPDL100MW)
meansDL100 <- matrix(c(meanvalDL100),nrow=1,byrow=TRUE)
points(x = col(meansDL100)+3, y = meansDL100, pch = 3, lwd = 2, col = "red")

meanvalDL200 <- mean(relVideo4APPDL200MW)
meansDL200 <- matrix(c(meanvalDL200),nrow=1,byrow=TRUE)
points(x = col(meansDL200)+4, y = meansDL200, pch = 3, lwd = 2, col = "red")

meanvalDL500 <- mean(relVideo4APPDL500MW)
meansDL500 <- matrix(c(meanvalDL500),nrow=1,byrow=TRUE)
points(x = col(meansDL500)+5, y = meansDL500, pch = 3, lwd = 2, col = "red")

meanvalDL1 <- mean(relVideo4APPDL1MW)
meansDL1 <- matrix(c(meanvalDL1),nrow=1,byrow=TRUE)
points(x = col(meansDL1)+6, y = meansDL1, pch = 3, lwd = 2, col = "red")



meanvalUL10 <- mean(relVideo4APPUL10MW)
meansUL10 <- matrix(c(meanvalUL10),nrow=1,byrow=TRUE)
points(x = col(meansUL10)+7, y = meansUL10, pch = 3, lwd = 2, col = "red")

meanvalUL20 <- mean(relVideo4APPUL20MW)
meansUL20 <- matrix(c(meanvalUL20),nrow=1,byrow=TRUE)
points(x = col(meansUL20)+8, y = meansUL20, pch = 3, lwd = 2, col = "red")

meanvalUL50 <- mean(relVideo4APPUL50MW)
meansUL50 <- matrix(c(meanvalUL50),nrow=1,byrow=TRUE)
points(x = col(meansUL50)+9, y = meansUL50, pch = 3, lwd = 2, col = "red")

meanvalUL100 <- mean(relVideo4APPUL100MW)
meansUL100 <- matrix(c(meanvalUL100),nrow=1,byrow=TRUE)
points(x = col(meansUL100)+10, y = meansUL100, pch = 3, lwd = 2, col = "red")

meanvalUL200 <- mean(relVideo4APPUL200MW)
meansUL200 <- matrix(c(meanvalUL200),nrow=1,byrow=TRUE)
points(x = col(meansUL200)+11, y = meansUL200, pch = 3, lwd = 2, col = "red")

meanvalUL500 <- mean(relVideo4APPUL500MW)
meansUL500 <- matrix(c(meanvalUL500),nrow=1,byrow=TRUE)
points(x = col(meansUL500)+12, y = meansUL500, pch = 3, lwd = 2, col = "red")

meanvalUL1 <- mean(relVideo4APPUL1MW)
meansUL1 <- matrix(c(meanvalUL1),nrow=1,byrow=TRUE)
points(x = col(meansUL1)+13, y = meansUL1, pch = 3, lwd = 2, col = "red")

#clean
meansDL10 <- NULL
meansUL10 <- NULL
meanvalDL10 <- NULL
meanvalUL10 <- NULL
meansDL20 <- NULL
meansUL20 <- NULL
meanvalDL20 <- NULL
meanvalUL20 <- NULL
meansDL50 <- NULL
meansUL50 <- NULL
meanvalDL50 <- NULL
meanvalUL50 <- NULL
meansDL100 <- NULL
meansUL100 <- NULL
meanvalDL100 <- NULL
meanvalUL100 <- NULL
meansDL200 <- NULL
meansUL200 <- NULL
meanvalDL200 <- NULL
meanvalUL200 <- NULL
meansDL500 <- NULL
meansUL500 <- NULL
meanvalDL500 <- NULL
meanvalUL500 <- NULL
meansDL1 <- NULL
meansUL1 <- NULL
meanvalDL1 <- NULL
meanvalUL1 <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(relData4APPDL10MW,relData4APPDL20MW,relData4APPDL50MW,relData4APPDL100MW,relData4APPDL200MW,relData4APPDL500MW,relData4APPDL1MW,
                relData4APPUL10MW,relData4APPUL20MW,relData4APPUL50MW,relData4APPUL100MW,relData4APPUL200MW,relData4APPUL500MW,relData4APPUL1MW)
MW270 <- boxplot(theData,
                 ylab = "Reliability",
                 names = c("DL 10 ms","DL 20 ms","DL 50 ms","DL 100 ms","DL 200 ms", "DL 500 ms","DL 1 s",
                           "UL 10 ms","UL 20 ms","UL 50 ms","UL 100 ms","UL 200 ms", "UL 500 ms","UL 1 s"),
                 main="Data",
                 #col=c("cornflowerblue","yellow"),
                 at=c(1,2,3,4,5,6,7,8,9,10,11,12,13,14),
                 #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                 cex.axis = 0.9,
                 cex.lab = 1,
                 las=2,
                 ylim=c(0,1),
                 outline = FALSE,
                 range=0,
                 boxwex=0.6,
                 staplewex=0.6)
abline(v=7.5,lty=1,lwd=1.5)

meanvalDL10 <- mean(relData4APPDL10MW)
meansDL10 <- matrix(c(meanvalDL10),nrow=1,byrow=TRUE)
points(x = col(meansDL10), y = meansDL10, pch = 3, lwd = 2, col = "red")

meanvalDL20 <- mean(relData4APPDL20MW)
meansDL20 <- matrix(c(meanvalDL20),nrow=1,byrow=TRUE)
points(x = col(meansDL20)+1, y = meansDL20, pch = 3, lwd = 2, col = "red")

meanvalDL50 <- mean(relData4APPDL50MW)
meansDL50 <- matrix(c(meanvalDL50),nrow=1,byrow=TRUE)
points(x = col(meansDL50)+2, y = meansDL50, pch = 3, lwd = 2, col = "red")

meanvalDL100 <- mean(relData4APPDL100MW)
meansDL100 <- matrix(c(meanvalDL100),nrow=1,byrow=TRUE)
points(x = col(meansDL100)+3, y = meansDL100, pch = 3, lwd = 2, col = "red")

meanvalDL200 <- mean(relData4APPDL200MW)
meansDL200 <- matrix(c(meanvalDL200),nrow=1,byrow=TRUE)
points(x = col(meansDL200)+4, y = meansDL200, pch = 3, lwd = 2, col = "red")

meanvalDL500 <- mean(relData4APPDL500MW)
meansDL500 <- matrix(c(meanvalDL500),nrow=1,byrow=TRUE)
points(x = col(meansDL500)+5, y = meansDL500, pch = 3, lwd = 2, col = "red")

meanvalDL1 <- mean(relData4APPDL1MW)
meansDL1 <- matrix(c(meanvalDL1),nrow=1,byrow=TRUE)
points(x = col(meansDL1)+6, y = meansDL1, pch = 3, lwd = 2, col = "red")



meanvalUL10 <- mean(relData4APPUL10MW)
meansUL10 <- matrix(c(meanvalUL10),nrow=1,byrow=TRUE)
points(x = col(meansUL10)+7, y = meansUL10, pch = 3, lwd = 2, col = "red")

meanvalUL20 <- mean(relData4APPUL20MW)
meansUL20 <- matrix(c(meanvalUL20),nrow=1,byrow=TRUE)
points(x = col(meansUL20)+8, y = meansUL20, pch = 3, lwd = 2, col = "red")

meanvalUL50 <- mean(relData4APPUL50MW)
meansUL50 <- matrix(c(meanvalUL50),nrow=1,byrow=TRUE)
points(x = col(meansUL50)+9, y = meansUL50, pch = 3, lwd = 2, col = "red")

meanvalUL100 <- mean(relData4APPUL100MW)
meansUL100 <- matrix(c(meanvalUL100),nrow=1,byrow=TRUE)
points(x = col(meansUL100)+10, y = meansUL100, pch = 3, lwd = 2, col = "red")

meanvalUL200 <- mean(relData4APPUL200MW)
meansUL200 <- matrix(c(meanvalUL200),nrow=1,byrow=TRUE)
points(x = col(meansUL200)+11, y = meansUL200, pch = 3, lwd = 2, col = "red")

meanvalUL500 <- mean(relData4APPUL500MW)
meansUL500 <- matrix(c(meanvalUL500),nrow=1,byrow=TRUE)
points(x = col(meansUL500)+12, y = meansUL500, pch = 3, lwd = 2, col = "red")

meanvalUL1 <- mean(relData4APPUL1MW)
meansUL1 <- matrix(c(meanvalUL1),nrow=1,byrow=TRUE)
points(x = col(meansUL1)+13, y = meansUL1, pch = 3, lwd = 2, col = "red")

#clean
meansDL10 <- NULL
meansUL10 <- NULL
meanvalDL10 <- NULL
meanvalUL10 <- NULL
meansDL20 <- NULL
meansUL20 <- NULL
meanvalDL20 <- NULL
meanvalUL20 <- NULL
meansDL50 <- NULL
meansUL50 <- NULL
meanvalDL50 <- NULL
meanvalUL50 <- NULL
meansDL100 <- NULL
meansUL100 <- NULL
meanvalDL100 <- NULL
meanvalUL100 <- NULL
meansDL200 <- NULL
meansUL200 <- NULL
meanvalDL200 <- NULL
meanvalUL200 <- NULL
meansDL500 <- NULL
meansUL500 <- NULL
meanvalDL500 <- NULL
meanvalUL500 <- NULL
meansDL1 <- NULL
meansUL1 <- NULL
meanvalDL1 <- NULL
meanvalUL1 <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################
dev.off()







###################################################################################################################################################################
##################################################################################################################################################################
################## Jitter

##########4AL##################################################################
##########MW
###Voip
#
datasetDL <- loadDataset(c('TODO/vector-*.vec'),
                         add('vector', 'name("delayVoipVariationReal:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayVariationVoipMWDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO/vector-*.vec'),
                         add('vector', 'name("delayVoipVariationReal:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayVariationVoipMWUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL

###Video

datasetDL <- loadDataset(c('TODO/vector-*.vec'),
                         add('vector', 'name("delayVideoVariationReal:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayVariationVideoMWDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO/vector-*.vec'),
                         add('vector', 'name("delayVideoVariationReal:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayVariationVideoMWUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL

###V2x

datasetDL <- loadDataset(c('TODO/vector-*.vec'),
                         add('vector', 'name("delayV2XVariationReal:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayVariationV2xMWDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO/vector-*.vec'),
                         add('vector', 'name("delayV2XVariationReal:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayVariationV2xMWUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL

###Data

datasetDL <- loadDataset(c('TODO/vector-*.vec'),
                         add('vector', 'name("delayDataVariationReal:vector")'))
vecsDL <- loadVectors(datasetDL,NULL)
delayVariationDataMWDL4APPL <- vecsDL$vectordata

#

datasetUL <- loadDataset(c('TODO/vector-*.vec'),
                         add('vector', 'name("delayDataVariationReal:vector")'))
vecsUL <- loadVectors(datasetUL,NULL)
delayVariationDataMWUL4APPL <- vecsUL$vectordata

datasetDL <- NULL
vecsDL <- NULL
datasetUL <- NULL
vecsUL <- NULL


#################### Plott
#pdf
pdf("Jitter 4 Applications.pdf", width=11.0, height=7.5)
par(mfrow=c(2,2))

########################################################
theData <- list(delayVariationV2xMWDL4APPL$y,delayVariationV2xMWUL4APPL$y)
MW25 <- boxplot(theData,
                ylab = "Packet Delay Variation in s",
                names = c("Downlink","Uplink"),
                main="Motorway, V2X",
                #col=c("cornflowerblue","yellow"),
                #at=c(1,2),
                #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                cex.axis = 0.8,
                cex.lab = 1,
                las=1,
                ylim=c(-0.15,0.1),
                outline = FALSE,
                range=1,
                boxwex=0.6,
                staplewex=0.6)

abline(h=0,lty=3,lwd=0.5)
abline(h=-0.05,lty=3,lwd=0.5)
abline(h=-0.1,lty=3,lwd=0.5)
abline(h=-0.15,lty=3,lwd=0.5)
abline(h=0.05,lty=3,lwd=0.5)
abline(h=0.1,lty=3,lwd=0.5)


median <- matrix(c(MW25$stats[3,]),nrow=1,byrow=TRUE)
#text(x = col(median)+0.45, y = median, labels = round(median,digits=4), cex = 0.6, col="black")

meanvalDL <- mean(delayVariationV2xMWDL4APPL$y)
meanvalUL <- mean(delayVariationV2xMWUL4APPL$y)

meansDL <- matrix(c(meanvalDL),nrow=1,byrow=TRUE)
points(x = col(meansDL), y = meansDL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansDL)+0.45, y = meansDL+0.003, labels = round(meansDL,digits=4),cex = 0.6, col="red")

meansUL <- matrix(c(meanvalUL),nrow=1,byrow=TRUE)
points(x = col(meansUL)+1, y = meansUL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansUL)+1.45, y = meansUL, labels = round(meansUL,digits=4),cex = 0.6, col="red")

#clean
meansDL <- NULL
meansUL <- NULL
meanvalDL <- NULL
meanvalUL <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(delayVariationVoipMWDL4APPL$y,delayVariationVoipMWUL4APPL$y)
MW50 <- boxplot(theData,
                ylab = "Packet Delay Variation in s",
                names = c("Downlink","Uplink"),
                main="Motorway, VoIP",
                #col=c("cornflowerblue","yellow"),
                #at=c(1,2),
                #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                cex.axis = 0.8,
                cex.lab = 1,
                las=1,
                ylim=c(-0.03,0.03),
                outline = FALSE,
                range=1,
                boxwex=0.6,
                staplewex=0.6)

abline(h=0,lty=3,lwd=0.5)
abline(h=-0.01,lty=3,lwd=0.5)
abline(h=-0.02,lty=3,lwd=0.5)
abline(h=-0.03,lty=3,lwd=0.5)
abline(h=0.01,lty=3,lwd=0.5)
abline(h=0.02,lty=3,lwd=0.5)
abline(h=0.03,lty=3,lwd=0.5)

median <- matrix(c(MW50$stats[3,]),nrow=1,byrow=TRUE)
#text(x = col(median)+0.45, y = median, labels = round(median,digits=4), cex = 0.6, col="black")

meanvalDL <- mean(delayVariationVoipMWDL4APPL$y)
meanvalUL <- mean(delayVariationVoipMWUL4APPL$y)
meansDL <- matrix(c(meanvalDL),nrow=1,byrow=TRUE)
points(x = col(meansDL), y = meansDL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansDL)+0.45, y = meansDL+0.003, labels = round(meansDL,digits=4),cex = 0.6, col="red")

meansUL <- matrix(c(meanvalUL),nrow=1,byrow=TRUE)
points(x = col(meansUL)+1, y = meansUL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansUL)+1.45, y = meansUL+0.002, labels = round(meansUL,digits=4),cex = 0.6, col="red")

#clean
meansDL <- NULL
meansUL <- NULL
meanvalDL <- NULL
meanvalUL <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(delayVariationVideoMWDL4APPL$y,delayVariationVideoMWUL4APPL$y)
MW100 <- boxplot(theData,
                 ylab = "Packet Delay Variation in s",
                 names = c("Downlink","Uplink"),
                 main="Motorway, Video",
                 #col=c("cornflowerblue","yellow"),
                 #at=c(1,2),
                 #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                 cex.axis = 0.8,
                 cex.lab = 1,
                 las=1,
                 ylim=c(-0.1,0.1),
                 outline = FALSE,
                 range=1,
                 boxwex=0.6,
                 staplewex=0.6)

abline(h=0,lty=3,lwd=0.5)
abline(h=-0.05,lty=3,lwd=0.5)
abline(h=-0.1,lty=3,lwd=0.5)
abline(h=-0.15,lty=3,lwd=0.5)
abline(h=0.05,lty=3,lwd=0.5)
abline(h=0.1,lty=3,lwd=0.5)

median <- matrix(c(MW100$stats[3,]),nrow=1,byrow=TRUE)
#text(x = col(median)+0.45, y = median, labels = round(median,digits=4), cex = 0.6, col="black")

meanvalDL <- mean(delayVariationVideoMWDL4APPL$y)
meanvalUL <- mean(delayVariationVideoMWUL4APPL$y)
meansDL <- matrix(c(meanvalDL),nrow=1,byrow=TRUE)
points(x = col(meansDL), y = meansDL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansDL)+0.45, y = meansDL+0.005, labels = round(meansDL,digits=4),cex = 0.6, col="red")

meansUL <- matrix(c(meanvalUL),nrow=1,byrow=TRUE)
points(x = col(meansUL)+1, y = meansUL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansUL)+1.45, y = meansUL-0.005, labels = round(meansUL,digits=4),cex = 0.6, col="red")

#clean
meansDL <- NULL
meansUL <- NULL
meanvalDL <- NULL
meanvalUL <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################

########################################################
theData <- list(delayVariationDataMWDL4APPL$y,delayVariationDataMWUL4APPL$y)
MW270 <- boxplot(theData,
                 ylab = "Packet Delay Variation in s",
                 names = c("Downlink","Uplink"),
                 main="Motorway, Data",
                 #col=c("cornflowerblue","yellow"),
                 #at=c(1,2),
                 #par(mar=c(8.5,4.5,0.5,0.5)+0.1),
                 cex.axis = 0.8,
                 cex.lab = 1,
                 las=1,
                 ylim=c(-0.15,0.15),
                 outline = FALSE,
                 range=1,
                 boxwex=0.6,
                 staplewex=0.6)

abline(h=0,lty=3,lwd=0.5)
abline(h=-0.05,lty=3,lwd=0.5)
abline(h=-0.1,lty=3,lwd=0.5)
abline(h=-0.15,lty=3,lwd=0.5)
abline(h=0.05,lty=3,lwd=0.5)
abline(h=0.1,lty=3,lwd=0.5)
abline(h=0.15,lty=3,lwd=0.5)

median <- matrix(c(MW270$stats[3,]),nrow=1,byrow=TRUE)
#text(x = col(median)+0.45, y = median, labels = round(median,digits=4), cex = 0.6, col="black")

meanvalDL <- mean(delayVariationDataMWDL4APPL$y)
meanvalUL <- mean(delayVariationDataMWUL4APPL$y)
meansDL <- matrix(c(meanvalDL),nrow=1,byrow=TRUE)
points(x = col(meansDL), y = meansDL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansDL)+0.45, y = meansDL+0.005, labels = round(meansDL,digits=4),cex = 0.6, col="red")

meansUL <- matrix(c(meanvalUL),nrow=1,byrow=TRUE)
points(x = col(meansUL)+1, y = meansUL, pch = 3, lwd = 2, col = "red")
#text(x = col(meansUL)+1.45, y = meansUL-0.002, labels = round(meansUL,digits=4),cex = 0.6, col="red")

#clean
meansDL <- NULL
meansUL <- NULL
meanvalDL <- NULL
meanvalUL <- NULL
theData <- NULL
median <- NULL
MW25 <- NULL
MW50 <- NULL
MW100 <- NULL
MW270 <- NULL
UR25 <- NULL
UR50 <- NULL
UR100 <- NULL
UR270 <- NULL
#######################################################
dev.off()






















