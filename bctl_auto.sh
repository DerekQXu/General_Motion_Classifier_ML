#!/bin/bash

# get the MAC Addresses
i=0
MAC=("123" "456")
cat bctl.txt | 
{
while read LINE
do
  MAC[$i]="$LINE"
  i=$i+1
done

# run Automated Gattool Connector
exp='./bctl_helper.exp '
rm -r training_set
mkdir training_set
eval $exp${MAC[1]}${MAC[2]}

# clear output
clear
}
