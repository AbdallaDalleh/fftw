#!../../bin/linux-x86_64/fftwtest

## You may have to change fftwtest to something else
## everywhere it appears in this file

#< envPaths

## Register all support components
dbLoadDatabase("../../dbd/fftwtest.dbd",0,0)
fftwtest_registerRecordDeviceDriver(pdbbase) 

## Load record instances
dbLoadRecords("../../db/single.db","P=A1,R=:,TIME_N=128,FREQ_N=65")
dbLoadRecords("../../db/single.db","P=A2,R=:,TIME_N=1024,FREQ_N=513")

dbLoadRecords("../../db/single_asub.db","P=A3,R=:,TIME_N=1024,FREQ_N=513")
dbLoadRecords("../../db/single_asub_issue001.db","P=A4,R=:,TIME_N=65636,FREQ_N=32769")

iocInit()

## Start any sequence programs
#seq sncfftwtest,"user=ralph"
