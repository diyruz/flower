#!/bin/bash

GW_HOST=http://192.168.1.209
IEEE_ADDR=0x00124B001FB1E013
DST_ADDR="0x000"
ENDPOINST_COUNT=20
for i in $(seq 1 $ENDPOINST_COUNT); do 
URL="$GW_HOST/api/zigbee/bind?action=bind&dev=$IEEE_ADDR&SrcEp=$i&ClusterId=6&DstNwkAddr=$DST_ADDR&DstEp=1"
curl $URL
done
